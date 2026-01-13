# 太空人智能时钟 - 微信小程序版本使用指南

## 📁 项目结构说明

```
clock/
├── src/                          # ESP8266 固件源码
│   ├── main.cpp                  # 主程序 (已更新)
│   ├── DataManager.cpp           # 数据管理 (已更新)
│   └── DataManager.h             # 数据管理头文件 (已更新)
├── platformio.ini                # 平台配置 (已添加QRCode库)
└── miniprogram/                  # 微信小程序源码 (新增)
    ├── app.js                    # 全局逻辑
    ├── app.json                  # 应用配置
    ├── app.wxss                  # 全局样式
    ├── project.config.json       # 项目配置
    ├── sitemap.json              
    ├── images/                   # 图标资源 (需补充)
    └── pages/                    # 页面目录
        ├── index/                # 设备列表页 (类似米家首页)
        ├── scan/                 # 扫码绑定页
        ├── device/               # 设备详情页 (天气时钟界面)
        ├── schedule/             # 日程管理页 (含日历视图)
        ├── pomodoro/             # 番茄钟页面 (三标签)
        └── stats/                # 专注统计页
```

---

## 🚀 快速开始

### 第一步：烧录固件到ESP8266

```bash
cd D:\pythonproject\clock
pio run --target upload
```

### 第二步：导入小程序到微信开发者工具

1. 打开微信开发者工具
2. 选择「导入项目」
3. 目录选择: `D:\pythonproject\clock\miniprogram`
4. AppID: 填写你的小程序AppID (或使用测试号)
5. 点击「导入」

### 第三步：添加图标资源

在 `miniprogram/images/` 目录下放置以下图标文件：

| 文件名 | 用途 | 建议尺寸 |
|--------|------|----------|
| device.png | TabBar设备图标 | 81x81 |
| device-active.png | TabBar设备图标(选中) | 81x81 |
| focus.png | TabBar专注图标 | 81x81 |
| focus-active.png | TabBar专注图标(选中) | 81x81 |
| stats.png | TabBar统计图标 | 81x81 |
| stats-active.png | TabBar统计图标(选中) | 81x81 |
| clock-icon.png | 设备列表图标 | 100x100 |
| astronaut.png | 太空人图片 | 200x200 |
| empty-device.png | 空设备提示图 | 300x300 |

> 可从 https://www.iconfont.cn 下载免费图标

---

## 📱 功能说明

### 1. 设备绑定流程

```
 ┌─────────────────┐
 │  ESP8266 开机   │
 └────────┬────────┘
          │ 检查绑定状态
          ▼
 ┌─────────────────┐
 │  未绑定?        │──是──▶ 显示二维码页面
 └────────┬────────┘
          │ 否
          ▼
 ┌─────────────────┐
 │  进入天气页面   │
 └─────────────────┘
```

**首次使用：**
1. ESP8266 首次开机自动显示二维码
2. 打开小程序 → 点击「+」 → 扫描二维码
3. 确认绑定 → 设备添加到列表

**重置设备：**
- 长按设备按钮 **10秒** → 恢复出厂设置
- 设备将清除所有数据并重新显示二维码

### 2. 小程序功能对照表

| 功能 | 小程序页面 | ESP8266 API |
|------|-----------|-------------|
| 设备列表 | pages/index | GET /api/device/info |
| 扫码绑定 | pages/scan | POST /api/device/bind |
| 解除绑定 | 设置菜单 | POST /api/device/unbind |
| 查看天气 | pages/device | GET /api/data |
| 日程管理 | pages/schedule | POST /api/schedule |
| 番茄钟 | pages/pomodoro | GET /api/data |
| 专注统计 | pages/stats | GET /api/data |

### 3. Web端访问 (备用)

设备连接WiFi后，可通过浏览器访问：
```
http://<设备IP地址>/
```

Web端已完全汉化，支持：
- 📅 日程计划管理
- 🍅 专注统计查看
- 📝 记录编辑和删除

---

## ⚙️ ESP8266 API 文档

### 设备信息
```http
GET /api/device/info
```
响应:
```json
{
  "deviceId": "SC_A1B2C3D4",
  "isBound": true,
  "boundNickname": "微信用户",
  "ip": "192.168.1.100"
}
```

### 绑定设备
```http
POST /api/device/bind
Content-Type: application/json

{
  "token": "ABC123",
  "userId": "wx_xxx",
  "nickname": "用户昵称"
}
```

### 解绑设备
```http
POST /api/device/unbind
Content-Type: application/json

{
  "userId": "wx_xxx"
}
```

### 获取数据
```http
GET /api/data
```
响应:
```json
{
  "schedule": [...],
  "pomodoro": { "today": 50, "total": 1200 },
  "focusRecords": [...]
}
```

### 添加日程
```http
POST /api/schedule
Content-Type: application/json

{
  "action": "add",
  "item": {
    "date": "2024-01-15",
    "startTime": "09:00",
    "endTime": "10:00",
    "content": "项目会议",
    "category": "work",
    "isDone": false
  }
}
```

---

## 🎨 界面预览

### 小程序界面

根据您提供的设计图，实现了以下界面：

1. **天气时钟首页**
   - 实时天气显示（温度、湿度、空气质量）
   - 太空人动画
   - 当前时间日期

2. **番茄钟页面** (三个标签)
   - 番茄钟：25/5/15分钟计时器
   - 日程提醒：今日日程列表
   - 专注统计：时间统计卡片

3. **日程管理页面**
   - 日程列表视图
   - 日历视图（可切换）
   - 添加/编辑/删除日程

4. **专注统计页面**
   - 本周/本月专注时长
   - 完成番茄钟数量
   - 任务分类占比

---

## ⚠️ 注意事项

### 网络要求
- 小程序和ESP8266必须在 **同一WiFi网络** 下
- 小程序通过局域网IP直接访问设备
- 如需远程控制，需要额外搭建云服务器

### 微信小程序审核
如果需要发布小程序：
1. 需要注册微信小程序账号
2. 类目选择：工具 > 设备辅助工具
3. 需要提供硬件设备实物照片
4. 需要开启「不校验合法域名」用于测试

### 已知限制
- ESP8266内存有限，最多存储30条日程
- 二维码绑定需要在5分钟内完成
- 专注记录最多保存最近50条

---

## 🔧 故障排除

| 问题 | 解决方案 |
|------|----------|
| 扫码后提示「连接失败」 | 检查手机和设备是否在同一WiFi |
| 二维码显示不出来 | 检查固件是否正确烧录 |
| 小程序无法添加任务 | 检查设备是否在线（设备列表显示状态） |
| 设备重启后数据丢失 | 正常现象，数据存储在LittleFS中，检查是否触发了出厂重置 |

---

## 📞 技术支持

如有问题，请检查：
1. 串口输出 (波特率: 9600)
2. 小程序控制台日志
3. ESP8266 IP地址是否正确

祝你验收顺利！🎉

