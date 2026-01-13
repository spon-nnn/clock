我将为该项目进行功能扩展，实现多页面切换（天气、日程、番茄钟），并添加Web端控制和物理按键支持。

### 1. 项目配置更新 (`platformio.ini`)
- **添加依赖库**:
  - `mathertel/OneButton`: 用于方便地处理物理按键（单击切换页面、长按执行操作）。
  - `ottowinter/ESPAsyncWebServer-esphome`: 用于创建一个响应迅速的Web界面，不会导致时钟卡顿。
  - `esphome/ESPAsyncTCP-esphome`: Web服务器所需的底层库。

### 2. 核心逻辑重构 (`src/main.cpp`)
- **非阻塞架构**: 
  - 重写 `imgAnim()`（太空人动画），使用 `millis()` 替代 `delay()`。这能确保即使在播放动画时，按键也能立即响应。
  - 实现一个 **状态机** 来管理三个页面：`PAGE_WEATHER` (天气), `PAGE_SCHEDULE` (日程), `PAGE_POMODORO` (番茄钟)。
- **按键处理**:
  - 配置 D1 引脚 (GPIO 5) 使用 `OneButton` 库。
  - **单击**: 在三个页面之间循环切换。
  - **长按**: 在番茄钟页面时，控制计时的 开始/暂停。

### 3. 新功能实现
- **第二页：日程计划**
  - 使用 `LittleFS` (文件系统) 存储 `schedule.json` 文件。
  - 在屏幕上显示前 3-4 条待办事项。
- **第三页：番茄钟**
  - 实现 25 分钟倒计时逻辑。
  - 显示“专注中”或“休息”状态。
  - 记录完成的专注时长到 `pomodoro.json`。
- **Web 管理界面**:
  - 在代码中嵌入一个轻量级的 HTML 控制台。
  - **日程管理**: 可以添加或删除待办事项，实时同步到屏幕。
  - **番茄钟统计**: 查看今日及历史专注总时长。

### 4. 实施步骤
1.  **修改 `platformio.ini`**：加入新库的依赖。
2.  **重构 `main.cpp`**：
    - 添加 `setupWebserver()` 和 `initFS()` 初始化函数。
    - 实现 `renderSchedulePage()` 和 `renderPomodoroPage()` 渲染逻辑。
    - 更新 `loop()` 以处理页面切换和非阻塞渲染。
3.  **验证**：编译代码，确保原有的天气时钟功能正常，并测试新功能的切换和Web控制。
