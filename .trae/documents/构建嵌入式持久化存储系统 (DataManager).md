这是一个针对 ESP8266 嵌入式环境的轻量级但可靠的持久化存储系统设计方案。考虑到硬件资源限制（4MB Flash, ~80KB RAM），我们将使用 **LittleFS 文件系统 + JSON 序列化** 的方案，并引入 **原子写入** 机制来确保数据可靠性。

### 1. 系统架构设计

我们将把数据管理逻辑从 `main.cpp` 中剥离出来，创建一个独立的 `DataManager` 模块。

#### **1.1 存储技术选择**
*   **存储引擎**: `LittleFS` (已集成，适合掉电保护)
*   **数据格式**: `JSON` (ArduinoJson 库，易于与 Web 前端交互)
*   **可靠性机制**: **原子写操作** (Atomic Write) —— 先写入 `.tmp` 临时文件，写入成功且校验通过后，再重命名覆盖原文件，防止写入过程中断电导致文件损坏。

#### **1.2 数据模型设计 (Schema)**

**A. 日程表 (Schedule)**
文件: `/data/schedules.json`
```json
[
  {
    "id": "1715623400",       // 唯一ID (使用时间戳)
    "date": "2024-05-14",     // 日期
    "time": "14:00",          // 开始时间
    "content": "写代码",       // 事项内容
    "category": "work",       // 标签: work, life, study
    "isDone": false           // 状态
  }
]
```

**B. 专注记录 (FocusRecord)**
文件: `/data/focus.json`
```json
[
  {
    "id": "1715628800",
    "startTime": 1715628800,  // Unix时间戳
    "duration": 25,           // 持续分钟
    "taskId": "1715623400",   // 关联的日程ID
    "interruptions": 0        // 中断次数
  }
]
```

### 2. 功能实现计划

我们将创建 `src/DataManager.h` 和 `src/DataManager.cpp`。

#### **核心功能**
1.  **CRUD 接口**:
    *   `addSchedule(ScheduleItem item)`
    *   `deleteSchedule(String id)`
    *   `updateSchedule(String id, ScheduleItem newItem)`
    *   `getSchedules()`
    *   *专注记录同理*
2.  **数据校验**:
    *   输入字符串长度限制（防止内存溢出）。
    *   JSON 格式合法性检查。
3.  **可靠性 (原子保存)**:
    *   `saveToFile()`: Write to `filename.tmp` -> Check Size -> `LittleFS.rename()`.
4.  **Web API 升级**:
    *   升级 `/api/schedule` 支持复杂的 JSON 对象传输。
    *   新增 `/api/focus` 用于同步专注数据。

### 3. 测试与验证
*   **单元测试**: 编写 `runStorageTests()` 函数，在启动时运行，模拟增删改查操作并验证结果。
*   **异常恢复**: 模拟加载损坏的 JSON 文件，确保系统能自动重置或忽略错误，而不是崩溃。

### 4. 实施步骤
1.  **创建 `src/DataManager.h/cpp`**: 实现数据结构定义和文件操作逻辑。
2.  **重构 `main.cpp`**: 替换原有的 `std::vector<String> scheduleList`，改用新的 `DataManager`。
3.  **更新 Web 界面**: 修改 `index_html`，增加日期/时间选择器和分类下拉框。
4.  **集成测试**: 烧录固件，通过串口观察测试报告，并通过 Web 界面验证功能。
