// pages/device/device.js - 设备详情页面逻辑
const app = getApp();

Page({
  data: {
    device: null,
    weather: {},
    scheduleCount: 0,
    focusMinutes: 0,
    currentTime: '--:--',
    currentDate: '----年--月--日 星期--'
  },

  timer: null,

  onLoad() {
    this.loadDevice();
    this.startTimeUpdate();
  },

  onShow() {
    this.refreshData();
  },

  onUnload() {
    if (this.timer) {
      clearInterval(this.timer);
    }
  },

  // 加载设备信息
  loadDevice() {
    const device = app.globalData.currentDevice;
    if (!device) {
      wx.showToast({ title: '设备未选择', icon: 'error' });
      setTimeout(() => wx.navigateBack(), 1500);
      return;
    }
    this.setData({ device });
  },

  // 开始时间更新
  startTimeUpdate() {
    this.updateTime();
    this.timer = setInterval(() => {
      this.updateTime();
    }, 1000);
  },

  // 更新时间显示
  updateTime() {
    const now = new Date();
    const hours = String(now.getHours()).padStart(2, '0');
    const minutes = String(now.getMinutes()).padStart(2, '0');

    const year = now.getFullYear();
    const month = now.getMonth() + 1;
    const day = now.getDate();
    const weekDays = ['日', '一', '二', '三', '四', '五', '六'];
    const weekDay = weekDays[now.getDay()];

    this.setData({
      currentTime: `${hours}:${minutes}`,
      currentDate: `${year}年${month}月${day}日 星期${weekDay}`
    });
  },

  // 刷新设备数据
  async refreshData() {
    const device = this.data.device;
    if (!device) return;

    wx.showLoading({ title: '刷新中...', mask: false });

    try {
      // 检查设备在线状态
      const info = await app.requestDevice(device, '/api/device/info');
      device.online = true;

      // 获取设备数据
      const data = await app.requestDevice(device, '/api/data');

      // 处理日程数量
      const today = this.getToday();
      const scheduleCount = (data.schedule || []).filter(s => s.date >= today).length;

      // 处理专注统计
      const focusMinutes = data.pomodoro?.today || 0;

      this.setData({
        device,
        scheduleCount,
        focusMinutes,
        weather: {
          city: '杭州市', // 从设备获取
          temp: '24',
          weather: '多云',
          humidity: '45',
          wind: '3级',
          aqi: '优',
          updateTime: this.formatTime(new Date())
        }
      });

      wx.hideLoading();

    } catch (err) {
      console.error('Refresh failed:', err);
      device.online = false;
      this.setData({ device });
      wx.hideLoading();
      wx.showToast({ title: '设备离线', icon: 'error' });
    }
  },

  // 获取今天日期
  getToday() {
    const d = new Date();
    const year = d.getFullYear();
    const month = String(d.getMonth() + 1).padStart(2, '0');
    const day = String(d.getDate()).padStart(2, '0');
    return `${year}-${month}-${day}`;
  },

  // 格式化时间
  formatTime(date) {
    return `${String(date.getHours()).padStart(2, '0')}:${String(date.getMinutes()).padStart(2, '0')}`;
  },

  // 跳转日程页面
  goToSchedule() {
    wx.navigateTo({
      url: '/pages/schedule/schedule'
    });
  },

  // 跳转番茄钟页面
  goToPomodoro() {
    wx.switchTab({
      url: '/pages/pomodoro/pomodoro'
    });
  },

  // 跳转统计页面
  goToStats() {
    wx.switchTab({
      url: '/pages/stats/stats'
    });
  },

  // 设置城市
  setCityCode() {
    wx.showModal({
      title: '设置天气城市',
      editable: true,
      placeholderText: '请输入城市名称',
      success: (res) => {
        if (res.confirm && res.content) {
          wx.showToast({ title: '功能开发中', icon: 'none' });
        }
      }
    });
  },

  // 显示解绑确认
  showUnbindConfirm() {
    const device = this.data.device;
    wx.showModal({
      title: '确认解绑',
      content: `确定要解除绑定 ${device.name || device.deviceId} 吗？解绑后设备将恢复到待绑定状态。`,
      confirmColor: '#ef4444',
      success: async (res) => {
        if (res.confirm) {
          await this.unbindDevice();
        }
      }
    });
  },

  // 解绑设备
  async unbindDevice() {
    const device = this.data.device;
    wx.showLoading({ title: '解绑中...' });

    try {
      await app.requestDevice(device, '/api/device/unbind', 'POST', {
        userId: app.globalData.openId
      });
    } catch (err) {
      console.log('Device may be offline');
    }

    app.removeDevice(device.deviceId);
    wx.hideLoading();
    wx.showToast({ title: '已解绑', icon: 'success' });

    setTimeout(() => {
      wx.navigateBack();
    }, 1500);
  }
});

