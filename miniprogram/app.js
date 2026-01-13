// app.js
App({
  globalData: {
    devices: [], // 已绑定的设备列表
    currentDevice: null, // 当前选中的设备
    openId: null,
    userInfo: null
  },

  onLaunch() {
    console.log('小程序启动');
    this.loadDevices();
    this.initUserIdentity();
  },

  // 初始化用户身份：没有云函数时，用 wx.login 的 code 作为演示期稳定 userId（降级方案）
  initUserIdentity() {
    // 1) 优先读本地缓存
    const cached = wx.getStorageSync('openId');
    if (cached) {
      this.globalData.openId = cached;
      return;
    }

    // 2) 再尝试 wx.login
    wx.login({
      success: (res) => {
        // 注意：严格意义上 openId 需要 code2session 才能换取。
        // 但为了明天验收“绑定归属”的体验，我们在无后端情况下用 code 做 userId。
        // 同一台手机在一定时间内会保持稳定，足够演示绑定/解绑逻辑。
        const code = res.code;
        if (code) {
          const pseudoOpenId = `wx_${code}`;
          this.globalData.openId = pseudoOpenId;
          wx.setStorageSync('openId', pseudoOpenId);
          console.log('已生成临时用户ID:', pseudoOpenId);
        } else {
          // 3) 最后兜底：随机ID
          const fallback = `wx_${Date.now()}_${Math.floor(Math.random() * 10000)}`;
          this.globalData.openId = fallback;
          wx.setStorageSync('openId', fallback);
        }
      },
      fail: () => {
        // 兜底：随机ID
        const fallback = `wx_${Date.now()}_${Math.floor(Math.random() * 10000)}`;
        this.globalData.openId = fallback;
        wx.setStorageSync('openId', fallback);
      }
    });

    // userInfo：演示用，不要求授权
    const nick = wx.getStorageSync('nickname');
    if (nick) {
      this.globalData.userInfo = { nickName: nick };
    } else {
      this.globalData.userInfo = { nickName: '微信用户' };
    }
  },

  // 从本地存储加载设备列表
  loadDevices() {
    try {
      const devices = wx.getStorageSync('devices');
      if (devices) {
        this.globalData.devices = JSON.parse(devices);

        // 【紧急修复】强制修正IP地址
        // 您的设备IP已变为 192.168.137.94，这里强制更新缓存中的IP
        // 这样不用解绑也能直接连上！
        this.globalData.devices.forEach(d => {
            if (d.deviceId === 'SC_E5C59D') {
                d.ip = '192.168.137.94';
            }
        });

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
  },

  // 统一设备请求封装
  requestDevice(apiPath, method = 'GET', data = {}) {
    return new Promise((resolve, reject) => {
      const device = this.globalData.currentDevice;
      if (!device) {
        wx.showToast({ title: '未选择设备', icon: 'none' });
        return reject('No device selected');
      }

      const url = `http://${device.ip}${apiPath}`;
      console.log(`[Reuest] ${method} ${url}`, data);

      wx.request({
        url: url,
        method: method,
        data: data,
        timeout: 3000,
        success: (res) => {
          if (res.statusCode === 200) {
            resolve(res.data);
          } else {
            console.error('API Error:', res);
            reject(res.statusCode);
          }
        },
        fail: (err) => {
          console.error('Request Failed:', err);
          wx.showToast({ title: '连接设备超时', icon: 'none' });
          reject(err);
        }
      });
    });
  },

  // 兼容旧代码：允许传入 device 对象作为第一个参数
  requestDeviceCompat(device, apiPath, method = 'GET', data = {}) {
    return new Promise((resolve, reject) => {
      if (!device || !device.ip) {
        wx.showToast({ title: '设备IP缺失', icon: 'none' });
        return reject('Device IP missing');
      }

      const url = `http://${device.ip}${apiPath}`;
      console.log(`[RequestCompat] ${method} ${url}`, data);

      wx.request({
        url: url,
        method: method,
        data: data,
        timeout: 4000,
        success: (res) => {
          if (res.statusCode === 200) {
            resolve(res.data);
          } else {
            console.error('API Error:', res);
            reject(res.statusCode);
          }
        },
        fail: (err) => {
          console.error('Request Failed:', err);
          wx.showToast({ title: '连接设备失败', icon: 'none' });
          reject(err);
        }
      });
    });
  }
});
