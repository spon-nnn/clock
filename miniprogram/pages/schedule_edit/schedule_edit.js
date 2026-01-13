const app = getApp();

Page({
  data: {
    mode: 'add',
    submitting: false,
    originalItem: null,
    formData: {
      date: '',
      startTime: '09:00',
      endTime: '10:00',
      content: '',
      category: 'work'
    }
  },

  ensureCurrentDevice() {
    const cur = app.globalData.currentDevice;
    if (cur && cur.ip) return cur;

    // 兜底：从已绑定设备列表里找一个有 ip 的
    const devices = app.getDevices ? app.getDevices() : (app.globalData.devices || []);
    const fallback = devices.find(d => d && d.ip);
    if (fallback) {
      app.setCurrentDevice(fallback);
      return fallback;
    }

    return null;
  },

  onLoad(query) {
    // 确保有当前设备
    const dev = this.ensureCurrentDevice();
    if (!dev) {
      wx.showModal({
        title: '未选择设备',
        content: '当前没有可用设备IP，请回到首页选择设备后重试。',
        showCancel: false,
        success: () => wx.navigateBack()
      });
      return;
    }

    const now = new Date();
    const today = `${now.getFullYear()}-${String(now.getMonth()+1).padStart(2,'0')}-${String(now.getDate()).padStart(2,'0')}`;

    const mode = query.mode || 'add';
    if (mode === 'edit' && query.item) {
      const item = JSON.parse(decodeURIComponent(query.item));
      this.setData({
        mode: 'edit',
        originalItem: item,
        formData: {
          date: item.date,
          startTime: item.startTime,
          endTime: item.endTime,
          content: item.content,
          category: item.category || 'work'
        }
      });
    } else {
      const date = query.date ? decodeURIComponent(query.date) : today;
      this.setData({
        mode: 'add',
        formData: { ...this.data.formData, date }
      });
    }
  },

  onDateChange(e) {
    this.setData({ 'formData.date': e.detail.value });
  },
  onStartTimeChange(e) {
    this.setData({ 'formData.startTime': e.detail.value });
  },
  onEndTimeChange(e) {
    this.setData({ 'formData.endTime': e.detail.value });
  },
  onContentInput(e) {
    this.setData({ 'formData.content': e.detail.value });
  },
  selectCategory(e) {
    this.setData({ 'formData.category': e.currentTarget.dataset.category });
  },

  async submit() {
    const device = this.ensureCurrentDevice();
    if (!device) {
      wx.showToast({ title: '设备IP缺失', icon: 'none' });
      return;
    }

    const { formData, originalItem, mode } = this.data;
    if (!formData.content.trim()) {
      wx.showToast({ title: '请输入内容', icon: 'none' });
      return;
    }

    this.setData({ submitting: true });

    try {
      // 编辑：先删后加（与原逻辑保持一致）
      if (mode === 'edit' && originalItem && originalItem.id) {
        await app.requestDeviceCompat(device, '/api/schedule', 'POST', {
          action: 'del',
          id: originalItem.id
        });
      }

      await app.requestDeviceCompat(device, '/api/schedule', 'POST', {
        action: 'add',
        item: {
          date: formData.date,
          startTime: formData.startTime,
          endTime: formData.endTime,
          content: formData.content,
          category: formData.category,
          isDone: false
        }
      });

      wx.showToast({ title: '保存成功', icon: 'success' });
      setTimeout(() => wx.navigateBack(), 300);
    } catch (e) {
      console.error(e);
      wx.showToast({ title: '保存失败', icon: 'error' });
      this.setData({ submitting: false });
    }
  },

  cancel() {
    wx.navigateBack();
  }
});
