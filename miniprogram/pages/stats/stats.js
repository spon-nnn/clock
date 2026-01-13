// pages/stats/stats.js - 统计页面逻辑
const app = getApp();

Page({
  data: {
    period: 'week',
    periodStats: {
      hours: 0,
      minutes: 0
    },
    totalPomodoros: 0,
    streakDays: 0,
    activeDays: 0,
    avgDaily: 0,

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
    // 使用兼容方法
    const device = app.globalData.currentDevice;
    if (!device) return;

    try {
      const data = await app.requestDeviceCompat(device, '/api/data');

      // 处理统计数据
      const totalMinutes = data.pomodoro?.total || 0;
      const focusRecords = data.focusRecords || [];

      // 基于真实记录计算
      const now = new Date();
      const oneDay = 24 * 60 * 60 * 1000;

      let weekMins = 0;
      let monthMins = 0;
      let activeDates = new Set();

      // 遍历所有记录进行统计
      focusRecords.forEach(rec => {
          const recDate = new Date(rec.startTime * 1000);
          const diffTime = now - recDate; // 毫秒差
          const diffDays = diffTime / oneDay;

          if (diffDays <= 7 && diffDays >= 0) weekMins += rec.duration;
          if (diffDays <= 30 && diffDays >= 0) monthMins += rec.duration;

          const dateStr = `${recDate.getFullYear()}-${recDate.getMonth()+1}-${recDate.getDate()}`;
          activeDates.add(dateStr);
      });

      // 连续天数计算
      let streak = 0;
      for (let i = 0; i < 60; i++) { // 此时回溯60天
          const d = new Date(now.getTime() - i * oneDay);
          const dateStr = `${d.getFullYear()}-${d.getMonth()+1}-${d.getDate()}`;

          if (activeDates.has(dateStr)) {
              streak++;
          } else {
              // 特殊处理：如果是今天且今天也没记录，不视为中断，继续检查昨天
              // 如果是昨天及更早没记录，则中断
              if (i === 0) continue;
              break;
          }
      }

      const periodStats = this.data.period === 'week'
        ? { hours: Math.floor(weekMins / 60), minutes: weekMins % 60 }
        : { hours: Math.floor(monthMins / 60), minutes: monthMins % 60 };

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
        totalPomodoros: focusRecords.length,
        streakDays: streak,
        activeDays: activeDates.size,
        avgDaily: activeDates.size > 0 ? Math.floor(totalMinutes / activeDates.size) : 0,
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
          wx.showToast({ title: '功能开发中', icon: 'none' });
        }
      }
    });
  }
});
