# 工程保存与打开 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 3DViewer 增加显式的工程保存、另存为、打开能力，使用单文件 `.3dproj`(JSON) 保存当前工程状态，并能在下次打开时恢复图层、底图、视角和量测成果。

**Architecture:** 新增 `ProjectConfig` 作为工程文件的数据模型与读写入口；`ApplicationController` 负责把应用层状态投影到 `ProjectConfig`，并在打开工程时按“清空当前工程 -> 恢复底图 -> 恢复图层/量测 -> 恢复相机”的顺序回放；`SceneController` 补齐当前底图与相机视角接口，`MainWindow` 补齐文件菜单工程动作。

**Tech Stack:** C++20, Qt6 (QJsonDocument/QFileDialog/QDir/QFileInfo), osgEarth EarthManipulator/Viewpoint, 现有 LayerManager/DataImporter/MainWindow/SceneController

---

## 文件结构

| 文件 | 职责 |
|------|------|
| `src/data/ProjectConfig.h` | 定义工程文件结构、相机状态、量测条目、图层条目，以及读写接口 |
| `src/data/ProjectConfig.cpp` | `.3dproj` 的 JSON 序列化、反序列化、相对路径处理 |
| `tests/data/ProjectConfigTests.cpp` | 工程文件 roundtrip 测试，覆盖底图、相机、普通图层、模型、量测 |
| `src/globe/SceneController.h/.cpp` | 暴露当前底图 id / 当前相机状态 / 应用相机状态 |
| `src/app/MainWindow.h/.cpp` | 文件菜单新增打开工程、保存工程、另存工程动作与信号 |
| `src/app/ApplicationController.h/.cpp` | 管理当前工程路径，保存/打开工程，对接菜单与状态恢复 |
| `tests/app/ApplicationSmokeTests.cpp` | 增加工程序列化/恢复最小冒烟验证 |
| `tests/CMakeLists.txt` | 新增 `ProjectConfigTests`，并为冒烟测试链接新模块 |
| `CMakeLists.txt` | 将 `ProjectConfig.cpp` 纳入主程序构建 |

---

### Task 1: 新增工程文件数据模型与读写测试

**Files:**
- Create: `src/data/ProjectConfig.h`
- Create: `src/data/ProjectConfig.cpp`
- Create: `tests/data/ProjectConfigTests.cpp`
- Modify: `tests/CMakeLists.txt`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: 先写 `ProjectConfig` 失败测试**

测试覆盖：
- `basemapId`
- `cameraState`
- 影像图层 `bandMapping`
- 模型图层 `modelPlacement`
- 量测图层 `measurementData`
- 保存相对路径、加载时按工程目录解析

- [ ] **Step 2: 运行测试确认失败**

Run: `cmake --build build --config Debug --target ProjectConfigTests`

Run: `ctest --test-dir build -C Debug -R ProjectConfigTests --output-on-failure`

Expected: 编译失败或测试失败，原因是 `ProjectConfig` 尚不存在。

- [ ] **Step 3: 实现最小 `ProjectConfig` 结构与 JSON 读写**

实现内容：
- `ProjectCameraState`
- `ProjectLayerEntry`
- `ProjectConfig`
- `loadProjectConfig(const QString &projectPath)`
- `saveProjectConfig(const QString &projectPath, const ProjectConfig &config)`

约束：
- 普通数据路径保存为相对工程目录的相对路径（可行时）
- 量测图层不依赖外部文件路径，直接保存量测点、长度、面积
- 结构中带 `version`

- [ ] **Step 4: 重新运行 `ProjectConfigTests`，确认转绿**

Run: `cmake --build build --config Debug --target ProjectConfigTests`

Run: `ctest --test-dir build -C Debug -R ProjectConfigTests --output-on-failure`

Expected: `ProjectConfigTests` PASS

- [ ] **Step 5: 提交一轮**

```bash
git add CMakeLists.txt tests/CMakeLists.txt tests/data/ProjectConfigTests.cpp src/data/ProjectConfig.h src/data/ProjectConfig.cpp
git commit -m "feat: 新增工程文件配置模型"
```

### Task 2: 扩展 SceneController 提供底图与相机状态接口

**Files:**
- Modify: `src/globe/SceneController.h`
- Modify: `src/globe/SceneController.cpp`
- Test: `tests/data/ProjectConfigTests.cpp`

- [ ] **Step 1: 写一个新的失败断言入口**

在现有测试或新增轻量断言中先假定存在：
- `currentBasemapId()`
- `currentCameraState()`
- `applyCameraState(...)`

- [ ] **Step 2: 运行相关测试确认失败**

Run: `cmake --build build --config Debug --target ApplicationSmokeTests`

Expected: 编译失败，缺少接口声明或定义。

- [ ] **Step 3: 最小实现 SceneController 状态接口**

实现内容：
- `Impl` 记录 `currentBasemapId`
- `setBasemapConfig(...)` 支持优先底图 id
- 通过 `osgEarth::EarthManipulator` 读取 / 应用 `Viewpoint`

建议状态字段：
- `longitude`
- `latitude`
- `altitude`
- `heading`
- `pitch`
- `range`

- [ ] **Step 4: 运行相关测试确认恢复**

Run: `cmake --build build --config Debug --target ApplicationSmokeTests`

Expected: 编译通过，若后续控制器功能未完成，可继续在后续任务中失败。

### Task 3: 扩展 MainWindow 文件菜单的工程动作

**Files:**
- Modify: `src/app/MainWindow.h`
- Modify: `src/app/MainWindow.cpp`
- Test: `tests/app/ApplicationSmokeTests.cpp`

- [ ] **Step 1: 先写失败测试**

验证：
- 文件菜单存在“打开工程”“保存工程”“另存工程”
- 保存快捷键为 `Ctrl+S`
- 动作触发时会发出对应信号

- [ ] **Step 2: 运行测试确认失败**

Run: `cmake --build build --config Debug --target ApplicationSmokeTests`

Expected: 测试失败，动作或信号尚不存在。

- [ ] **Step 3: 最小实现菜单动作与信号**

新增：
- `openProjectRequested()`
- `saveProjectRequested()`
- `saveProjectAsRequested()`

约束：
- 不抢占当前“导入数据”的 `Ctrl+O`
- “保存工程”使用 `Ctrl+S`
- `.3dproj` 过滤器保持统一

- [ ] **Step 4: 重新运行冒烟测试**

Run: `ctest --test-dir build -C Debug -R ApplicationSmokeTests --output-on-failure`

Expected: 工程菜单部分断言 PASS

### Task 4: 在 ApplicationController 中实现保存/打开工程

**Files:**
- Modify: `src/app/ApplicationController.h`
- Modify: `src/app/ApplicationController.cpp`
- Modify: `src/main.cpp`
- Test: `tests/app/ApplicationSmokeTests.cpp`

- [ ] **Step 1: 先写失败测试**

新增最小流程：
1. 构造一个带普通图层、模型图层、量测图层的场景
2. 保存到 `.3dproj`
3. 新建 controller/window/manager
4. 打开该工程
5. 断言图层数量、顺序、可见性、量测数据已恢复

- [ ] **Step 2: 运行测试确认失败**

Run: `cmake --build build --config Debug --target ApplicationSmokeTests`

Run: `ctest --test-dir build -C Debug -R ApplicationSmokeTests --output-on-failure`

Expected: 测试失败，缺少 `saveProject/openProject` 等能力。

- [ ] **Step 3: 实现最小保存工程能力**

新增：
- `resourceDir_`
- `currentProjectPath_`
- `saveProject(const QString &path)`
- `saveProjectWithDialog()`
- `saveProjectAsWithDialog()`

保存时：
- 遍历 `layerManager_`
- 普通图层记录路径、显隐、透明度
- 影像图层记录波段组合
- 模型记录 `modelPlacement`
- 量测图层记录 `measurementData`
- 从 `sceneController_` 读取底图 id、相机状态

- [ ] **Step 4: 实现最小打开工程能力**

新增：
- `openProject(const QString &path)`
- `openProjectWithDialog()`

打开顺序：
1. 读取 `ProjectConfig`
2. 清空当前图层与界面
3. 应用底图
4. 恢复普通图层
5. 恢复量测图层（写临时 geojson 并注入 `measurementData`）
6. 恢复相机视角
7. 更新 `currentProjectPath_`

- [ ] **Step 5: 在构造函数和 `main.cpp` 中接线**

连接：
- `MainWindow::openProjectRequested`
- `MainWindow::saveProjectRequested`
- `MainWindow::saveProjectAsRequested`

保留现有：
- 启动 `loadBasemapAndLayers(resourceDir)`
- 退出 `saveLayerConfigOnExit(resourceDir)`

- [ ] **Step 6: 重新运行冒烟测试**

Run: `cmake --build build --config Debug --target ApplicationSmokeTests`

Run: `ctest --test-dir build -C Debug -R ApplicationSmokeTests --output-on-failure`

Expected: `ApplicationSmokeTests` PASS

- [ ] **Step 7: 提交一轮**

```bash
git add src/app/ApplicationController.h src/app/ApplicationController.cpp src/app/MainWindow.h src/app/MainWindow.cpp src/globe/SceneController.h src/globe/SceneController.cpp src/main.cpp tests/app/ApplicationSmokeTests.cpp
git commit -m "feat: 支持工程保存与打开"
```

### Task 5: 全量验证与收尾

**Files:**
- Modify: `docs/followups/2026-05-10-status-and-next-steps.md`（如需要）

- [ ] **Step 1: 运行核心测试集**

Run: `cmake --build build --config Debug`

Run: `ctest --test-dir build -C Debug --output-on-failure`

Expected: 全部测试 PASS

- [ ] **Step 2: 运行 Release 构建与安装验证**

Run: `cmake --build build --config Release`

Run: `cmake --install build --config Release`

Expected: Release 构建与安装通过

- [ ] **Step 3: 更新阶段文档**

在跟进文档里补充：
- 工程文件格式
- 当前已支持恢复的状态
- 后续可继续扩展的状态（如图层样式、选择状态、分析结果）

- [ ] **Step 4: 最终提交**

```bash
git add docs/superpowers/plans/2026-05-10-project-save-open.md docs/followups/2026-05-10-status-and-next-steps.md
git commit -m "docs: 更新工程文件阶段计划"
```
