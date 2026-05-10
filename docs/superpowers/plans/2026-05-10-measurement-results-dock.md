# 量测成果工作台 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 3DViewer 新增独立的量测成果工作台停靠面板，支持列表展示、选择联动、单条操作、批量删除和批量导出。

**Architecture:** 新增 `MeasurementResultsDock` 作为纯 UI 工作台，真实数据仍由 `LayerManager` 持有，动作仍由 `ApplicationController` 落地。`MainWindow` 负责挂载工作台并透传信号，同时在图层变化和选择变化时同步工作台列表与选中状态。

**Tech Stack:** C++20, Qt6 (`QDockWidget`, `QTableWidget`, `QToolBar`, `QAction`), 现有 `MainWindow` / `ApplicationController` / `LayerManager` / `Layer`

---

### Task 1: 新建量测成果工作台 UI

**Files:**
- Create: `src/ui/MeasurementResultsDock.h`
- Create: `src/ui/MeasurementResultsDock.cpp`
- Test: `tests/app/ApplicationSmokeTests.cpp`

- [ ] **Step 1: 写失败测试，确认主窗口中存在量测成果工作台**
- [ ] **Step 2: 运行 `ApplicationSmokeTests`，确认当前缺少工作台**
- [ ] **Step 3: 新建 `MeasurementResultsDock`，提供量测条目刷新、添加、更新、删除、选中同步接口**
- [ ] **Step 4: 为工作台增加表格展示和基础动作按钮，支持单选/多选**
- [ ] **Step 5: 重新运行 `ApplicationSmokeTests`，确认工作台可被主窗口发现**

### Task 2: MainWindow 挂载工作台并接通信号

**Files:**
- Modify: `src/app/MainWindow.h`
- Modify: `src/app/MainWindow.cpp`
- Modify: `src/ui/CMakeLists.txt` or project source list if needed
- Test: `tests/app/ApplicationSmokeTests.cpp`

- [ ] **Step 1: 写失败测试，确认工作台存在并暴露批量删除、批量导出等动作对象名**
- [ ] **Step 2: 运行 `ApplicationSmokeTests`，确认动作尚不存在**
- [ ] **Step 3: 在 `MainWindow` 中创建并挂载 `MeasurementResultsDock`**
- [ ] **Step 4: 新增工作台相关信号透传：选择量测、定位、编辑、删除、批量删除、批量导出**
- [ ] **Step 5: 将工作台动作启用状态与当前量测选择数量联动**
- [ ] **Step 6: 重新运行 `ApplicationSmokeTests`，确认动作和工作台存在**

### Task 3: 同步量测图层到工作台列表

**Files:**
- Modify: `src/app/MainWindow.h`
- Modify: `src/app/MainWindow.cpp`
- Modify: `src/app/ApplicationController.h`
- Modify: `src/app/ApplicationController.cpp`
- Modify: `src/ui/MeasurementResultsDock.h`
- Modify: `src/ui/MeasurementResultsDock.cpp`
- Test: `tests/app/ApplicationSmokeTests.cpp`

- [ ] **Step 1: 写失败测试，确认新增量测图层后工作台出现对应条目**
- [ ] **Step 2: 运行 `ApplicationSmokeTests`，确认当前列表不同步**
- [ ] **Step 3: 在 `MainWindow` 增加工作台条目维护接口，只同步 `LayerKind::Measurement`**
- [ ] **Step 4: 在 `ApplicationController` 的量测新增、重命名、删除链路中同步工作台**
- [ ] **Step 5: 将量测类型和摘要值整理为统一的展示文本**
- [ ] **Step 6: 重新运行 `ApplicationSmokeTests`，确认工作台列表正确显示量测成果**

### Task 4: 打通选择联动与单条操作

**Files:**
- Modify: `src/app/MainWindow.h`
- Modify: `src/app/MainWindow.cpp`
- Modify: `src/app/ApplicationController.cpp`
- Modify: `src/ui/MeasurementResultsDock.h`
- Modify: `src/ui/MeasurementResultsDock.cpp`
- Test: `tests/app/ApplicationSmokeTests.cpp`

- [ ] **Step 1: 写失败测试，确认工作台选中量测会联动当前图层选择**
- [ ] **Step 2: 运行 `ApplicationSmokeTests`，确认当前没有联动**
- [ ] **Step 3: 工作台发出选中量测信号，`MainWindow` 选择对应图层并展示属性**
- [ ] **Step 4: 图层树或控制器切换当前量测时，回写工作台当前项**
- [ ] **Step 5: 复用现有编辑、导出、删除、定位逻辑接通工作台单条操作**
- [ ] **Step 6: 重新运行 `ApplicationSmokeTests`，确认联动和单条动作可用**

### Task 5: 实现批量删除

**Files:**
- Modify: `src/app/MainWindow.h`
- Modify: `src/app/MainWindow.cpp`
- Modify: `src/app/ApplicationController.h`
- Modify: `src/app/ApplicationController.cpp`
- Modify: `src/ui/MeasurementResultsDock.h`
- Modify: `src/ui/MeasurementResultsDock.cpp`
- Test: `tests/app/ApplicationSmokeTests.cpp`

- [ ] **Step 1: 写失败测试，确认工作台多选后可批量删除且保留非量测图层**
- [ ] **Step 2: 运行 `ApplicationSmokeTests`，确认当前没有批量删除链路**
- [ ] **Step 3: 工作台暴露选中量测 id 列表**
- [ ] **Step 4: `MainWindow` 新增批量删除信号透传**
- [ ] **Step 5: `ApplicationController` 新增按 id 列表批量删除量测逻辑，并复用现有 `removeLayer`**
- [ ] **Step 6: 重新运行 `ApplicationSmokeTests`，确认只删除选中的量测成果**

### Task 6: 实现批量导出

**Files:**
- Modify: `src/app/MainWindow.h`
- Modify: `src/app/MainWindow.cpp`
- Modify: `src/app/ApplicationController.h`
- Modify: `src/app/ApplicationController.cpp`
- Modify: `src/ui/MeasurementResultsDock.h`
- Modify: `src/ui/MeasurementResultsDock.cpp`
- Test: `tests/app/ApplicationSmokeTests.cpp`

- [ ] **Step 1: 写失败测试，确认工作台可发出批量导出请求**
- [ ] **Step 2: 运行 `ApplicationSmokeTests`，确认当前缺少批量导出链路**
- [ ] **Step 3: 在控制器中新增“选择目录后逐条导出 GeoJSON”的批量导出实现**
- [ ] **Step 4: 为导出文件名增加稳定安全清洗和重名去重**
- [ ] **Step 5: 通过状态栏反馈导出成功数与失败数**
- [ ] **Step 6: 重新运行 `ApplicationSmokeTests`，确认批量导出动作和基础成功路径成立**

### Task 7: 全量验证、文档与提交

**Files:**
- Modify: `docs/followups/2026-05-10-status-and-next-steps.md`

- [ ] **Step 1: 运行 `cmake --build build/debug --config Debug`**
- [ ] **Step 2: 运行 `ctest --test-dir build/debug -C Debug --output-on-failure`**
- [ ] **Step 3: 更新阶段状态文档，记录量测成果工作台第一版**
- [ ] **Step 4: 用中文提交本轮实现**
