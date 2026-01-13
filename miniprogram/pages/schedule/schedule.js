// pages/schedule/schedule.js - 日程管理逻辑
const app = getApp();

Page({
  data: {
    activeTab: 'list',
    schedules: [],
    todaySchedules: [],
    futureSchedules: [],

    // 日历相关
    calendarYear: 2024,
    calendarMonth: 1,
    calendarDays: [],
    weekDays: ['日', '一', '二', '三', '四', '五', '六'],
    selectedDate: '',
    selectedDateSchedules: [],

    // 表单相关
    showModal: false,
    editingItem: null,
    formData: {
      date: '',
      startTime: '09:00',
      endTime: '10:00',
      content: '',
      category: 'work'
    },
    submitting: false,

    categoryNames: {
      'work': '工作',
      'life': '生活',
      'study': '学习'
    }
  },

  onLoad() {
    this.initCalendar();
    this.loadSchedules();
  },

  onShow() {
    // 返回该页时刷新数据（从编辑页回来也会触发）
    if (!app.globalData.currentDevice || !app.globalData.currentDevice.ip) {
      wx.showModal({
        title: '未选择设备',
        content: '当前设备IP缺失，请先回到首页选择设备。',
        showCancel: false,
        success: () => wx.switchTab({ url: '/pages/index/index' })
      });
      return;
    }
    this.loadSchedules();
  },

  // 初始化日历
  initCalendar() {
    const now = new Date();
    this.setData({
      calendarYear: now.getFullYear(),
      calendarMonth: now.getMonth() + 1,
      selectedDate: this.formatDate(now)
    });
    this.generateCalendarDays();
  },

  // 生成日历天数
  generateCalendarDays() {
    const { calendarYear, calendarMonth } = this.data;
    const firstDay = new Date(calendarYear, calendarMonth - 1, 1);
    const lastDay = new Date(calendarYear, calendarMonth, 0);
    const startDay = firstDay.getDay();
    const totalDays = lastDay.getDate();

    const today = this.formatDate(new Date());
    const days = [];

    // 上个月的天数
    const prevMonthLastDay = new Date(calendarYear, calendarMonth - 1, 0).getDate();
    for (let i = startDay - 1; i >= 0; i--) {
      const d = prevMonthLastDay - i;
      const date = this.formatDateYMD(calendarYear, calendarMonth - 1, d);
      days.push({
        day: d,
        date: date,
        isCurrentMonth: false,
        isToday: date === today,
        hasSchedule: this.hasScheduleOnDate(date)
      });
    }

    // 当前月的天数
    for (let i = 1; i <= totalDays; i++) {
      const date = this.formatDateYMD(calendarYear, calendarMonth, i);
      days.push({
        day: i,
        date: date,
        isCurrentMonth: true,
        isToday: date === today,
        isSelected: date === this.data.selectedDate,
        hasSchedule: this.hasScheduleOnDate(date)
      });
    }

    // 下个月的天数（补齐到42个格子）
    const remaining = 42 - days.length;
    for (let i = 1; i <= remaining; i++) {
      const date = this.formatDateYMD(calendarYear, calendarMonth + 1, i);
      days.push({
        day: i,
        date: date,
        isCurrentMonth: false,
        isToday: date === today,
        hasSchedule: this.hasScheduleOnDate(date)
      });
    }

    this.setData({ calendarDays: days });
  },

  // 检查日期是否有日程
  hasScheduleOnDate(date) {
    return this.data.schedules.some(s => s.date === date);
  },

  // 格式化日期
  formatDate(date) {
    const year = date.getFullYear();
    const month = String(date.getMonth() + 1).padStart(2, '0');
    const day = String(date.getDate()).padStart(2, '0');
    return `${year}-${month}-${day}`;
  },

  formatDateYMD(year, month, day) {
    // 处理月份溢出
    const date = new Date(year, month - 1, day);
    return this.formatDate(date);
  },

  // 加载日程数据
  async loadSchedules() {
    // 兼容代码：不需要传 device 参数，直接用路径
    // const device = app.globalData.currentDevice;
    // if (!device) return;

    try {
      const data = await app.requestDeviceCompat(app.globalData.currentDevice, '/api/data');
      const schedules = data.schedule || [];

      // 排序
      schedules.sort((a, b) => (a.date + a.startTime).localeCompare(b.date + b.startTime));

      const today = this.formatDate(new Date());
      const todaySchedules = schedules.filter(s => s.date === today);
      const futureSchedules = schedules.filter(s => s.date > today);

      this.setData({
        schedules,
        todaySchedules,
        futureSchedules
      });

      // 更新选中日期的日程
      this.updateSelectedDateSchedules();
      // 更新日历标记
      this.generateCalendarDays();

    } catch (err) {
      console.error('Load schedules failed:', err);
      wx.showToast({ title: '加载失败', icon: 'error' });
    }
  },

  // 更新选中日期的日程
  updateSelectedDateSchedules() {
    const { schedules, selectedDate } = this.data;
    const selectedDateSchedules = schedules.filter(s => s.date === selectedDate);
    this.setData({ selectedDateSchedules });
  },

  // 切换标签页
  switchTab(e) {
    const tab = e.currentTarget.dataset.tab;
    this.setData({ activeTab: tab });
  },

  // 上个月
  prevMonth() {
    let { calendarYear, calendarMonth } = this.data;
    calendarMonth--;
    if (calendarMonth < 1) {
      calendarMonth = 12;
      calendarYear--;
    }
    this.setData({ calendarYear, calendarMonth });
    this.generateCalendarDays();
  },

  // 下个月
  nextMonth() {
    let { calendarYear, calendarMonth } = this.data;
    calendarMonth++;
    if (calendarMonth > 12) {
      calendarMonth = 1;
      calendarYear++;
    }
    this.setData({ calendarYear, calendarMonth });
    this.generateCalendarDays();
  },

  // 选择日期
  selectDate(e) {
    const date = e.currentTarget.dataset.date;
    this.setData({ selectedDate: date });
    this.generateCalendarDays();
    this.updateSelectedDateSchedules();
  },

  // 获取时间段
  getPeriod(time) {
    const hour = parseInt(time.split(':')[0]);
    if (hour < 12) return '上午';
    if (hour < 18) return '下午';
    return '晚上';
  },

  // 显示添加弹窗
  showAddModal() {
    // 改为独立页面编辑，彻底避开键盘顶飞 modal 的兼容问题
    const today = this.formatDate(new Date());
    const date = this.data.selectedDate || today;
    wx.navigateTo({
      url: `/pages/schedule_edit/schedule_edit?mode=add&date=${encodeURIComponent(date)}`
    });
  },

  // 编辑日程
  editSchedule(e) {
    const item = e.currentTarget.dataset.item;
    wx.navigateTo({
      url: `/pages/schedule_edit/schedule_edit?mode=edit&item=${encodeURIComponent(JSON.stringify(item))}`
    });
  },

  // 隐藏弹窗（保留，兼容旧UI，不再使用）
  hideModal() {
    this.setData({ showModal: false });
  },

  // 表单事件处理
  onDateChange(e) {
    this.setData({ 'formData.date': e.detail.value });
  },

  onStartTimeChange(e) {
    this.setData({ 'formData.startTime': e.detail.value });
  },

  onEndTimeChange(e) {
    this.setData({ 'formData.endTime': e.detail.value });
  },

  onContentInput(e) {
    this.setData({ 'formData.content': e.detail.value });
  },

  selectCategory(e) {
    const category = e.currentTarget.dataset.category;
    this.setData({ 'formData.category': category });
  },

  // 提交日程
  async submitSchedule() {
    const { formData, editingItem } = this.data;

    // 针对中文显示的提示：如果输入包含中文，提醒用户
    if (/[\u4e00-\u9fa5]/.test(formData.content.trim())) {
        const res = await new Promise(resolve => {
            wx.showModal({
                title: '友情提示',
                content: '设备屏幕可能仅支持显示英文/拼音，中文可能会显示为空白或乱码。确认继续吗？',
                success: resolve
            });
        });
        if (!res.confirm) return;
    }

    if (!formData.content.trim()) {
      wx.showToast({ title: '请输入内容', icon: 'none' });
      return;
    }

    this.setData({ submitting: true });

    // 同样使用兼容方法
    try {
      const payload = {
        action: 'add',
        item: {
          date: formData.date,
          startTime: formData.startTime,
          endTime: formData.endTime,
          content: formData.content, // 如果设备不支持中文，这里发过去也是乱码
          category: formData.category,
          isDone: false
        }
      };

      // 如果是编辑，先删除旧的再添加新的（因为设备端可能没有update接口或逻辑简单）
      if (editingItem && editingItem.id) {
          await app.requestDeviceCompat(app.globalData.currentDevice, '/api/schedule', 'POST', {
              action: 'del',
              id: editingItem.id
          });
      }

      await app.requestDeviceCompat(app.globalData.currentDevice, '/api/schedule', 'POST', payload);

      wx.showToast({ title: '保存成功', icon: 'success' });
      this.setData({ showModal: false, submitting: false });

      // 延迟加载，给设备写入一点时间
      setTimeout(() => this.loadSchedules(), 500);

    } catch (err) {
      console.error('Submit failed:', err);
      this.setData({ submitting: false });
      wx.showToast({ title: '保存失败', icon: 'error' });
    }
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
          try {
            await app.requestDeviceCompat(app.globalData.currentDevice, '/api/schedule', 'POST', {
              action: 'del',
              id: id
            });
            wx.showToast({ title: '已删除', icon: 'success' });
            setTimeout(() => this.loadSchedules(), 500);
          } catch (err) {
            wx.showToast({ title: '删除失败', icon: 'error' });
          }
        }
      }
    });
  }
});
