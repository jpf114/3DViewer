# 量测成果管理 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为量测成果补齐第一阶段管理能力，支持在图层树重命名成果，并在再次编辑与工程恢复后保留自定义名称。

**Architecture:** 复用现有图层树与控制器链路，在 `LayerTreeDock` 增加重命名入口和信号，在 `MainWindow` 透传，在 `ApplicationController` 更新图层名称；量测编辑时根据“是否仍是自动摘要名称”决定是否覆盖名称。

**Tech Stack:** C++20, Qt6 (QTreeWidget/QAction/itemChanged), 现有 MainWindow/ApplicationController/LayerTreeDock/Layer

---

### Task 1: 图层树重命名入口

**Files:**
- Modify: `src/ui/LayerTreeDock.h`
- Modify: `src/ui/LayerTreeDock.cpp`
- Test: `tests/app/ApplicationSmokeTests.cpp`

- [ ] **Step 1: 写失败测试**
- [ ] **Step 2: 运行 `ApplicationSmokeTests`，确认缺少重命名信号或动作**
- [ ] **Step 3: 增加右键菜单“重命名”动作与 `layerRenameRequested` 信号**
- [ ] **Step 4: 重新运行 `ApplicationSmokeTests`，确认转绿**

### Task 2: MainWindow 与 ApplicationController 接线

**Files:**
- Modify: `src/app/MainWindow.h`
- Modify: `src/app/MainWindow.cpp`
- Modify: `src/app/ApplicationController.h`
- Modify: `src/app/ApplicationController.cpp`
- Test: `tests/app/ApplicationSmokeTests.cpp`

- [ ] **Step 1: 写失败测试**
- [ ] **Step 2: 运行测试，确认控制器尚不能处理重命名**
- [ ] **Step 3: 透传重命名信号并实现 `renameLayer`**
- [ ] **Step 4: 重新运行测试**

### Task 3: 保留量测自定义名称

**Files:**
- Modify: `src/app/ApplicationController.cpp`
- Test: `tests/app/ApplicationSmokeTests.cpp`

- [ ] **Step 1: 写失败测试，覆盖“重命名后再次编辑仍保留名称”**
- [ ] **Step 2: 运行测试，确认当前会被自动摘要覆盖**
- [ ] **Step 3: 实现命名保留策略**
- [ ] **Step 4: 重新运行测试**

### Task 4: 全量验证与提交

**Files:**
- Modify: `docs/followups/2026-05-10-status-and-next-steps.md`

- [ ] **Step 1: 运行 `cmake --build build/debug --config Debug`**
- [ ] **Step 2: 运行 `ctest --test-dir build/debug -C Debug --output-on-failure`**
- [ ] **Step 3: 更新阶段文档**
- [ ] **Step 4: 中文提交**
