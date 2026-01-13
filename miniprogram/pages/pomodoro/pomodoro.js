// pages/pomodoro/pomodoro.js - 番茄钟逻辑
const app = getApp();

Page({
  data: {
    activeTab: 'timer',

    // 计时器状态
    isFocusing: false,
    focusDuration: 25, // 分钟
    remainingSeconds: 25 * 60,
    timerDisplay: '25:00',

    // 任务相关
    currentTask: null,
    taskList: [],
    showTaskModal: false,
    quickTaskName: '',

    // 统计数据
    todayFocusTime: '0分钟',
    weekFocusTime: '0小时0分',
    monthFocusTime: '0小时0分',
    totalPomodoros: 0,
    streakDays: 0,
    categoryStats: {
      work: 45,
      study: 30,
      life: 25
    },

    // 日程相关
    currentDateStr: '',
    todaySchedules: []
  },

  timer: null,

  onLoad() {
    this.updateDateStr();
    this.loadData();
  },

  onShow() {
    this.loadData();
  },

  onUnload() {
    this.stopTimer();
  },

  // 更新日期显示
  updateDateStr() {
    const now = new Date();
    const month = now.getMonth() + 1;
    const day = now.getDate();
    const weekDays = ['日', '一', '二', '三', '四', '五', '六'];
    this.setData({
      currentDateStr: `${month}月${day}日 星期${weekDays[now.getDay()]}`
    });
  },

  // 加载数据
  async loadData() {
    const device = app.globalData.currentDevice;
    if (!device) return;

    try {
      const data = await app.requestDevice(device, '/api/data');

      // 处理日程
      const today = this.getToday();
      const todaySchedules = (data.schedule || []).filter(s => s.date === today);

      // 处理统计
      const todayMinutes = data.pomodoro?.today || 0;
      const totalMinutes = data.pomodoro?.total || 0;

      this.setData({
        todaySchedules,
        taskList: todaySchedules,
        todayFocusTime: this.formatMinutes(todayMinutes),
        weekFocusTime: this.formatHoursMinutes(todayMinutes * 3), // 模拟数据
        monthFocusTime: this.formatHoursMinutes(totalMinutes),
        totalPomodoros: Math.floor(totalMinutes / 25)
      });

    } catch (err) {
      console.error('Load data failed:', err);
    }
  },

  getToday() {
    const d = new Date();
    return `${d.getFullYear()}-${String(d.getMonth()+1).padStart(2,'0')}-${String(d.getDate()).padStart(2,'0')}`;
  },

  formatMinutes(mins) {
    if (mins < 60) return `${mins}分钟`;
    const hours = Math.floor(mins / 60);
    const minutes = mins % 60;
    return `${hours}小时${minutes}分钟`;
  },

  formatHoursMinutes(mins) {
    const hours = Math.floor(mins / 60);
    const minutes = mins % 60;
    return `${hours}小时${minutes}分`;
  },

  // 切换标签
  switchTab(e) {
    const tab = e.currentTarget.dataset.tab;
    this.setData({ activeTab: tab });
  },

  // 设置时长
  setDuration(e) {
    if (this.data.isFocusing) return;

    const duration = parseInt(e.currentTarget.dataset.duration);
    this.setData({
      focusDuration: duration,
      remainingSeconds: duration * 60,
      timerDisplay: `${String(duration).padStart(2, '0')}:00`
    });
  },

  // 切换计时器
  toggleTimer() {
    if (this.data.isFocusing) {
      this.pauseTimer();
    } else {
      this.startTimer();
    }
  },

  // 开始计时
  startTimer() {
    this.setData({ isFocusing: true });

    this.timer = setInterval(() => {
      let remaining = this.data.remainingSeconds - 1;

      if (remaining <= 0) {
        this.completeTimer();
        return;
      }

      const mins = Math.floor(remaining / 60);
      const secs = remaining % 60;

      this.setData({
        remainingSeconds: remaining,
        timerDisplay: `${String(mins).padStart(2, '0')}:${String(secs).padStart(2, '0')}`
      });
    }, 1000);

    // 同步到设备
    this.syncToDevice('start');
  },

  // 暂停计时
  pauseTimer() {
    this.stopTimer();
    this.setData({ isFocusing: false });
  },

  // 停止计时器
  stopTimer() {
    if (this.timer) {
      clearInterval(this.timer);
      this.timer = null;
    }
  },

  // 完成计时
  completeTimer() {
    this.stopTimer();

    wx.vibrateLong();
    wx.showModal({
      title: '专注完成！',
      content: `恭喜完成 ${this.data.focusDuration} 分钟专注`,
      showCancel: false
    });

    this.setData({
      isFocusing: false,
      remainingSeconds: this.data.focusDuration * 60,
      timerDisplay: `${String(this.data.focusDuration).padStart(2, '0')}:00`
    });

    // 同步到设备
    this.syncToDevice('complete');
    this.loadData();
  },

  // 重置计时器
  resetTimer() {
    this.stopTimer();
    const duration = this.data.focusDuration;
    this.setData({
      isFocusing: false,
      remainingSeconds: duration * 60,
      timerDisplay: `${String(duration).padStart(2, '0')}:00`
    });
  },

  // 跳过计时器
  skipTimer() {
    wx.showModal({
      title: '确认跳过',
      content: '确定要跳过当前专注吗？这次专注将不会被记录。',
      success: (res) => {
        if (res.confirm) {
          this.resetTimer();
        }
      }
    });
  },

  // 同步到设备
  async syncToDevice(action) {
    const device = app.globalData.currentDevice;
    if (!device) return;

    try {
      // 这里可以添加向设备同步专注状态的逻辑
      console.log('Sync to device:', action);
    } catch (err) {
      console.error('Sync failed:', err);
    }
  },

  // 显示任务选择
  selectTask() {
    this.setData({ showTaskModal: true });
  },

  hideTaskModal() {
    this.setData({ showTaskModal: false });
  },

  // 选择任务
  selectTaskItem(e) {
    const task = e.currentTarget.dataset.task;
    this.setData({
      currentTask: task,
      showTaskModal: false
    });
  },

  // 切换任务完成状态
  toggleTask() {
    if (!this.data.currentTask) return;
    const task = this.data.currentTask;
    task.isDone = !task.isDone;
    this.setData({ currentTask: task });
  },

  // 快速添加任务
  onQuickInput(e) {
    this.setData({ quickTaskName: e.detail.value });
  },

  async quickAddTask() {
    const name = this.data.quickTaskName.trim();
    if (!name) {
      wx.showToast({ title: '请输入任务名称', icon: 'none' });
      return;
    }

    const device = app.globalData.currentDevice;
    if (!device) return;

    try {
      const today = this.getToday();
      await app.requestDevice(device, '/api/schedule', 'POST', {
        action: 'add',
        item: {
          date: today,
          startTime: '09:00',
          endTime: '10:00',
          content: name,
          category: 'work',
          isDone: false
        }
      });

      wx.showToast({ title: '添加成功', icon: 'success' });
      this.setData({ quickTaskName: '', showTaskModal: false });
      this.loadData();

    } catch (err) {
      wx.showToast({ title: '添加失败', icon: 'error' });
    }
  },

  // 跳转日程页面
  goToSchedule() {
    wx.navigateTo({
      url: '/pages/schedule/schedule'
    });
  },

  // 删除日程
  deleteSchedule(e) {
    const id = e.currentTarget.dataset.id;
    wx.showModal({
      title: '确认删除',
      content: '确定要删除这条日程吗？',
      confirmColor: '#ef4444',
      success: async (res) => {
        if (res.confirm) {
          const device = app.globalData.currentDevice;
          try {
            await app.requestDevice(device, '/api/schedule', 'POST', {
              action: 'del',
              id: id
            });
            wx.showToast({ title: '已删除', icon: 'success' });
            this.loadData();
          } catch (err) {
            wx.showToast({ title: '删除失败', icon: 'error' });
          }
        }
      }
    });
  }
});

