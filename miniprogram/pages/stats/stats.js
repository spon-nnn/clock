// pages/stats/stats.js - 统计页面逻辑
const app = getApp();

Page({
  data: {
    period: 'week',
    periodStats: {
      hours: 12,
      minutes: 30
    },
    totalPomodoros: 36,
    streakDays: 12,
    activeDays: 15,
    avgDaily: 45,

    categories: [
      { name: '工作', percent: 45, color: '#3b82f6' },
      { name: '学习', percent: 30, color: '#22c55e' },
      { name: '生活', percent: 25, color: '#f59e0b' }
    ],

    recentRecords: [],
    device: null
  },

  onLoad() {
    this.loadDevice();
    this.loadStats();
  },

  onShow() {
    this.loadStats();
  },

  loadDevice() {
    const device = app.globalData.currentDevice;
    this.setData({ device });
  },

  async loadStats() {
    const device = app.globalData.currentDevice;
    if (!device) return;

    try {
      const data = await app.requestDevice(device, '/api/data');

      // 处理统计数据
      const todayMinutes = data.pomodoro?.today || 0;
      const totalMinutes = data.pomodoro?.total || 0;
      const focusRecords = data.focusRecords || [];

      // 计算本周/本月数据 (简化计算)
      const weekMinutes = todayMinutes * 5; // 模拟
      const monthMinutes = totalMinutes;

      const periodStats = this.data.period === 'week'
        ? { hours: Math.floor(weekMinutes / 60), minutes: weekMinutes % 60 }
        : { hours: Math.floor(monthMinutes / 60), minutes: monthMinutes % 60 };

      // 处理最近记录
      const recentRecords = focusRecords.slice(-10).reverse().map(rec => {
        const startDate = new Date(rec.startTime * 1000);
        const endDate = new Date(rec.endTime * 1000);
        return {
          id: rec.id,
          day: startDate.getDate(),
          month: startDate.getMonth() + 1,
          startTime: this.formatTime(startDate),
          endTime: this.formatTime(endDate),
          duration: rec.duration,
          note: rec.note
        };
      });

      this.setData({
        periodStats,
        totalPomodoros: Math.floor(totalMinutes / 25),
        avgDaily: Math.floor(totalMinutes / 7),
        recentRecords,
        device: { ...device, online: true }
      });

    } catch (err) {
      console.error('Load stats failed:', err);
      if (this.data.device) {
        this.setData({ 'device.online': false });
      }
    }
  },

  formatTime(date) {
    const hours = String(date.getHours()).padStart(2, '0');
    const minutes = String(date.getMinutes()).padStart(2, '0');
    return `${hours}:${minutes}`;
  },

  setPeriod(e) {
    const period = e.currentTarget.dataset.period;
    this.setData({ period });
    this.loadStats();
  },

  clearRecords() {
    wx.showModal({
      title: '确认清空',
      content: '确定要清空所有专注记录吗？此操作不可恢复。',
      confirmColor: '#ef4444',
      success: (res) => {
        if (res.confirm) {
          // 这里可以添加清空记录的API调用
          wx.showToast({ title: '功能开发中', icon: 'none' });
        }
      }
    });
  }
});

