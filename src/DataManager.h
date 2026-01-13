#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>
#include <LittleFS.h>

// 数据模型定义

// 0. 设备绑定配置
struct BindingConfig {
    String deviceId;      // 设备唯一ID (基于芯片ID)
    String bindToken;     // 绑定Token (随机生成，用于验证)
    bool isBound;         // 是否已绑定
    String boundUserId;   // 绑定的微信用户OpenID
    String boundNickname; // 绑定用户昵称
    unsigned long bindTime; // 绑定时间戳
    int focusDuration = 25; // 专注时长（分钟），默认25
};

// 1. 日程项
struct ScheduleItem {
    String id;          // 唯一ID (时间戳)
    String date;        // 日期 "YYYY-MM-DD"
    String startTime;   // 开始时间 "HH:MM"
    String endTime;     // 结束时间 "HH:MM"
    String content;     // 内容
    String category;    // 分类: "work", "life", "study"
    bool isDone;        // 完成状态

    // 转换为 JSON 对象
    JsonObject toJson(JsonDocument& doc) const {
        JsonObject obj = doc.createNestedObject();
        obj["id"] = id;
        obj["date"] = date;
        obj["startTime"] = startTime;
        obj["endTime"] = endTime;
        obj["content"] = content;
        obj["category"] = category;
        obj["isDone"] = isDone;
        return obj;
    }

    // 从 JSON 对象解析
    static ScheduleItem fromJson(JsonObject obj) {
        ScheduleItem item;
        item.id = obj["id"].as<String>();
        item.date = obj["date"].as<String>();
        item.startTime = obj["startTime"].as<String>();
        item.endTime = obj["endTime"].as<String>();
        item.content = obj["content"].as<String>();
        item.category = obj["category"].as<String>();
        item.isDone = obj["isDone"].as<bool>();
        return item;
    }
};

// 2. 专注记录
struct FocusRecord {
    String id;
    unsigned long startTime; // Unix 时间戳
    unsigned long endTime;   // Unix 时间戳 (新增)
    int duration;            // 分钟
    String taskId;           // 关联的日程ID
    String note;             // 备注 (新增)
    int interruptions;       // 中断次数

    JsonObject toJson(JsonDocument& doc) const {
        JsonObject obj = doc.createNestedObject();
        obj["id"] = id;
        obj["startTime"] = startTime;
        obj["endTime"] = endTime;
        obj["duration"] = duration;
        obj["taskId"] = taskId;
        obj["note"] = note;
        obj["interruptions"] = interruptions;
        return obj;
    }

    static FocusRecord fromJson(JsonObject obj) {
        FocusRecord rec;
        rec.id = obj["id"].as<String>();
        rec.startTime = obj["startTime"].as<unsigned long>();
        rec.endTime = obj["endTime"].as<unsigned long>();
        rec.duration = obj["duration"].as<int>();
        rec.taskId = obj["taskId"].as<String>();
        rec.note = obj["note"].as<String>();
        rec.interruptions = obj["interruptions"].as<int>();
        return rec;
    }
};

class DataManager {
public:
    DataManager();
    void begin();

    // === 设备绑定管理 ===
    BindingConfig& getBindingConfig();
    void initDeviceId();           // 初始化设备ID (基于芯片ID)
    void generateBindToken();      // 生成新的绑定Token
    bool bindDevice(String userId, String nickname); // 绑定设备到用户
    void unbindDevice();           // 解绑设备
    void factoryReset();           // 恢复出厂设置
    String getQRCodeContent();     // 获取二维码内容JSON
    bool verifyBindToken(String token); // 验证绑定Token

    // 日程操作
    void addSchedule(ScheduleItem item);
    void deleteSchedule(String id);
    void updateSchedule(String id, ScheduleItem item);
    std::vector<ScheduleItem>& getSchedules();
    
    // 专注记录操作
    void addFocusRecord(FocusRecord record);
    void updateFocusRecord(String id, String note); // 新增：仅更新备注
    void deleteFocusRecord(String id);              // 新增：删除记录
    std::vector<FocusRecord>& getFocusRecords();

    // 统计数据
    int getTodayFocusMinutes();
    int getTotalFocusMinutes();

    // 设置管理
    void setFocusDuration(int minutes);
    int getFocusDuration();

    // 测试接口
    bool runSelfTest();

private:
    BindingConfig bindingConfig;
    std::vector<ScheduleItem> schedules;
    std::vector<FocusRecord> focusRecords;

    const char* BINDING_FILE = "/binding.json";
    const char* SCHEDULE_FILE = "/schedule.json";
    const char* FOCUS_FILE = "/focus.json";

    void loadBindingConfig();
    void saveBindingConfig();
    void loadSchedules();
    void saveSchedules(); 
    void loadFocusRecords();
    void saveFocusRecords();

    // 内部辅助函数
    String generateId();
    void deduplicateSchedules();
};

extern DataManager DB;

#endif
