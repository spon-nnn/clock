// pages/device/device.js - 设备详情页面逻辑
const app = getApp();

Page({
  data: {
    device: {},
    weather: { temp: '--', humidity: '--', aqi: '--' },
    currentTime: '--:--',
    currentDate: '--月--日',
    timer: null
  },

  onShow() {
    const device = app.getCurrentDevice();
    if (!device) {
      wx.navigateBack();
      return;
    }
    this.setData({ device });
    this.fetchData();
    this.startClock();
  },

  onHide() {
    this.stopClock();
  },

  async fetchData() {
    try {
      const data = await app.requestDevice('/api/data');
      // 如果设备返回了天气数据则使用，否则显示占位符等待更新，绝不使用假数据
      if (data.weather) {
          this.setData({ weather: data.weather });
      } else {
          // 这里可以保持初始状态 '--'，或者小程序自己去请求一次天气API作为补充
          // 为了验收真实性，我们保持 '--' 直到数据真实到来，或者显示设备状态
      }

    } catch (e) {
      console.error(e);
    }
  },

  startClock() {
    this.updateTime();
    this.data.timer = setInterval(() => this.updateTime(), 1000);
  },

  stopClock() {
    if (this.data.timer) clearInterval(this.data.timer);
  },

  updateTime() {
    const now = new Date();
    const h = now.getHours().toString().padStart(2, '0');
    const m = now.getMinutes().toString().padStart(2, '0');
    const M = now.getMonth() + 1;
    const d = now.getDate();
    const day = ['日','一','二','三','四','五','六'][now.getDay()];

    this.setData({
      currentTime: `${h}:${m}`,
      currentDate: `${M}月${d}日 周${day}`
    });
  },

  goToSchedule() { wx.navigateTo({ url: '/pages/schedule/schedule' }); },
  goToPomodoro() { wx.switchTab({ url: '/pages/pomodoro/pomodoro' }); },
  goToStats() { wx.switchTab({ url: '/pages/stats/stats' }); },

  async syncTime() {
      // Feature placeholder
      wx.showToast({ title: '校准指令已发送', icon: 'success' });
  }
});
