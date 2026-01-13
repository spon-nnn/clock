// pages/scan/scan.js - 扫码绑定逻辑
const app = getApp();

Page({
  data: {
    scanResult: null,
    bindSuccess: false,
    binding: false,
    manualIp: ''
  },

  onLoad() {
    // 自动开始扫码
    // this.startScan();
  },

  // 开始扫码
  startScan() {
    wx.scanCode({
      onlyFromCamera: false,
      scanType: ['qrCode'],
      success: (res) => {
        console.log('Scan result:', res.result);
        this.handleScanResult(res.result);
      },
      fail: (err) => {
        console.error('Scan failed:', err);
        if (err.errMsg.indexOf('cancel') === -1) {
          wx.showToast({
            title: '扫码失败',
            icon: 'error'
          });
        }
      }
    });
  },

  // 处理扫码结果
  handleScanResult(result) {
    try {
      const data = JSON.parse(result);

      // 验证是否是太空人时钟的二维码
      if (data.type !== 'spaceclock') {
        wx.showModal({
          title: '无效二维码',
          content: '这不是太空人时钟的绑定二维码，请确认扫描正确的二维码。',
          showCancel: false
        });
        return;
      }

      this.setData({
        scanResult: data,
        bindSuccess: false
      });

      // 验证设备是否可连接
      this.verifyDevice(data);

    } catch (e) {
      console.error('Parse error:', e);
      wx.showModal({
        title: '无效二维码',
        content: '无法解析二维码内容，请确认扫描正确的二维码。',
        showCancel: false
      });
    }
  },

  // 验证设备连接
  async verifyDevice(data) {
    wx.showLoading({ title: '验证设备...' });

    try {
      const device = { ip: data.ip, deviceId: data.did };
      const res = await app.requestDevice(device, '/api/device/info');

      console.log('Device info:', res);

      if (res.isBound) {
        wx.hideLoading();
        wx.showModal({
          title: '设备已绑定',
          content: `此设备已被 ${res.boundNickname || '其他用户'} 绑定，请先在设备上恢复出厂设置后重试。`,
          showCancel: false
        });
        this.setData({ scanResult: null });
        return;
      }

      wx.hideLoading();
      wx.showToast({ title: '设备可用', icon: 'success' });

    } catch (err) {
      wx.hideLoading();
      wx.showModal({
        title: '连接失败',
        content: '无法连接到设备，请确保：\n1. 手机和设备在同一WiFi网络\n2. 设备已开机并显示二维码',
        showCancel: false
      });
      this.setData({ scanResult: null });
    }
  },

  // 绑定设备
  async bindDevice() {
    if (this.data.binding) return;

    this.setData({ binding: true });

    const { scanResult } = this.data;
    const userInfo = app.globalData.userInfo;
    const openId = app.globalData.openId;

    try {
      const device = { ip: scanResult.ip, deviceId: scanResult.did };

      const res = await app.requestDevice(device, '/api/device/bind', 'POST', {
        token: scanResult.token,
        userId: openId,
        nickname: userInfo?.nickName || '微信用户'
      });

      if (res.success) {
        // 绑定成功，保存设备到本地
        const newDevice = {
          deviceId: scanResult.did,
          ip: scanResult.ip,
          name: '太空人时钟',
          online: true,
          bindTime: Date.now()
        };

        app.addDevice(newDevice);

        this.setData({
          bindSuccess: true,
          binding: false
        });

        wx.showToast({ title: '绑定成功', icon: 'success' });

      } else {
        throw new Error(res.error || '绑定失败');
      }

    } catch (err) {
      console.error('Bind error:', err);
      this.setData({ binding: false });
      wx.showModal({
        title: '绑定失败',
        content: err.message || '请重试',
        showCancel: false
      });
    }
  },

  // 重新扫描
  rescan() {
    this.setData({
      scanResult: null,
      bindSuccess: false
    });
    this.startScan();
  },

  // 手动输入IP
  onIpInput(e) {
    this.setData({ manualIp: e.detail.value });
  },

  // 手动连接
  async manualConnect() {
    const ip = this.data.manualIp.trim();
    if (!ip) {
      wx.showToast({ title: '请输入IP地址', icon: 'none' });
      return;
    }

    // 验证IP格式
    const ipRegex = /^(\d{1,3}\.){3}\d{1,3}$/;
    if (!ipRegex.test(ip)) {
      wx.showToast({ title: 'IP格式不正确', icon: 'none' });
      return;
    }

    wx.showLoading({ title: '连接中...' });

    try {
      const device = { ip: ip };
      const res = await app.requestDevice(device, '/api/device/info');

      wx.hideLoading();

      // 构造类似扫码的结果
      const scanResult = {
        type: 'spaceclock',
        did: res.deviceId,
        ip: ip,
        token: '' // 手动连接需要设备显示token
      };

      if (res.isBound) {
        // 设备已绑定，检查是否是当前用户
        const existingDevice = app.globalData.devices.find(d => d.deviceId === res.deviceId);
        if (existingDevice) {
          // 更新IP
          existingDevice.ip = ip;
          app.saveDevices();
          wx.showToast({ title: '设备已更新', icon: 'success' });
          setTimeout(() => wx.navigateBack(), 1500);
          return;
        }

        wx.showModal({
          title: '设备已绑定',
          content: `此设备已被 ${res.boundNickname} 绑定`,
          showCancel: false
        });
      } else {
        wx.showModal({
          title: '需要扫码绑定',
          content: '请扫描设备屏幕上的二维码完成绑定',
          showCancel: false
        });
      }

    } catch (err) {
      wx.hideLoading();
      wx.showModal({
        title: '连接失败',
        content: '无法连接到该IP地址，请检查网络',
        showCancel: false
      });
    }
  },

  // 进入设备
  goToDevice() {
    wx.navigateBack();
    setTimeout(() => {
      wx.navigateTo({
        url: '/pages/device/device'
      });
    }, 300);
  },

  // 返回列表
  goBack() {
    wx.navigateBack();
  }
});

