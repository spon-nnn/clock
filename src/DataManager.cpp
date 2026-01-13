#include "DataManager.h"
#include <TimeLib.h>
#include <algorithm> // 引入排序算法
#include <ESP8266WiFi.h> // 获取芯片ID和IP

DataManager DB;

DataManager::DataManager() {}

void DataManager::begin() {
    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed");
        return;
    }
    loadBindingConfig();
    loadSchedules();
    loadFocusRecords();
}

// ==================== 设备绑定管理实现 ====================

void DataManager::initDeviceId() {
    if (bindingConfig.deviceId.length() == 0) {
        // 基于ESP8266芯片ID生成唯一设备ID
        uint32_t chipId = ESP.getChipId();
        bindingConfig.deviceId = "SC_" + String(chipId, HEX);
        bindingConfig.deviceId.toUpperCase();
        generateBindToken();
        saveBindingConfig();
        Serial.printf("Device ID initialized: %s\n", bindingConfig.deviceId.c_str());
    }
}

void DataManager::generateBindToken() {
    // 生成6位随机Token
    String token = "";
    const char charset[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; // 去掉易混淆字符
    for (int i = 0; i < 6; i++) {
        token += charset[random(0, sizeof(charset) - 1)];
    }
    bindingConfig.bindToken = token;
    saveBindingConfig();
    Serial.printf("New bind token: %s\n", token.c_str());
}

BindingConfig& DataManager::getBindingConfig() {
    return bindingConfig;
}

String DataManager::getQRCodeContent() {
    // 生成二维码内容JSON
    DynamicJsonDocument doc(256);
    doc["type"] = "spaceclock";
    doc["did"] = bindingConfig.deviceId;
    doc["token"] = bindingConfig.bindToken;
    doc["ip"] = WiFi.localIP().toString();

    String output;
    serializeJson(doc, output);
    return output;
}

bool DataManager::verifyBindToken(String token) {
    return token == bindingConfig.bindToken;
}

bool DataManager::bindDevice(String userId, String nickname) {
    if (bindingConfig.isBound) {
        Serial.println("Device already bound!");
        return false;
    }

    bindingConfig.isBound = true;
    bindingConfig.boundUserId = userId;
    bindingConfig.boundNickname = nickname;
    bindingConfig.bindTime = now();
    saveBindingConfig();

    Serial.printf("Device bound to user: %s (%s)\n", nickname.c_str(), userId.c_str());
    return true;
}

void DataManager::unbindDevice() {
    bindingConfig.isBound = false;
    bindingConfig.boundUserId = "";
    bindingConfig.boundNickname = "";
    bindingConfig.bindTime = 0;
    generateBindToken(); // 生成新Token
    saveBindingConfig();
    Serial.println("Device unbound");
}

void DataManager::factoryReset() {
    // 清除所有数据
    schedules.clear();
    focusRecords.clear();

    // 重置绑定状态
    bindingConfig.isBound = false;
    bindingConfig.boundUserId = "";
    bindingConfig.boundNickname = "";
    bindingConfig.bindTime = 0;
    generateBindToken();

    // 保存并删除旧文件
    LittleFS.remove(SCHEDULE_FILE);
    LittleFS.remove(FOCUS_FILE);
    saveBindingConfig();

    Serial.println("Factory reset complete!");
}

void DataManager::loadBindingConfig() {
    // 先初始化设备ID
    if (bindingConfig.deviceId.length() == 0) {
        uint32_t chipId = ESP.getChipId();
        bindingConfig.deviceId = "SC_" + String(chipId, HEX);
        bindingConfig.deviceId.toUpperCase();
    }

    if (!LittleFS.exists(BINDING_FILE)) {
        // 首次启动，生成Token
        generateBindToken();
        bindingConfig.isBound = false;
        saveBindingConfig();
        return;
    }

    File f = LittleFS.open(BINDING_FILE, "r");
    if (!f) return;

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, f);
    f.close();

    if (error) {
        Serial.println("Failed to parse binding config");
        generateBindToken();
        return;
    }

    bindingConfig.deviceId = doc["deviceId"].as<String>();
    bindingConfig.bindToken = doc["bindToken"].as<String>();
    bindingConfig.isBound = doc["isBound"].as<bool>();
    bindingConfig.boundUserId = doc["boundUserId"].as<String>();
    bindingConfig.boundNickname = doc["boundNickname"].as<String>();
    bindingConfig.bindTime = doc["bindTime"].as<unsigned long>();
    // 加载专注时长配置，默认25
    if (doc.containsKey("focusDuration")) {
        bindingConfig.focusDuration = doc["focusDuration"].as<int>();
    } else {
        bindingConfig.focusDuration = 25;
    }

    Serial.printf("Binding config loaded. Bound: %s\n", bindingConfig.isBound ? "Yes" : "No");
}

void DataManager::saveBindingConfig() {
    File f = LittleFS.open(BINDING_FILE, "w");
    if (!f) {
        Serial.println("Failed to save binding config");
        return;
    }

    DynamicJsonDocument doc(512);
    doc["deviceId"] = bindingConfig.deviceId;
    doc["bindToken"] = bindingConfig.bindToken;
    doc["isBound"] = bindingConfig.isBound;
    doc["boundUserId"] = bindingConfig.boundUserId;
    doc["boundNickname"] = bindingConfig.boundNickname;
    doc["bindTime"] = bindingConfig.bindTime;
    doc["focusDuration"] = bindingConfig.focusDuration; // 保存设置

    serializeJson(doc, f);
    f.close();
}

// ==================== 原有功能 ====================

String DataManager::generateId() {
    return String(millis()) + String(random(1000, 9999));
}

// 辅助排序函数
bool compareSchedules(const ScheduleItem& a, const ScheduleItem& b) {

    if (a.date != b.date) {
        return a.date < b.date; // 按日期升序
    }
    return a.startTime < b.startTime; // 按时间升序
}

void DataManager::addSchedule(ScheduleItem item) {
    // 1. 数量限制
    if (schedules.size() >= 30) {
        Serial.println("[WARN] Schedule limit reached (30). Cannot add more.");
        return;
    }

    // 2. 去重
    for (const auto& s : schedules) {
        if (s.date == item.date && s.startTime == item.startTime && s.content == item.content) {
            Serial.println("[WARN] Duplicate schedule ignored.");
            return;
        }
    }

    if (item.id.length() == 0) item.id = generateId();
    schedules.push_back(item);
    
    // 3. 排序
    std::sort(schedules.begin(), schedules.end(), compareSchedules);
    
    saveSchedules();
}

void DataManager::deleteSchedule(String id) {
    if (id == "all") {
        schedules.clear();
        saveSchedules();
        return;
    }

    for (auto it = schedules.begin(); it != schedules.end(); ++it) {
        if (it->id == id) {
            schedules.erase(it);
            saveSchedules();
            return;
        }
    }
}

void DataManager::updateSchedule(String id, ScheduleItem newItem) {
    for (auto& item : schedules) {
        if (item.id == id) {
            item = newItem;
            item.id = id; // 保持ID不变
            saveSchedules();
            return;
        }
    }
}

std::vector<ScheduleItem>& DataManager::getSchedules() {
    return schedules;
}

void DataManager::loadSchedules() {
    if (!LittleFS.exists(SCHEDULE_FILE)) return;
    
    File f = LittleFS.open(SCHEDULE_FILE, "r");
    if (!f) return;

    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, f);
    f.close();

    if (error) {
        Serial.println("Failed to parse schedules file");
        return;
    }

    schedules.clear();
    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject v : arr) {
        schedules.push_back(ScheduleItem::fromJson(v));
    }
    
    // 自动清理
    if (schedules.size() > 40) {
        Serial.printf("[WARN] Too many schedules (%d)! Truncating to last 20.\n", schedules.size());
        std::vector<ScheduleItem> kept;
        for (size_t i = schedules.size() - 20; i < schedules.size(); i++) {
            kept.push_back(schedules[i]);
        }
        schedules = kept;
    }
    
    // 排序
    std::sort(schedules.begin(), schedules.end(), compareSchedules);
    
    // 执行全量去重 (应对历史遗留的重复数据)
    deduplicateSchedules();
    
    // 如果发生了截断或排序变化，保存一下
    if (schedules.size() > 40) saveSchedules(); 
    
    Serial.printf("Loaded %d schedules\n", schedules.size());
}

void DataManager::deduplicateSchedules() {
    if (schedules.empty()) return;
    
    std::vector<ScheduleItem> unique;
    bool hasDuplicates = false;
    
    for (const auto& item : schedules) {
        bool exists = false;
        for (const auto& u : unique) {
            // 判定重复标准：日期、时间、内容完全一致
            if (u.date == item.date && u.startTime == item.startTime && u.content == item.content) {
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            unique.push_back(item);
        } else {
            hasDuplicates = true;
            Serial.printf("[CLEANUP] Removed duplicate task: %s %s\n", item.date.c_str(), item.content.c_str());
        }
    }
    
    if (hasDuplicates) {
        schedules = unique;
        saveSchedules(); // 立即保存去重后的结果
    }
}

// 原子写入机制：Write -> Flush -> Close -> Rename
void DataManager::saveSchedules() {
    String tempFile = String(SCHEDULE_FILE) + ".tmp";
    File f = LittleFS.open(tempFile, "w");
    if (!f) {
        Serial.println("Failed to open temp file for writing");
        return;
    }

    DynamicJsonDocument doc(8192);
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& item : schedules) {
        item.toJson(doc); // toJson 会自动添加到 arr (因为 arr 是 doc 的引用)
        // Wait, toJson needs to create object IN the doc
        // Let's fix ScheduleItem::toJson to return void and take JsonArray&
    }
    
    // Correct way with ArduinoJson 6:
    for (const auto& item : schedules) {
        JsonObject obj = arr.createNestedObject();
        obj["id"] = item.id;
        obj["date"] = item.date;
        obj["startTime"] = item.startTime;
        obj["endTime"] = item.endTime;
        obj["content"] = item.content;
        obj["category"] = item.category;
        obj["isDone"] = item.isDone;
    }

    if (serializeJson(doc, f) == 0) {
        Serial.println("Failed to write to file");
        f.close();
        return;
    }
    f.close();

    LittleFS.remove(SCHEDULE_FILE);
    LittleFS.rename(tempFile, SCHEDULE_FILE);
    Serial.println("Schedules saved successfully");
}

// ------------------- 专注记录实现 -------------------

void DataManager::addFocusRecord(FocusRecord record) {
    if (record.id.length() == 0) record.id = generateId();
    focusRecords.push_back(record);
    saveFocusRecords();
}

void DataManager::updateFocusRecord(String id, String note) {
    for (auto& rec : focusRecords) {
        if (rec.id == id) {
            rec.note = note;
            saveFocusRecords();
            return;
        }
    }
}

void DataManager::deleteFocusRecord(String id) {
    for (auto it = focusRecords.begin(); it != focusRecords.end(); ++it) {
        if (it->id == id) {
            focusRecords.erase(it);
            saveFocusRecords();
            return;
        }
    }
}

std::vector<FocusRecord>& DataManager::getFocusRecords() {
    return focusRecords;
}

int DataManager::getTodayFocusMinutes() {
    int todayTotal = 0;
    
    // 获取当前时间 (Unix Timestamp)
    time_t nowTime = now();
    // 计算今天的开始时间戳 (00:00:00)
    // 简单算法：当前时间戳 - (当前时间戳 % 86400) (注意时区偏移)
    // 但使用 TimeLib 库更方便：
    // 如果没有 TimeLib，这里需要根据 NTP 时间来判断
    // 假设系统时间已通过 NTP 同步
    
    // 获取今天的年、月、日
    int currentYear = year(nowTime);
    int currentMonth = month(nowTime);
    int currentDay = day(nowTime);

    for (const auto& rec : focusRecords) {
        // 检查每条记录的时间
        if (rec.startTime > 0) {
             if (year(rec.startTime) == currentYear && 
                 month(rec.startTime) == currentMonth && 
                 day(rec.startTime) == currentDay) {
                 todayTotal += rec.duration;
             }
        }
    }
    return todayTotal; 
}

int DataManager::getTotalFocusMinutes() {
    int total = 0;
    for (const auto& rec : focusRecords) {
        total += rec.duration;
    }
    return total;
}

void DataManager::loadFocusRecords() {
    if (!LittleFS.exists(FOCUS_FILE)) return;
    
    File f = LittleFS.open(FOCUS_FILE, "r");
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, f);
    f.close();

    focusRecords.clear();
    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject v : arr) {
        focusRecords.push_back(FocusRecord::fromJson(v));
    }
}

void DataManager::saveFocusRecords() {
    String tempFile = String(FOCUS_FILE) + ".tmp";
    File f = LittleFS.open(tempFile, "w");
    
    DynamicJsonDocument doc(4096);
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& rec : focusRecords) {
        JsonObject obj = arr.createNestedObject();
        obj["id"] = rec.id;
        obj["startTime"] = rec.startTime;
        obj["endTime"] = rec.endTime; // 新增字段
        obj["duration"] = rec.duration;
        obj["taskId"] = rec.taskId;
        obj["note"] = rec.note;       // 新增字段
        obj["interruptions"] = rec.interruptions;
    }

    serializeJson(doc, f);
    f.close();

    LittleFS.remove(FOCUS_FILE);
    LittleFS.rename(tempFile, FOCUS_FILE);
}

// ------------------- 自检/单元测试 -------------------

bool DataManager::runSelfTest() {
    Serial.println("[TEST] Starting Storage Self-Test...");
    
    // 1. Create
    String testId = "test_999";
    ScheduleItem item = {testId, "2024-01-01", "10:00", "11:00", "Test Task", "work", false};
    addSchedule(item);
    
    // 2. Read
    bool found = false;
    for(const auto& s : schedules) {
        if(s.id == testId && s.content == "Test Task") found = true;
    }
    if(!found) { Serial.println("[TEST] FAIL: Add/Read"); return false; }
    
    // 3. Update
    item.content = "Updated Task";
    updateSchedule(testId, item);
    found = false;
    for(const auto& s : schedules) {
        if(s.id == testId && s.content == "Updated Task") found = true;
    }
    if(!found) { Serial.println("[TEST] FAIL: Update"); return false; }
    
    // 4. Delete
    deleteSchedule(testId);
    for(const auto& s : schedules) {
        if(s.id == testId) { Serial.println("[TEST] FAIL: Delete"); return false; }
    }
    
    Serial.println("[TEST] PASSED: All CRUD operations verified.");
    return true;
}

// ==================== 设置管理 ====================

void DataManager::setFocusDuration(int minutes) {
    if (minutes < 1) minutes = 1;
    if (minutes > 120) minutes = 120; // 限制在1-120分钟
    bindingConfig.focusDuration = minutes;
    saveBindingConfig(); // 立即保存
}

int DataManager::getFocusDuration() {
    return bindingConfig.focusDuration > 0 ? bindingConfig.focusDuration : 25;
}

