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

    // 分类占比：改为动态计算
    categories: [
      { name: '工作', percent: 0, color: '#3b82f6' },
      { name: '学习', percent: 0, color: '#22c55e' },
      { name: '生活', percent: 0, color: '#f59e0b' }
    ],

    recentRecords: [],
    device: null
  },

  onLoad() {
    this.loadDevice();
    this.loadStats();
  },

  onShow() {
    this.loadDevice();
    this.loadStats();
  },

  loadDevice() {
    const device = app.globalData.currentDevice;
    this.setData({ device });
  },

  calcCategoryStats(focusRecords, schedules) {
    const scheduleById = new Map();
    (schedules || []).forEach(s => {
      if (s && s.id) scheduleById.set(s.id, s);
    });

    const minutesByCat = { work: 0, study: 0, life: 0, other: 0 };

    (focusRecords || []).forEach(rec => {
      const mins = Number(rec.duration || 0);
      if (!mins) return;

      // 记录有 taskId 就按日程分类，否则归为 other
      const taskId = rec.taskId;
      const sch = taskId ? scheduleById.get(taskId) : null;
      const cat = sch?.category || 'other';

      if (cat === 'work' || cat === 'study' || cat === 'life') {
        minutesByCat[cat] += mins;
      } else {
        minutesByCat.other += mins;
      }
    });

    const total = minutesByCat.work + minutesByCat.study + minutesByCat.life + minutesByCat.other;
    const pct = (v) => total > 0 ? Math.round((v / total) * 100) : 0;

    return {
      categories: [
        { name: '工作', percent: pct(minutesByCat.work), color: '#3b82f6' },
        { name: '学习', percent: pct(minutesByCat.study), color: '#22c55e' },
        { name: '生活', percent: pct(minutesByCat.life), color: '#f59e0b' }
      ],
      minutesByCat
    };
  },

  async loadStats() {
    const device = app.globalData.currentDevice;
    if (!device || !device.ip) return;

    try {
      const data = await app.requestDeviceCompat(device, '/api/data');

      // 数据源（真实）：pomodoro 是设备按记录汇总的今天/累计
      const totalMinutes = Number(data.pomodoro?.total || 0);
      const focusRecords = data.focusRecords || [];
      const schedules = data.schedule || [];

      // === 周/月统计：基于 focusRecords 真实累加（rec.duration 单位是“分钟”） ===
      const now = new Date();
      const oneDayMs = 24 * 60 * 60 * 1000;

      let weekMins = 0;
      let monthMins = 0;
      let activeDates = new Set();

      focusRecords.forEach(rec => {
        const startSec = Number(rec.startTime || 0);
        const mins = Number(rec.duration || 0);
        if (!startSec || !mins) return;

        const recDate = new Date(startSec * 1000);
        const diffDays = (now - recDate) / oneDayMs;

        if (diffDays <= 7 && diffDays >= 0) weekMins += mins;
        if (diffDays <= 30 && diffDays >= 0) monthMins += mins;

        const dateStr = `${recDate.getFullYear()}-${recDate.getMonth() + 1}-${recDate.getDate()}`;
        activeDates.add(dateStr);
      });

      // 连续天数（按“有记录的日期”回溯）
      let streak = 0;
      for (let i = 0; i < 60; i++) {
        const d = new Date(now.getTime() - i * oneDayMs);
        const dateStr = `${d.getFullYear()}-${d.getMonth() + 1}-${d.getDate()}`;
        if (activeDates.has(dateStr)) {
          streak++;
        } else {
          if (i === 0) continue;
          break;
        }
      }

      const periodMins = this.data.period === 'week' ? weekMins : monthMins;
      const periodStats = { hours: Math.floor(periodMins / 60), minutes: periodMins % 60 };

      // 最近记录
      const recentRecords = focusRecords
        .slice(-10)
        .reverse()
        .map(rec => {
          const startDate = new Date((rec.startTime || 0) * 1000);
          const endDate = new Date((rec.endTime || 0) * 1000);
          return {
            id: rec.id,
            day: startDate.getDate(),
            month: startDate.getMonth() + 1,
            startTime: this.formatTime(startDate),
            endTime: (rec.endTime ? this.formatTime(endDate) : '--:--'),
            duration: rec.duration,
            note: rec.note
          };
        });

      // 分类占比（真实计算，不再硬编码）
      const { categories } = this.calcCategoryStats(focusRecords, schedules);

      this.setData({
        periodStats,
        totalPomodoros: focusRecords.length,
        streakDays: streak,
        activeDays: activeDates.size,
        avgDaily: activeDates.size > 0 ? Math.floor(totalMinutes / activeDates.size) : 0,
        recentRecords,
        categories,
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
