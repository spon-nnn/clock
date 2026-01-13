#include <ArduinoJson.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <TJpg_Decoder.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <OneButton.h>
#include <vector>
#include <qrcode.h>
#include "DataManager.h" // 引入新的数据管理器

// 用户配置
const char ssid[] = "spxzy666";  // WIFI名称
const char pass[] = "88888888";  // WIFI密码
String cityCode = "101210101";   // 杭州天气城市代码

// 字体和图片资源
#include "font/ZdyLwFont_20.h"
#include "font/FxLED_32.h"

extern TFT_eSPI tft; // 声明 tft 对象


#include "img/pangzi/i0.h"
#include "img/pangzi/i1.h"
#include "img/pangzi/i2.h"
#include "img/pangzi/i3.h"
#include "img/pangzi/i4.h"
#include "img/pangzi/i5.h"
#include "img/pangzi/i6.h"
#include "img/pangzi/i7.h"
#include "img/pangzi/i8.h"
#include "img/pangzi/i9.h"

#include "img/temperature.h"
#include "img/humidity.h"
#include "img/watch_top.h"
#include "img/watch_bottom.h"

// 对象初始化
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite clk = TFT_eSprite(&tft);
TFT_eSprite clkb = TFT_eSprite(&tft);
AsyncWebServer server(80);
OneButton btn(5, true); // D1 (GPIO 5)

// 全局变量
uint32_t targetTime = 0;   
uint16_t bgColor = 0xFFFF;
static const char ntpServerName[] = "ntp6.aliyun.com";
const int timeZone = 8;
WiFiUDP Udp;
unsigned int localPort = 8000;
byte packetBuffer[48]; 

// 页面状态管理
enum Page { PAGE_BINDQR, PAGE_WEATHER, PAGE_SCHEDULE, PAGE_POMODORO };
Page currentPage = PAGE_WEATHER;
bool pageChanged = true;

// 长按重置检测
unsigned long buttonPressStart = 0;
bool isLongPressing = false;
const unsigned long FACTORY_RESET_HOLD = 10000; // 长按10秒恢复出厂

// 日程数据 (已由 DataManager 接管)
// std::vector<String> scheduleList;

// 番茄钟数据 (部分保留用于运行时状态，持久化数据由 DataManager 接管)
bool isFocusing = false;
unsigned long focusStartTime = 0;
const unsigned long FOCUS_DURATION = 25 * 60 * 1000; // 25分钟
// unsigned long todayFocusMinutes = 0;
// unsigned long totalFocusMinutes = 0;

// 天气数据
String scrollText[6];
time_t prevDisplay = 0; 
unsigned long weaterTime = 0;

// 函数声明
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);
void getCityCode();
void getCityWeater();
void weaterData(String *cityDZ, String *dataSK, String *dataFC);
void scrollBanner();
void scrollTxt(int pos);
void imgAnim();
void onBtnDuringLongPress(); // 长按检测
String week();
String monthDay();
String hourMinute();
String num2str(int digits);
int second();
int minute();
int hour();
int weekday();
int month();
int day();

// JPEG解码回调
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if ( y >= tft.height() ) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

// ------------------- 文件系统操作 (已废弃，由 DataManager 接管) -------------------
// void loadData() { ... }
// void saveSchedule() { ... }
// void savePomodoro() { ... }

// ------------------- Web服务器 -------------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>太空人时钟控制台</title>
<style>
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;margin:0;padding:20px;background:#f0f2f5;color:#333}
.card{background:#fff;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);margin-bottom:20px}
h2,h3{margin-top:0}
input,button,select{padding:10px;margin:5px 0;width:100%;box-sizing:border-box;border:1px solid #ddd;border-radius:4px}
button{background:#1890ff;color:#fff;border:none;cursor:pointer;font-weight:bold}
button:hover{background:#40a9ff}
button.del{background:#ff4d4f;width:auto;padding:6px 12px;font-size:12px}
button.del:hover{background:#ff7875}
.row{display:flex;gap:10px}
table{width:100%;border-collapse:collapse;margin-top:10px}
th,td{padding:12px 8px;text-align:left;border-bottom:1px solid #eee}
th{background-color:#fafafa;font-weight:600}
tr:hover{background-color:#f5f5f5}
.tag{padding:2px 8px;border-radius:10px;font-size:12px;color:#fff}
.tag.work{background:#1890ff}
.tag.life{background:#52c41a}
.tag.study{background:#faad14}
.modal{display:none;position:fixed;z-index:99;left:0;top:0;width:100%;height:100%;overflow:auto;background-color:rgba(0,0,0,0.4)}
.modal-content{background-color:#fefefe;margin:10% auto;padding:25px;border:1px solid #888;width:90%;max-width:500px;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,0.15)}
.close{color:#aaa;float:right;font-size:28px;font-weight:bold;cursor:pointer}
.close:hover{color:#000}
.form-group{margin-bottom:15px}
.form-group label{display:block;margin-bottom:5px;font-weight:500}
</style></head><body>

<h1>🚀 太空人时钟控制台</h1>

<div class="card">
  <div style="display:flex;justify-content:space-between;align-items:center">
    <h2>📅 日程计划</h2>
    <div>
        <button onclick="delTodo('all')" style="width:auto;padding:8px 16px;background:#ff4d4f;margin-right:10px">清空全部</button>
        <button onclick="openModal()" style="width:auto;padding:8px 16px">+ 新建任务</button>
    </div>
  </div>
  <div style="overflow-x:auto">
    <table id="scheduleTable">
      <thead><tr><th>日期</th><th>时间</th><th>内容</th><th>分类</th><th>操作</th></tr></thead>
      <tbody id="scheduleBody"></tbody>
    </table>
  </div>
</div>

<div class="card">
  <h2>🍅 专注统计</h2>
  <div style="display:flex;justify-content:space-around;text-align:center;margin-bottom:20px">
    <div>
        <div style="font-size:24px;font-weight:bold;color:#1890ff" id="todayFocus">0</div>
        <div style="color:#666">今日专注(分钟)</div>
    </div>
    <div>
        <div style="font-size:24px;font-weight:bold;color:#52c41a" id="totalFocus">0</div>
        <div style="color:#666">累计专注(分钟)</div>
    </div>
  </div>
  <h3>📝 最近记录</h3>
  <ul id="focusList" style="list-style:none;padding:0"></ul>
</div>

<!-- 添加任务弹窗 -->
<div id="taskModal" class="modal">
  <div class="modal-content">
    <span class="close" onclick="closeModal()">&times;</span>
    <h3>添加新任务</h3>
    <div class="form-group">
        <label>日期</label>
        <input type="date" id="mDate">
    </div>
    <div class="row">
        <div class="form-group" style="flex:1">
            <label>开始时间</label>
            <input type="time" id="mStart" value="09:00">
        </div>
        <div class="form-group" style="flex:1">
            <label>结束时间</label>
            <input type="time" id="mEnd" value="10:00">
        </div>
    </div>
    <div class="form-group">
        <label>内容</label>
        <input type="text" id="mContent" placeholder="请输入任务内容">
    </div>
    <div class="form-group">
        <label>分类</label>
        <select id="mCat">
            <option value="work">工作</option>
            <option value="life">生活</option>
            <option value="study">学习</option>
        </select>
    </div>
    <div style="text-align:right;margin-top:20px">
        <button onclick="saveTodo()" style="width:auto;padding:10px 24px">保存</button>
    </div>
  </div>
</div>

<!-- 确认弹窗 -->
<div id="confirmModal" class="modal">
  <div class="modal-content" style="max-width:300px;text-align:center">
    <h3 id="confirmTitle">确认操作</h3>
    <p id="confirmMsg" style="margin:20px 0;font-size:16px">确定要执行此操作吗？</p>
    <div style="display:flex;justify-content:center;gap:15px">
        <button onclick="closeConfirm()" style="background:#f5f5f5;color:#333;border:1px solid #ddd">取消</button>
        <button id="confirmBtn" style="background:#ff4d4f">确定</button>
    </div>
  </div>
</div>

<script>
const catNames = {work:'工作',life:'生活',study:'学习'};
async function load() {
  try {
    const res = await fetch('/api/data');
    const data = await res.json();
    renderTable(data.schedule);
    document.getElementById('todayFocus').innerText = data.pomodoro.today;
    document.getElementById('totalFocus').innerText = data.pomodoro.total;
    renderFocusList(data.focusRecords);
  } catch(e) { console.error(e); }
}

function renderTable(list) {
  const tbody = document.getElementById('scheduleBody');
  tbody.innerHTML = '';
  if(list.length === 0) {
      tbody.innerHTML = '<tr><td colspan="5" style="text-align:center;color:#999">暂无任务</td></tr>';
      return;
  }
  list.sort((a,b) => (a.date + a.startTime).localeCompare(b.date + b.startTime));
  
  list.forEach((item) => {
     const catClass = item.category || 'work';
     const catText = catNames[item.category] || '工作';
     tbody.innerHTML += `
     <tr>
         <td>${item.date}</td>
         <td>${item.startTime} - ${item.endTime || item.startTime}</td>
         <td>${item.content}</td>
         <td><span class="tag ${catClass}">${catText}</span></td>
         <td><button class="del" onclick="delTodo('${item.id}')">删除</button></td>
     </tr>`;
   });
}

function renderFocusList(list) {
  const ul = document.getElementById('focusList');
  if(!ul || !list) return;
  ul.innerHTML = '';
  const recent = list.slice(-10).reverse();
  
  const fmtTime = (ts) => {
      const d = new Date(ts * 1000);
      return d.getUTCHours().toString().padStart(2,'0') + ":" + d.getUTCMinutes().toString().padStart(2,'0');
  };
  const fmtDate = (ts) => {
      const d = new Date(ts * 1000);
      return (d.getUTCMonth()+1) + "月" + d.getUTCDate() + "日";
  };

  recent.forEach((item) => {
    const timeStr = fmtTime(item.startTime) + " - " + fmtTime(item.endTime);
    const dateStr = fmtDate(item.startTime);
    const note = item.note || "";
    
    ul.innerHTML += `
    <li style="background:#fafafa;border-bottom:1px solid #eee;padding:15px;margin-bottom:10px;border-radius:4px">
        <div style="display:flex;justify-content:space-between;align-items:center">
            <div>
                <div style="font-weight:bold">${dateStr}</div>
                <div style="color:#666;font-size:14px">${timeStr} <span style="color:#1890ff;margin-left:5px">(${item.duration}分钟)</span></div>
            </div>
            <button class="del" onclick="delFocus('${item.id}')">删除</button>
        </div>
        <div style="display:flex;gap:5px;margin-top:10px">
            <input type="text" value="${note}" placeholder="添加备注..." id="note-${item.id}" style="margin:0;font-size:14px">
            <button onclick="updateFocus('${item.id}')" style="width:auto;padding:5px 12px;font-size:14px">保存</button>
        </div>
    </li>`;
  });
}

const modal = document.getElementById("taskModal");
const confirmModal = document.getElementById("confirmModal");
let pendingAction = null;

function openModal() {
    modal.style.display = "block";
    const d = new Date();
    const yyyy = d.getFullYear();
    const mm = String(d.getMonth() + 1).padStart(2, '0');
    const dd = String(d.getDate()).padStart(2, '0');
    document.getElementById('mDate').value = `${yyyy}-${mm}-${dd}`;
}
function closeModal() { modal.style.display = "none"; }

function showConfirm(msg, action) {
    document.getElementById("confirmMsg").innerText = msg;
    pendingAction = action;
    confirmModal.style.display = "block";
}
function closeConfirm() {
    confirmModal.style.display = "none";
    pendingAction = null;
}
document.getElementById("confirmBtn").onclick = function() {
    if(pendingAction) pendingAction();
    closeConfirm();
};

window.onclick = function(event) {
    if (event.target == modal) closeModal();
    if (event.target == confirmModal) closeConfirm();
}

async function saveTodo() {
  const btn = document.querySelector("#taskModal button");
  const date = document.getElementById('mDate').value;
  const start = document.getElementById('mStart').value;
  const end = document.getElementById('mEnd').value;
  const content = document.getElementById('mContent').value;
  const cat = document.getElementById('mCat').value;
  
  if(!content) { alert("请输入任务内容"); return; }

  btn.disabled = true;
  btn.innerText = "保存中...";

  const payload = {
      action: 'add',
      item: {
          date: date,
          startTime: start,
          endTime: end,
          content: content,
          category: cat,
          isDone: false
      }
  };

  try {
      await fetch('/api/schedule', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(payload)});
      closeModal();
      document.getElementById('mContent').value = '';
      load();
  } catch(e) {
      alert("保存失败，请重试");
  } finally {
      btn.disabled = false;
      btn.innerText = "保存";
  }
}

async function delTodo(id) {
  const msg = id === 'all' ? "确定要清空所有任务吗？此操作无法撤销！" : "确定要删除这条任务吗？";
  showConfirm(msg, async () => {
      await fetch('/api/schedule', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({action:'del', id:id})});
      load();
  });
}

async function updateFocus(id) {
    const note = document.getElementById(`note-${id}`).value;
    await fetch('/api/focus', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({action:'update', id:id, note:note})});
    load();
}

async function delFocus(id) {
    showConfirm("确定要删除这条记录吗？", async () => {
        await fetch('/api/focus', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({action:'del', id:id})});
        load();
    });
}

load();
</script></body></html>
)rawliteral";

// ------------------- 二维码绑定页面渲染 -------------------
void renderBindingQRPage() {
    if (pageChanged) {
        tft.fillScreen(TFT_BLACK);

        // 获取二维码内容
        String qrContent = DB.getQRCodeContent();
        Serial.printf("QR Content: %s\n", qrContent.c_str());

        // 创建二维码
        QRCode qrcode;
        uint8_t qrcodeData[qrcode_getBufferSize(6)]; // Version 6 = 41x41 modules
        qrcode_initText(&qrcode, qrcodeData, 6, ECC_LOW, qrContent.c_str());

        // 计算绘制参数 - 在240x240屏幕上居中显示
        int scale = 4; // 每个模块4像素
        int qrSize = qrcode.size * scale;
        int offsetX = (240 - qrSize) / 2;
        int offsetY = 50; // 顶部留空显示文字

        // 绘制白色背景
        tft.fillRect(offsetX - 8, offsetY - 8, qrSize + 16, qrSize + 16, TFT_WHITE);

        // 绘制二维码
        for (uint8_t y = 0; y < qrcode.size; y++) {
            for (uint8_t x = 0; x < qrcode.size; x++) {
                if (qrcode_getModule(&qrcode, x, y)) {
                    tft.fillRect(offsetX + x * scale, offsetY + y * scale, scale, scale, TFT_BLACK);
                }
            }
        }

        // 绘制提示文字
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextDatum(TC_DATUM);
        tft.drawString("WeChat Scan", 120, 10, 4);

        // 底部显示设备ID
        tft.setTextDatum(BC_DATUM);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString(DB.getBindingConfig().deviceId, 120, 230, 2);

        pageChanged = false;
    }
}

void setupWebserver() {
  // 添加CORS支持
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, X-Auth-Token");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // ==================== 设备信息与绑定API ====================

  // 获取设备信息
  server.on("/api/device/info", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(512);
    BindingConfig& cfg = DB.getBindingConfig();

    doc["deviceId"] = cfg.deviceId;
    doc["isBound"] = cfg.isBound;
    doc["boundNickname"] = cfg.boundNickname;
    doc["ip"] = WiFi.localIP().toString();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // 绑定设备 (小程序调用)
  server.on("/api/device/bind", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    DynamicJsonDocument doc(512);
    deserializeJson(doc, data);

    String token = doc["token"].as<String>();
    String userId = doc["userId"].as<String>();
    String nickname = doc["nickname"].as<String>();

    DynamicJsonDocument resp(256);

    if (!DB.verifyBindToken(token)) {
        resp["success"] = false;
        resp["error"] = "Invalid token";
    } else if (DB.getBindingConfig().isBound) {
        resp["success"] = false;
        resp["error"] = "Device already bound";
    } else {
        DB.bindDevice(userId, nickname);
        resp["success"] = true;
        resp["deviceId"] = DB.getBindingConfig().deviceId;

        // 绑定成功后切换到天气页面
        currentPage = PAGE_WEATHER;
        pageChanged = true;
    }

    String response;
    serializeJson(resp, response);
    request->send(200, "application/json", response);
  });

  // 解绑设备
  server.on("/api/device/unbind", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    DynamicJsonDocument doc(256);
    deserializeJson(doc, data);

    String userId = doc["userId"].as<String>();
    DynamicJsonDocument resp(128);

    // 验证是否是绑定用户
    if (DB.getBindingConfig().boundUserId == userId) {
        DB.unbindDevice();
        resp["success"] = true;

        // 解绑后显示二维码
        currentPage = PAGE_BINDQR;
        pageChanged = true;
    } else {
        resp["success"] = false;
        resp["error"] = "Not authorized";
    }

    String response;
    serializeJson(resp, response);
    request->send(200, "application/json", response);
  });

  // 验证用户权限
  server.on("/api/device/verify", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    DynamicJsonDocument doc(256);
    deserializeJson(doc, data);

    String userId = doc["userId"].as<String>();
    DynamicJsonDocument resp(256);

    BindingConfig& cfg = DB.getBindingConfig();
    if (cfg.isBound && cfg.boundUserId == userId) {
        resp["authorized"] = true;
        resp["deviceId"] = cfg.deviceId;
        resp["nickname"] = cfg.boundNickname;
    } else {
        resp["authorized"] = false;
    }

    String response;
    serializeJson(resp, response);
    request->send(200, "application/json", response);
  });

  // ==================== 原有数据API ====================

  server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(8192); // 增加缓冲区大小
    
    // 1. 日程列表
    JsonArray scheduleArr = doc.createNestedArray("schedule");
    for(const auto& item : DB.getSchedules()) {
        JsonObject obj = scheduleArr.createNestedObject();
        obj["id"] = item.id;
        obj["date"] = item.date;
        obj["startTime"] = item.startTime;
        obj["endTime"] = item.endTime;
        obj["content"] = item.content;
        obj["category"] = item.category;
        obj["isDone"] = item.isDone;
    }
    
    // 2. 番茄钟统计
    JsonObject pom = doc.createNestedObject("pomodoro");
    pom["today"] = DB.getTodayFocusMinutes();
    pom["total"] = DB.getTotalFocusMinutes();
    
    // 3. 专注记录列表
    JsonArray focusArr = doc.createNestedArray("focusRecords");
    for(const auto& rec : DB.getFocusRecords()) {
        JsonObject obj = focusArr.createNestedObject();
        obj["id"] = rec.id;
        obj["startTime"] = rec.startTime;
        obj["endTime"] = rec.endTime;
        obj["duration"] = rec.duration;
        obj["taskId"] = rec.taskId;
        obj["note"] = rec.note;
        obj["interruptions"] = rec.interruptions;
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/schedule", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, data);
    String action = doc["action"];
    
    if (action == "add") {
      JsonObject itemObj = doc["item"];
      ScheduleItem newItem = ScheduleItem::fromJson(itemObj);
      DB.addSchedule(newItem);
    } else if (action == "del") {
      String id = doc["id"];
      DB.deleteSchedule(id);
    }
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/api/focus", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, data);
    String action = doc["action"];
    String id = doc["id"];
    
    if (action == "update") {
      String note = doc["note"];
      DB.updateFocusRecord(id, note);
    } else if (action == "del") {
      DB.deleteFocusRecord(id);
    }
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.begin();
}

// ------------------- 页面渲染逻辑 -------------------

// 1. 天气页面 (保持原有逻辑，但去除阻塞)
void renderWeatherPage() {
  if (pageChanged) {
    tft.fillScreen(0x0000);
    tft.fillRoundRect(0,0,240,200,5,bgColor);
    tft.drawFastHLine(0,34,240,TFT_BLACK);
    tft.drawFastVLine(150,0,34,TFT_BLACK);
    tft.drawFastHLine(0,166,240,TFT_BLACK);
    tft.drawFastVLine(60,166,34,TFT_BLACK);
    tft.drawFastVLine(160,166,34,TFT_BLACK);
    
    TJpgDec.drawJpg(161,171,temperature, sizeof(temperature));
    TJpgDec.drawJpg(159,130,humidity, sizeof(humidity));
    TJpgDec.drawJpg(0,0,watchtop, sizeof(watchtop));
    TJpgDec.drawJpg(0,220,watchbottom, sizeof(watchbottom));

    // 注意：不要用 NULL 调用 weaterData()。原版逻辑是通过 getCityWeater()/weaterData() 刷新数据。
    // 这里仅绘制框架，并强制下一轮去拉取天气。
    weaterTime = 0;

    pageChanged = false;
  }

  // 动画 (非阻塞)
  imgAnim();

  // 如果后台正在专注，在右上角显示一个小红点或图标提示
  if (isFocusing) {
      tft.fillCircle(230, 10, 4, TFT_RED);
  }

  // 滚动字幕
  scrollBanner();
  
  // 时间显示
  if (now() != prevDisplay) {
    prevDisplay = now();
    // 复制原有digitalClockDisplay逻辑
    clk.setColorDepth(8);
    // 时分
    clk.createSprite(140, 48);
    clk.fillSprite(bgColor);
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(TFT_BLACK, bgColor);
    clk.drawString(hourMinute(),70,24,7);
    clk.pushSprite(28,40);
    clk.deleteSprite();
    // 秒
    clk.createSprite(40, 32);
    clk.fillSprite(bgColor);
    clk.loadFont(FxLED_32);
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(TFT_BLACK, bgColor); 
    clk.drawString(num2str(second()),20,16);
    clk.unloadFont();
    clk.pushSprite(170,60);
    clk.deleteSprite();
    // 底部日期
    clk.loadFont(ZdyLwFont_20);
    clk.createSprite(58, 32);
    clk.fillSprite(bgColor);
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(TFT_BLACK, bgColor);
    clk.drawString(week(),29,16);
    clk.pushSprite(1,168);
    clk.deleteSprite();
    clk.createSprite(98, 32);
    clk.fillSprite(bgColor);
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(TFT_BLACK, bgColor);  
    clk.drawString(monthDay(),49,16);
    clk.pushSprite(61,168);
    clk.deleteSprite();
    clk.unloadFont();
  }

  // 天气更新
  if(millis() - weaterTime > 300000 || weaterTime == 0){ 
    weaterTime = millis();
    getCityWeater();
  }
}

// 2. 日程页面
void renderSchedulePage() {
  if (pageChanged) {
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    
    // 标题渲染 (强制英文)
    tft.setTextDatum(TC_DATUM);
    tft.loadFont(ZdyLwFont_20);
    tft.drawString("Schedule", 120, 10); 
    tft.unloadFont();
    
    // 后台专注提示
    if (isFocusing) {
        tft.fillCircle(230, 10, 4, TFT_RED);
    }
    
    int y = 50;
    
    std::vector<ScheduleItem>& list = DB.getSchedules();

    if (list.empty()) {
        tft.setTextDatum(CC_DATUM);
        tft.drawString("No Tasks", 120, 100, 4);
    } else {
        // 使用内置字库渲染英文内容，避免乱码
        tft.loadFont(ZdyLwFont_20);
        
        tft.setTextDatum(ML_DATUM);
        tft.setTextColor(TFT_BLACK, 0xE71C); // 设置文字颜色和背景色(灰色)

        int count = 0;
        for (const auto& item : list) {
          if (count >= 4) break;
          
          // 绘制灰色背景条
          tft.fillRoundRect(10, y, 220, 30, 5, 0xE71C);
          
          // 绘制日期 + 时间 + 内容
          // 格式: "05-14 09:00 Meeting"
          String dateStr = item.date.length() >= 5 ? item.date.substring(5) : item.date;
          String displayStr = dateStr + " " + item.startTime + " " + item.content;
          tft.drawString(displayStr, 20, y + 15);
          
          y += 40;
          count++;
        }
        
        tft.unloadFont();
    }
    
    pageChanged = false;
  }
}

// 3. 番茄钟页面
void renderPomodoroPage() {
  static unsigned long lastUpdate = 0;
  if (pageChanged) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    
    // 强制英文
    tft.drawString("Pomodoro", 120, 20, 4);
    tft.drawString("Long Press: Start/Stop", 120, 200, 2);

    pageChanged = false;
    lastUpdate = 0; // 强制更新
  }

  if (millis() - lastUpdate > 1000 || pageChanged) { // 页面切换回来时立即更新
    unsigned long remaining = FOCUS_DURATION;
    if (isFocusing) {
      unsigned long elapsed = millis() - focusStartTime;
      if (elapsed >= FOCUS_DURATION) {
        // ... (保持原有完成逻辑)
        isFocusing = false;
        
        // 记录专注时间到数据库
        FocusRecord rec;
        rec.startTime = now() - (FOCUS_DURATION/1000); // 倒推开始时间
        rec.endTime = now();                           // 记录结束时间
        rec.duration = 25;
        rec.interruptions = 0;
        rec.note = ""; // 默认为空
        DB.addFocusRecord(rec);
        
        remaining = 0;
        tft.fillScreen(TFT_GREEN);
        tft.setTextColor(TFT_BLACK, TFT_GREEN);
        tft.drawString("Done!", 120, 100, 4);
        delay(2000);
        pageChanged = true; 
        return;
      }
      remaining -= elapsed;
    }
    
    int seconds = (remaining / 1000) % 60;
    int minutes = (remaining / 1000) / 60;
    
    String timeStr = num2str(minutes) + ":" + num2str(seconds);
    
    // 使用大字体显示时间
    clk.setColorDepth(8);
    clk.createSprite(200, 80);
    clk.fillSprite(TFT_BLACK);
    clk.setTextColor(isFocusing ? TFT_GREEN : TFT_YELLOW, TFT_BLACK);
    clk.setTextDatum(CC_DATUM);
    clk.drawString(timeStr, 100, 40, 7); 
    clk.pushSprite(20, 80);
    clk.deleteSprite();
    
    lastUpdate = millis();
  }
}

// ------------------- 按键回调 -------------------
void onBtnClick() {
  isLongPressing = false; // 重置长按状态

  // 未绑定状态下，点击刷新二维码
  if (currentPage == PAGE_BINDQR) {
    DB.generateBindToken();
    pageChanged = true;
    return;
  }

  // 已绑定状态下，切换页面
  int next = (int)currentPage + 1;
  if (next > PAGE_POMODORO) next = PAGE_WEATHER;
  currentPage = (Page)next;
  pageChanged = true;
}

void onBtnLongPress() {
  isLongPressing = false; // 重置

  if (currentPage == PAGE_POMODORO) {
    if (isFocusing) {
      // 长按停止专注并保存
      unsigned long elapsed = millis() - focusStartTime;
      int minutes = elapsed / 1000 / 60;
      
      if (minutes >= 1) {
          FocusRecord rec;
          rec.endTime = now();
          rec.startTime = rec.endTime - (elapsed / 1000);
          rec.duration = minutes;
          rec.interruptions = 1;
          rec.note = "";
          DB.addFocusRecord(rec);
      }
      isFocusing = false;
      Serial.println("Pomodoro Stopped (LongPress)");
    } else {
      isFocusing = true;
      focusStartTime = millis();
      Serial.println("Pomodoro Started");
    }
    pageChanged = true;
  }
}

// ------------------- 动画重构 (非阻塞) -------------------
unsigned long lastAnimTime = 0;
int animFrame = 0;
void imgAnim(){
  if (millis() - lastAnimTime > 30) {
    int x=80,y=94;
    switch(animFrame) {
      case 0: TJpgDec.drawJpg(x,y,i0, sizeof(i0)); break;
      case 1: TJpgDec.drawJpg(x,y,i1, sizeof(i1)); break;
      case 2: TJpgDec.drawJpg(x,y,i2, sizeof(i2)); break;
      case 3: TJpgDec.drawJpg(x,y,i3, sizeof(i3)); break;
      case 4: TJpgDec.drawJpg(x,y,i4, sizeof(i4)); break;
      case 5: TJpgDec.drawJpg(x,y,i5, sizeof(i5)); break;
      case 6: TJpgDec.drawJpg(x,y,i6, sizeof(i6)); break;
      case 7: TJpgDec.drawJpg(x,y,i7, sizeof(i7)); break;
      case 8: TJpgDec.drawJpg(x,y,i8, sizeof(i8)); break;
      case 9: TJpgDec.drawJpg(x,y,i9, sizeof(i9)); break;
    }
    animFrame++;
    if(animFrame > 9) animFrame = 0;
    lastAnimTime = millis();
  }
}

// ------------------- Setup & Loop -------------------
void loading(byte delayTime){
  clk.setColorDepth(8);
  clk.createSprite(200, 50);
  clk.fillSprite(0x0000);
  clk.drawRoundRect(0,0,200,16,8,0xFFFF);
  clk.fillRoundRect(3,3,100,10,5,0xFFFF); // Simplified loading
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_GREEN, 0x0000); 
  clk.drawString("Connecting to WiFi",100,40,2);
  clk.pushSprite(20,110);
  clk.deleteSprite();
  delay(delayTime);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n========================================");
  Serial.println("🚀 太空人智能时钟 v1.0");
  Serial.println("========================================");

  DB.begin(); // 初始化数据管理器
  DB.initDeviceId(); // 确保设备ID已初始化
  Serial.printf("📱 Device ID: %s\n", DB.getBindingConfig().deviceId.c_str());

  Serial.println("🖥️  初始化屏幕...");
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(0x0000);
  tft.setTextColor(TFT_BLACK, bgColor);

  // WiFi
  Serial.printf("📡 正在连接WiFi: %s\n", ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    loading(100);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi已连接！");
  Serial.print("🌐 IP地址: ");
  Serial.println(WiFi.localIP());
  Serial.println("========================================");

  // UDP & Time
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(300);

  // TJpgDec
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  // Web Server
  setupWebserver();
  Serial.printf("🌍 Web服务器: http://%s/\n", WiFi.localIP().toString().c_str());

  // Button
  btn.attachClick(onBtnClick);
  btn.attachLongPressStart(onBtnLongPress);
  btn.attachDuringLongPress(onBtnDuringLongPress);

  // 检查绑定状态 - 未绑定则显示二维码
  if (!DB.getBindingConfig().isBound) {
    // 关键：确保 bindToken 有值，但不要每次循环都生成
    if (DB.getBindingConfig().bindToken.length() == 0) {
      DB.generateBindToken();
    }

    currentPage = PAGE_BINDQR;
    Serial.println("⚠️  设备未绑定，显示二维码页面");
    Serial.println("📱 请使用微信小程序扫码绑定");
  } else {
    currentPage = PAGE_WEATHER;
    Serial.printf("✅ 设备已绑定: %s\n", DB.getBindingConfig().boundNickname.c_str());
  }
  Serial.println("========================================\n");

  pageChanged = true;
}

void loop() {
  btn.tick();
  
  switch(currentPage) {
    case PAGE_BINDQR:
      renderBindingQRPage();
      break;
    case PAGE_WEATHER:
      renderWeatherPage();
      break;
    case PAGE_SCHEDULE:
      renderSchedulePage();
      break;
    case PAGE_POMODORO:
      renderPomodoroPage();
      break;
  }
}

// 长按过程中检测 - 用于恢复出厂设置
void onBtnDuringLongPress() {
    if (!isLongPressing) {
        isLongPressing = true;
        buttonPressStart = millis();
    }

    unsigned long holdTime = millis() - buttonPressStart;

    // 显示重置进度
    if (holdTime > 3000) { // 3秒后开始显示提示
        int progress = map(holdTime, 3000, FACTORY_RESET_HOLD, 0, 100);
        if (progress > 100) progress = 100;

        tft.fillRect(20, 200, 200, 30, TFT_BLACK);
        tft.drawRect(20, 205, 200, 20, TFT_WHITE);
        tft.fillRect(22, 207, progress * 196 / 100, 16, TFT_RED);

        if (holdTime >= FACTORY_RESET_HOLD) {
            // 执行恢复出厂设置
            tft.fillScreen(TFT_RED);
            tft.setTextColor(TFT_WHITE, TFT_RED);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("FACTORY RESET", 120, 100, 4);
            tft.drawString("Please wait...", 120, 140, 2);

            DB.factoryReset();
            delay(2000);

            // 重启设备
            ESP.restart();
        }
    }
}

// ------------------- 辅助函数 (保持原有逻辑) -------------------

// 这里我们需要把原来放在 loop 外面定义的函数补全
void getCityCode(){
 String URL = "http://wgeo.weather.com.cn/ip/?_="+String(now());
  HTTPClient httpClient;
  WiFiClient client;
  httpClient.begin(client, URL); 
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");
  int httpCode = httpClient.GET();
  if (httpCode == HTTP_CODE_OK) {
    String str = httpClient.getString();
    int aa = str.indexOf("id=");
    if(aa>-1){
       cityCode = str.substring(aa+4,aa+4+9);
       getCityWeater();
    }
  } 
  httpClient.end();
}

void getCityWeater(){
 String URL = "http://d1.weather.com.cn/weather_index/" + cityCode + ".html?_="+String(now());
  HTTPClient httpClient;
  WiFiClient client;
  httpClient.begin(client, URL); 
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");
  int httpCode = httpClient.GET();
  if (httpCode == HTTP_CODE_OK) {
    String str = httpClient.getString();
    int indexStart = str.indexOf("weatherinfo\":");
    int indexEnd = str.indexOf("};var alarmDZ");
    String jsonCityDZ = str.substring(indexStart+13,indexEnd);
    indexStart = str.indexOf("dataSK =");
    indexEnd = str.indexOf(";var dataZS");
    String jsonDataSK = str.substring(indexStart+8,indexEnd);
    indexStart = str.indexOf("\"f\":[");
    indexEnd = str.indexOf(",{\"fa");
    String jsonFC = str.substring(indexStart+5,indexEnd);
    weaterData(&jsonCityDZ,&jsonDataSK,&jsonFC);
  } 
  httpClient.end();
}

void weaterData(String *cityDZ,String *dataSK,String *dataFC){
  if (cityDZ == NULL) return; // 防止空指针

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, *dataSK);
  JsonObject sk = doc.as<JsonObject>();

  // 温度
  clk.setColorDepth(8);
  clk.loadFont(ZdyLwFont_20);
  clk.createSprite(54, 32); 
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_BLACK, bgColor); 
  clk.drawString(sk["temp"].as<String>()+"℃",27,16);
  clk.pushSprite(185,168);
  clk.deleteSprite();

  // 城市名称
  clk.createSprite(88, 32); 
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_BLACK, bgColor); 
  clk.drawString(sk["cityname"].as<String>(),44,16);
  clk.pushSprite(151,1);
  clk.deleteSprite();
  
  // PM2.5
  uint16_t pm25BgColor = tft.color565(156,202,127);
  String aqiTxt = "优";
  int pm25V = sk["aqi"];
  if(pm25V>200){
    pm25BgColor = tft.color565(136,11,32); aqiTxt = "重度";
  }else if(pm25V>150){
    pm25BgColor = tft.color565(186,55,121); aqiTxt = "中度";
  }else if(pm25V>100){
    pm25BgColor = tft.color565(242,159,57); aqiTxt = "轻度";
  }else if(pm25V>50){
    pm25BgColor = tft.color565(247,219,100); aqiTxt = "良";
  }
  clk.createSprite(50, 24); 
  clk.fillSprite(bgColor);
  clk.fillRoundRect(0,0,50,24,4,pm25BgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(0xFFFF); 
  clk.drawString(aqiTxt,25,13);
  clk.pushSprite(5,130);
  clk.deleteSprite();

  // 湿度
  clk.createSprite(56, 24); 
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_BLACK, bgColor); 
  clk.drawString(sk["SD"].as<String>(),28,13);
  clk.pushSprite(180,130);
  clk.deleteSprite();

  scrollText[0] = "实时天气 "+sk["weather"].as<String>();
  scrollText[1] = "空气质量 "+aqiTxt;
  scrollText[2] = "风向 "+sk["WD"].as<String>()+sk["WS"].as<String>();
  
  deserializeJson(doc, *cityDZ);
  JsonObject dz = doc.as<JsonObject>();
  scrollText[3] = "今日"+dz["weather"].as<String>();
  
  deserializeJson(doc, *dataFC);
  JsonObject fc = doc.as<JsonObject>();
  scrollText[4] = "最低温度"+fc["fd"].as<String>()+"℃";
  scrollText[5] = "最高温度"+fc["fc"].as<String>()+"℃";
  
  clk.unloadFont();
}

int currentIndex = 0;
int prevTimeScroll = 0;
void scrollBanner(){
  if(millis() - prevTimeScroll > 2500){ 
    if(scrollText[currentIndex]){
      clkb.setColorDepth(8);
      clkb.loadFont(ZdyLwFont_20);
      for(int pos = 24; pos>0 ; pos--){
        scrollTxt(pos);
      }
      clkb.deleteSprite();
      clkb.unloadFont();
      if(currentIndex>=5) currentIndex = 0;
      else currentIndex += 1;
    }
    prevTimeScroll = millis();
  }
}

void scrollTxt(int pos){
    clkb.createSprite(148, 24); 
    clkb.fillSprite(bgColor);
    clkb.setTextWrap(false);
    clkb.setTextDatum(CC_DATUM);
    clkb.setTextColor(TFT_BLACK, bgColor); 
    clkb.drawString(scrollText[currentIndex],74,pos+12);
    clkb.pushSprite(2,4);
}

String week(){
  String wk[7] = {"日","一","二","三","四","五","六"};
  return "周" + wk[weekday()-1];
}

String monthDay(){
  return String(month()) + "月" + day() + "日";
}

String hourMinute(){
  return num2str(hour()) + ":" + num2str(minute());
}

String num2str(int digits){
  String s = "";
  if (digits < 10) s = s + "0";
  s = s + digits;
  return s;
}

time_t getNtpTime(){
  IPAddress ntpServerIP; 
  while (Udp.parsePacket() > 0) ; 
  WiFi.hostByName(ntpServerName, ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= 48) {
      Udp.read(packetBuffer, 48); 
      unsigned long secsSince1900;
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * 3600;
    }
  }
  return 0; 
}

void sendNTPpacket(IPAddress &address){
  memset(packetBuffer, 0, 48);
  packetBuffer[0] = 0b11100011;   
  packetBuffer[1] = 0;     
  packetBuffer[2] = 6;     
  packetBuffer[3] = 0xEC;  
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  Udp.beginPacket(address, 123); 
  Udp.write(packetBuffer, 48);
  Udp.endPacket();
}
