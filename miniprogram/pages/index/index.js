// pages/index/index.js
const app = getApp();

Page({
  data: {
    devices: [],
    showMenu: false,
    selectedDeviceId: null
  },

  onShow() {
    this.setData({ devices: app.getDevices() });
  },

  selectDevice(e) {
    const id = e.currentTarget.dataset.id;
    const device = this.data.devices.find(d => d.deviceId === id);
    if (device) {
      app.setCurrentDevice(device);
      // 检查在线状态 (可选：先ping一下)
      wx.navigateTo({ url: '/pages/device/device' });
    }
  },

  showActionSheet(e) {
    this.setData({
      showMenu: true,
      selectedDeviceId: e.currentTarget.dataset.id
    });
  },

  hideMenu() {
    this.setData({ showMenu: false });
  },

  goToScan() {
    wx.navigateTo({ url: '/pages/scan/scan' });
  },

  unbindDevice() {
    if (this.data.selectedDeviceId) {
      wx.showModal({
        title: '确认解绑',
        content: '确定要解除绑定此设备吗？',
        success: (res) => {
          if (res.confirm) {
            app.removeDevice(this.data.selectedDeviceId);
            this.setData({ devices: app.getDevices(), showMenu: false });
            wx.showToast({ title: '已解绑', icon: 'success' });
          }
        }
      });
    }
  },

  renameDevice() {
      wx.showToast({ title: '敬请期待', icon: 'none' });
      this.hideMenu();
  },

  refreshDevice() {
      wx.showToast({ title: '已刷新', icon: 'success' });
      this.hideMenu();
  }
});
