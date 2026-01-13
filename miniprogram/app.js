// app.js
App({
  globalData: {
    devices: [], // 已绑定的设备列表
    currentDevice: null // 当前选中的设备
  },

  onLaunch() {
    console.log('小程序启动');
    this.loadDevices();
  },

  // 从本地存储加载设备列表
  loadDevices() {
    try {
      const devices = wx.getStorageSync('devices');
      if (devices) {
        this.globalData.devices = JSON.parse(devices);
        console.log('已加载设备列表:', this.globalData.devices);
      }
    } catch (e) {
      console.error('加载设备列表失败:', e);
      this.globalData.devices = [];
    }
  },

  // 保存设备列表到本地存储
  saveDevices() {
    try {
      wx.setStorageSync('devices', JSON.stringify(this.globalData.devices));
      console.log('设备列表已保存');
    } catch (e) {
      console.error('保存设备列表失败:', e);
    }
  },

  // 添加设备
  addDevice(device) {
    // 检查是否已存在
    const index = this.globalData.devices.findIndex(d => d.deviceId === device.deviceId);
    if (index === -1) {
      this.globalData.devices.push(device);
    } else {
      // 更新已存在的设备
      this.globalData.devices[index] = device;
    }
    this.saveDevices();
  },

  // 删除设备
  removeDevice(deviceId) {
    this.globalData.devices = this.globalData.devices.filter(d => d.deviceId !== deviceId);
    this.saveDevices();
  },

  // 获取设备列表
  getDevices() {
    return this.globalData.devices;
  },

  // 设置当前设备
  setCurrentDevice(device) {
    this.globalData.currentDevice = device;
  },

  // 获取当前设备
  getCurrentDevice() {
    return this.globalData.currentDevice;
  }
});

