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
enum Page { PAGE_WEATHER, PAGE_SCHEDULE, PAGE_POMODORO };
Page currentPage = PAGE_WEATHER;
bool pageChanged = true;

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
time_t now();

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
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Space Clock Console v2.1</title>
<style>
body{font-family:sans-serif;margin:0;padding:20px;background:#f0f2f5;color:#333}
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

/* Modal Styles */
.modal{display:none;position:fixed;z-index:99;left:0;top:0;width:100%;height:100%;overflow:auto;background-color:rgba(0,0,0,0.4)}
.modal-content{background-color:#fefefe;margin:10% auto;padding:25px;border:1px solid #888;width:90%;max-width:500px;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,0.15)}
.close{color:#aaa;float:right;font-size:28px;font-weight:bold;cursor:pointer}
.close:hover{color:#000}
.form-group{margin-bottom:15px}
.form-group label{display:block;margin-bottom:5px;font-weight:500}
</style></head><body>

<h1>Space Clock Console</h1>

<div class="card">
  <div style="display:flex;justify-content:space-between;align-items:center">
    <h2>Schedule</h2>
    <div>
        <button onclick="delTodo('all')" style="width:auto;padding:8px 16px;background:#ff4d4f;margin-right:10px">Clear All</button>
        <button onclick="openModal()" style="width:auto;padding:8px 16px">+ New Task</button>
    </div>
  </div>
  <div style="overflow-x:auto">
    <table id="scheduleTable">
      <thead><tr><th>Date</th><th>Time</th><th>Content</th><th>Cat</th><th>Action</th></tr></thead>
      <tbody id="scheduleBody"></tbody>
    </table>
  </div>
</div>

<div class="card">
  <h2>Focus Stats</h2>
  <div style="display:flex;justify-content:space-around;text-align:center;margin-bottom:20px">
    <div>
        <div style="font-size:24px;font-weight:bold;color:#1890ff" id="todayFocus">0</div>
        <div style="color:#666">Today (mins)</div>
    </div>
    <div>
        <div style="font-size:24px;font-weight:bold;color:#52c41a" id="totalFocus">0</div>
        <div style="color:#666">Total (mins)</div>
    </div>
  </div>
  <h3>Recent Sessions</h3>
  <ul id="focusList" style="list-style:none;padding:0"></ul>
</div>

<!-- Add Task Modal -->
<div id="taskModal" class="modal">
  <div class="modal-content">
    <span class="close" onclick="closeModal()">&times;</span>
    <h3>Add New Task</h3>
    <div class="form-group">
        <label>Date</label>
        <input type="date" id="mDate">
    </div>
    <div class="row">
        <div class="form-group" style="flex:1">
            <label>Start Time</label>
            <input type="time" id="mStart" value="09:00">
        </div>
        <div class="form-group" style="flex:1">
            <label>End Time</label>
            <input type="time" id="mEnd" value="10:00">
        </div>
    </div>
    <div class="form-group">
        <label>Content</label>
        <input type="text" id="mContent" placeholder="What to do?">
    </div>
    <div class="form-group">
        <label>Category</label>
        <select id="mCat">
            <option value="work">Work</option>
            <option value="life">Life</option>
            <option value="study">Study</option>
        </select>
    </div>
    <div style="text-align:right;margin-top:20px">
        <button onclick="saveTodo()" style="width:auto;padding:10px 24px">Save</button>
    </div>
  </div>
</div>

<!-- Confirm Modal -->
<div id="confirmModal" class="modal">
  <div class="modal-content" style="max-width:300px;text-align:center">
    <h3 id="confirmTitle">Confirm</h3>
    <p id="confirmMsg" style="margin:20px 0;font-size:16px">Are you sure?</p>
    <div style="display:flex;justify-content:center;gap:15px">
        <button onclick="closeConfirm()" style="background:#f5f5f5;color:#333;border:1px solid #ddd">Cancel</button>
        <button id="confirmBtn" style="background:#ff4d4f">Delete</button>
    </div>
  </div>
</div>

<script>
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
      tbody.innerHTML = '<tr><td colspan="5" style="text-align:center;color:#999">No tasks yet</td></tr>';
      return;
  }
  // Sort by date/time
  list.sort((a,b) => (a.date + a.startTime).localeCompare(b.date + b.startTime));
  
  list.forEach((item) => {
     const catClass = item.category || 'work';
     const catText = item.category ? item.category.toUpperCase() : 'WORK';
     tbody.innerHTML += `
     <tr>
         <td>${item.date}</td>
         <td>${item.startTime} - ${item.endTime || item.startTime}</td>
         <td>${item.content}</td>
         <td><span class="tag ${catClass}">${catText}</span></td>
         <td><button class="del" onclick="delTodo('${item.id}')">Del</button></td>
     </tr>`;
   });
}

function renderFocusList(list) {
  const ul = document.getElementById('focusList');
  if(!ul || !list) return;
  ul.innerHTML = '';
  const recent = list.slice(-10).reverse();
  
  // Helper to format timestamp (treated as UTC to avoid browser timezone shift)
  const fmtTime = (ts) => {
      const d = new Date(ts * 1000);
      return d.getUTCHours().toString().padStart(2,'0') + ":" + d.getUTCMinutes().toString().padStart(2,'0');
  };
  const fmtDate = (ts) => {
      const d = new Date(ts * 1000);
      return (d.getUTCMonth()+1) + "/" + d.getUTCDate();
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
                <div style="color:#666;font-size:14px">${timeStr} <span style="color:#1890ff;margin-left:5px">(${item.duration}m)</span></div>
            </div>
            <button class="del" onclick="delFocus('${item.id}')">Del</button>
        </div>
        <div style="display:flex;gap:5px;margin-top:10px">
            <input type="text" value="${note}" placeholder="Add note..." id="note-${item.id}" style="margin:0;font-size:14px">
            <button onclick="updateFocus('${item.id}')" style="width:auto;padding:5px 12px;font-size:14px">Save</button>
        </div>
    </li>`;
  });
}

// Modal Logic
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
  
  if(!content) { alert("Please enter task content"); return; }
  
  // 防抖：禁用按钮并更改文本
  btn.disabled = true;
  btn.innerText = "Saving...";
  
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
      alert("Failed to save task. Please try again.");
  } finally {
      // 恢复按钮状态
      btn.disabled = false;
      btn.innerText = "Save";
  }
}

async function delTodo(id) {
  const msg = id === 'all' ? "Clear ALL tasks? This cannot be undone!" : "Delete this task?";
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
    showConfirm("Delete this record?", async () => {
        await fetch('/api/focus', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({action:'del', id:id})});
        load();
    });
}

load();
</script></body></html>
)rawliteral";

void setupWebserver() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

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
    
    // 强制刷新一次数据
    weaterData(NULL, NULL, NULL); // 需要修改weaterData以支持缓存重绘，这里暂时只绘制框架
    // 简单起见，这里假设weaterTime会自动触发刷新
    weaterTime = 0; // 强制刷新
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
  // 即使在专注模式下，也允许切换页面查看其他信息
  // 专注状态 isFocusing 保持不变，计时继续在后台运行

  // 切换页面
  int next = (int)currentPage + 1;
  if (next > PAGE_POMODORO) next = PAGE_WEATHER;
  currentPage = (Page)next;
  pageChanged = true;
}

void onBtnLongPress() {
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
  Serial.begin(9600);
  // LittleFS.begin(); // DataManager 会自己调用
  DB.begin(); // 初始化数据管理器
  // loadData(); // 已废弃

  // 运行存储自检 (已禁用，防止重复写数据)
  // DB.runSelfTest();

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(0x0000);
  tft.setTextColor(TFT_BLACK, bgColor);

  // WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    loading(100);
  }
  Serial.println(WiFi.localIP());

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

  // 尝试加载字体 (已禁用)
  // loadCustomFont();

  // Button
  btn.attachClick(onBtnClick);
  btn.attachLongPressStart(onBtnLongPress);
  
  // 初始化界面
  pageChanged = true;
}

void loop() {
  btn.tick();
  
  switch(currentPage) {
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
