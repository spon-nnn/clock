#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>
#include <LittleFS.h>

// 数据模型定义

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

    // 测试接口
    bool runSelfTest();

private:
    std::vector<ScheduleItem> schedules;
    std::vector<FocusRecord> focusRecords;
    
    const char* SCHEDULE_FILE = "/schedules_v2.json";
    const char* FOCUS_FILE = "/focus_v3.json"; // 升级文件版本以避免格式冲突

    void loadSchedules();
    void saveSchedules(); 
    
    void loadFocusRecords();
    void saveFocusRecords(); 

    // 辅助：生成唯一ID
    String generateId();
    
    // 辅助：全量去重
    void deduplicateSchedules();
};

extern DataManager DB;

#endif
