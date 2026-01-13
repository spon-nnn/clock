// pages/index/index.js - 设备列表页面逻辑
const app = getApp();

Page({
  data: {
    devices: [],
    currentDevice: null,
    weatherData: null,
    todaySchedules: [],
    focusStats: {
      today: 0,
      sessions: 0
    },
    showMenu: false,
    menuDevice: null,
    categoryNames: {
      'work': '工作',
      'life': '生活',
      'study': '学习'
    }
  },

  onLoad() {
    this.loadDevices();
  },

  onShow() {
    this.loadDevices();
    if (this.data.currentDevice) {
      this.refreshDeviceData();
    }
  },

  // 加载设备列表
  loadDevices() {
    const devices = app.globalData.devices || [];
    const currentDevice = app.globalData.currentDevice;

    this.setData({
      devices: devices,
      currentDevice: currentDevice
    });

    // 检查每个设备的在线状态
    devices.forEach((device, index) => {
      this.checkDeviceOnline(device, index);
    });
  },

  // 检查设备在线状态
  async checkDeviceOnline(device, index) {
    try {
      const res = await app.requestDevice(device, '/api/device/info');
      const devices = this.data.devices;
      devices[index].online = true;
      this.setData({ devices });
    } catch (err) {
      const devices = this.data.devices;
      devices[index].online = false;
      this.setData({ devices });
    }
  },

  // 刷新当前设备数据
  async refreshDeviceData() {
    const device = this.data.currentDevice;
    if (!device) return;

    wx.showLoading({ title: '加载中...' });

    try {
      const data = await app.requestDevice(device, '/api/data');

      // 处理日程数据 - 只显示今天的
      const today = this.getToday();
      const todaySchedules = (data.schedule || []).filter(s => s.date === today);

      // 处理专注统计
      const focusStats = {
        today: data.pomodoro?.today || 0,
        sessions: Math.floor((data.pomodoro?.today || 0) / 25)
      };

      this.setData({
        todaySchedules,
        focusStats
      });

    } catch (err) {
      console.error('Failed to refresh device data:', err);
    } finally {
      wx.hideLoading();
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

  // 选择设备
  selectDevice(e) {
    const device = e.currentTarget.dataset.device;
    app.setCurrentDevice(device);
    this.setData({ currentDevice: device });
    this.refreshDeviceData();
  },

  // 进入设备详情
  goToDevice(e) {
    const device = e.currentTarget.dataset.device;
    app.setCurrentDevice(device);
    wx.navigateTo({
      url: '/pages/device/device'
    });
  },

  // 跳转到扫码页面
  goToScan() {
    wx.navigateTo({
      url: '/pages/scan/scan'
    });
  },

  // 跳转到日程页面
  goToSchedule() {
    wx.navigateTo({
      url: '/pages/schedule/schedule'
    });
  },

  // 跳转到统计页面
  goToStats() {
    wx.switchTab({
      url: '/pages/stats/stats'
    });
  },

  // 显示设备菜单
  showDeviceMenu(e) {
    const device = e.currentTarget.dataset.device;
    this.setData({
      showMenu: true,
      menuDevice: device
    });
  },

  // 隐藏菜单
  hideMenu() {
    this.setData({ showMenu: false });
  },

  // 重命名设备
  renameDevice() {
    const device = this.data.menuDevice;
    wx.showModal({
      title: '重命名设备',
      editable: true,
      placeholderText: '请输入设备名称',
      success: (res) => {
        if (res.confirm && res.content) {
          // 更新设备名称
          const devices = this.data.devices;
          const index = devices.findIndex(d => d.deviceId === device.deviceId);
          if (index >= 0) {
            devices[index].name = res.content;
            app.globalData.devices = devices;
            app.saveDevices();
            this.setData({ devices, showMenu: false });
          }
        }
      }
    });
  },

  // 刷新设备
  refreshDevice() {
    const device = this.data.menuDevice;
    const index = this.data.devices.findIndex(d => d.deviceId === device.deviceId);
    this.checkDeviceOnline(device, index);
    this.setData({ showMenu: false });
    wx.showToast({ title: '已刷新', icon: 'success' });
  },

  // 解绑设备
  unbindDevice() {
    const device = this.data.menuDevice;

    wx.showModal({
      title: '确认解绑',
      content: `确定要解除绑定 ${device.name || device.deviceId} 吗？`,
      confirmColor: '#ef4444',
      success: async (res) => {
        if (res.confirm) {
          wx.showLoading({ title: '解绑中...' });

          try {
            // 向设备发送解绑请求
            await app.requestDevice(device, '/api/device/unbind', 'POST', {
              userId: app.globalData.openId
            });
          } catch (err) {
            console.log('Device may be offline');
          }

          // 从本地移除
          app.removeDevice(device.deviceId);
          this.loadDevices();

          wx.hideLoading();
          wx.showToast({ title: '已解绑', icon: 'success' });
          this.setData({ showMenu: false });
        }
      }
    });
  }
});

