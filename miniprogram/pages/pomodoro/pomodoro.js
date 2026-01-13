// pages/pomodoro/pomodoro.js
const app = getApp();

Page({
  data: {
    activeTab: 'timer',
    isFocusing: false,
    remaining: '25:00',
    progress: 0,
    durations: [1, 5, 10, 15, 20, 25, 30, 45, 60, 90, 120],
    durationIndex: 5,
    todayFocus: 0,
    totalFocus: 0,

    // Stats data
    weekFocusTime: 0,
    monthFocusTime: 0,
    totalPomodoros: 0,
    streakDays: 0,
    categoryStats: { work: 0, study: 0, life: 0 },

    // Schedule data
    todaySchedules: [],

    timer: null
  },

  onShow() {
    this.fetchData();
    this.startLocalTimer(); // Keep UI updated
  },

  onHide() {
    this.stopLocalTimer();
  },

  switchTab(e) {
    this.setData({ activeTab: e.currentTarget.dataset.tab });
  },

  async fetchData() {
    try {
      // 兼容调用：使用 requestDeviceCompat 防止参数错误
      const data = await app.requestDeviceCompat(app.globalData.currentDevice, '/api/data');

      // Update Settings & Stats
      const duration = data.settings.focusDuration || 25;
      const dIndex = this.data.durations.indexOf(duration);

      this.setData({
        durationIndex: dIndex >= 0 ? dIndex : 5,
        todayFocus: data.pomodoro.today,
        totalFocus: data.pomodoro.total,
        // TODO: Calculate advanced stats from data.focusRecords if needed
        weekFocusTime: data.pomodoro.today, // Placeholder
        monthFocusTime: data.pomodoro.total, // Placeholder
        totalPomodoros: data.focusRecords.length
      });

      // Update Schedule
      // Filter today's schedule
      const todayStr = new Date().toISOString().split('T')[0]; // Simple YYYY-MM-DD
      // Note: Device stores date as string, check format match
      // Device format: YYYY-MM-DD
      const todayTasks = (data.schedule || []).filter(task => task.date === todayStr);
      this.setData({ todaySchedules: todayTasks });

    } catch (e) {
      console.error('Fetch data failed', e);
    }
  },

  async onDurationChange(e) {
    const idx = e.detail.value;
    const duration = this.data.durations[idx];

    this.setData({ durationIndex: idx });

    try {
      await app.requestDeviceCompat(app.globalData.currentDevice, '/api/settings', 'POST', { focusDuration: duration });
      wx.showToast({ title: '时长已更新', icon: 'success' });
      this.updateTimerDisplay(duration * 60); // Reset display
    } catch (e) {
      wx.showToast({ title: '设置失败', icon: 'none' });
    }
  },

  async toggleFocus() {
    const isFocusing = !this.data.isFocusing;
    const cmd = isFocusing ? 'start_focus' : 'stop_focus';

    try {
      await app.requestDeviceCompat(app.globalData.currentDevice, '/api/control', 'POST', { command: cmd });

      this.setData({ isFocusing });
      if (isFocusing) {
        wx.showToast({ title: '开始专注', icon: 'none' });
        // Reset local timer logic
        this.startTime = Date.now();
        this.targetDuration = this.data.durations[this.data.durationIndex] * 60 * 1000;
      } else {
        wx.showToast({ title: '专注结束', icon: 'none' });
        this.fetchData(); // Refresh stats
      }
    } catch (e) {
      wx.showToast({ title: '指令发送失败', icon: 'none' });
    }
  },

  // Local Timer UI Update (Runs every second)
  startLocalTimer() {
    if (this.data.timer) return;
    this.data.timer = setInterval(() => {
      // If focusing, calculate remaining time
      if (this.data.isFocusing) {
        const elapsed = Date.now() - this.startTime;
        let remainingMs = this.targetDuration - elapsed;

        if (remainingMs <= 0) {
          remainingMs = 0;
          this.setData({ isFocusing: false });
          this.fetchData();

          // 增强结束提醒：多次震动 + 提示音（如果系统支持）
          const vibrate = () => {
             wx.vibrateLong();
             // wx.showToast({title: '专注完成！', icon: 'none'});
          };
          vibrate();
          setTimeout(vibrate, 1000);
          setTimeout(vibrate, 2000);

          wx.showModal({
              title: '专注完成',
              content: '恭喜你完成了一次专注！休息一下吧。',
              showCancel: false
          });
        }

        this.updateTimerDisplay(Math.ceil(remainingMs / 1000));

        // Progress
        const progress = Math.min(100, (elapsed / this.targetDuration) * 100);
        this.setData({ progress });

      } else {
        // Not focusing, just show setting duration
        const duration = this.data.durations[this.data.durationIndex];
        this.updateTimerDisplay(duration * 60);
        this.setData({ progress: 0 });
      }
    }, 1000);
  },

  stopLocalTimer() {
    if (this.data.timer) {
      clearInterval(this.data.timer);
      this.data.timer = null;
    }
  },

  updateTimerDisplay(seconds) {
    const m = Math.floor(seconds / 60).toString().padStart(2, '0');
    const s = (seconds % 60).toString().padStart(2, '0');
    this.setData({ remaining: `${m}:${s}` });
  },

  goToSchedule() {
      wx.navigateTo({ url: '/pages/schedule/schedule' });
  }
});
