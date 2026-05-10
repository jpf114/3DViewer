# 3DViewer 易用性与界面优化设计

## 目标

在现有架构上渐进式优化，专注于影像和矢量两种数据类型，不引入新功能。三个方向按优先级推进：易用性 > 实用性 > 界面样式。

## 视觉风格

混合风格：地球渲染区域保持深色（OSG 默认），面板区域使用浅灰色调，工具栏和菜单使用中性色调。

## 图标系统

参考 GIS_TOOL 项目（`D:\Develop\GIS\GIS_TOOL`）的图标方案，使用 Phosphor Icons 风格的 SVG 图标。

### 图标来源

从 GIS_TOOL 的 `resources/icons/regular/` 目录复制所需 SVG 文件到 3DViewer 的 `resources/icons/regular/` 目录。

### 图标部署（参考 GIS_TOOL 模式）

GIS_TOOL 的图标部署流程：
1. **源码目录**：`resources/icons/` 存放 SVG 文件（项目自带，非 vcpkg 安装）
2. **构建时**：CMake POST_BUILD 将 `resources/icons/` 复制到 `$<TARGET_FILE_DIR>/icons/`
3. **安装时**：`install(DIRECTORY resources/icons/ DESTINATION share/icons)`
4. **运行时**：`IconManager` 从 exe 目录查找 `icons/` 子目录，或回退到 `../share/icons/`
5. **依赖**：需要 `Qt6::Svg` 模块（`QSvgRenderer` 渲染 SVG）

3DViewer 采用相同模式：
- CMakeLists.txt 添加 `find_package(Qt6 COMPONENTS Svg)` 和 `target_link_libraries(... Qt6::Svg)`
- POST_BUILD 复制 `resources/icons/` → `$<TARGET_FILE_DIR>/icons/`
- install 复制 `resources/icons/` → `share/icons/`
- main.cpp 中 `IconManager::instance().setIconsBasePath(iconsDir)`

### 图标映射

| 功能 | SVG 文件 | 来源目录 |
|------|---------|---------|
| 平移工具 | `arrows-out-regular.svg` | regular |
| 拾取工具 | `crosshair-regular.svg` | regular |
| 归位 | `arrows-clockwise-regular.svg` | regular |
| 截图 | `copy-regular.svg` | regular |
| 影像图层 | `mountains-regular.svg` | regular |
| 矢量图层 | `polygon-regular.svg` | regular |
| 导入数据 | `cloud-arrow-up-regular.svg` | regular |
| 图层可见 | `eye-regular.svg` | regular |
| 删除图层 | `minus-circle-regular.svg` | regular |
| 缩放到图层 | `magnifying-glass-plus-regular.svg` | regular |
| 信息 | `info-regular.svg` | regular |
| 波段组合 | `palette-regular.svg` | regular |
| 不透明度 | `drop-half-regular.svg` | regular |

### IconManager 简化实现

参考 GIS_TOOL 的 `src/gui/icon_manager.h/cpp`，实现一个简化版本：

- 单例模式，缓存已渲染的 QPixmap（`std::map<string, QPixmap> cache_`）
- `setIconsBasePath(path)` 设置图标根目录
- `icon(name, size, color)` 返回 QIcon
- SVG 渲染：读取 SVG 文件 → 替换 `currentColor` 为指定颜色 → `QSvgRenderer` 渲染为 `QPixmap`
- 默认颜色：工具栏图标 `#4F637A`（深灰蓝），图层图标 `#2E7EC9`（主题蓝）

## 第一批：易用性改进

### 1.1 快捷键支持

| 快捷键 | 功能 |
|--------|------|
| Ctrl+O | 导入数据 |
| Ctrl+Shift+H | 视图归位（Home） |
| 1 | 切换到平移工具 |
| 2 | 切换到拾取工具 |
| Delete | 删除选中图层 |

实现方式：在 MainWindow 中重写 `keyPressEvent`，根据按键组合发射对应信号。

### 1.2 视图归位

- 工具栏添加"归位"按钮（SVG 图标 + 文字）
- 功能：将相机重置到初始全球视角（经度 0，纬度 0，高度 3 倍地球半径）
- SceneController 新增 `resetView()` 方法
- 快捷键 Ctrl+Shift+H

### 1.3 图层缩放到范围

- 图层树右键菜单添加"缩放到图层"选项
- 双击图层项也触发缩放
- 复用已有的 `SceneController::flyToGeographicBounds()`
- 新增信号 `zoomToLayerRequested(layerId)`

### 1.4 属性面板结构化

将 PropertyDock 中的 QTextEdit 纯文本替换为分组显示：

- **基本信息组**（QGroupBox）：名称、类型、来源、可见性（QFormLayout）
- **影像信息组**（QGroupBox，仅影像图层）：尺寸、波段数、坐标系、像素大小（QFormLayout）
- **矢量信息组**（QGroupBox，仅矢量图层）：几何类型、要素数、坐标系（QFormLayout）
- **波段详情组**（QGroupBox，仅影像图层）：QTableWidget 显示每个波段的类型、范围、NoData
- **字段列表组**（QGroupBox，仅矢量图层）：QTableWidget 显示字段名和类型

保留不透明度滑块和波段组合下拉框。

### 1.5 图层类型图标

- 影像图层：`mountains-regular.svg`（山脉图标）
- 矢量图层：`polygon-regular.svg`（多边形图标）
- 在 QTreeWidgetItem 上设置图标（`setIcon(0, icon)`）

## 第二批：实用性改进

### 2.1 最近打开文件

- 文件菜单底部显示最近 5 个打开的文件路径
- 使用 QSettings 持久化存储
- 点击直接导入，无需重新浏览

### 2.2 导入进度反馈

- 大文件导入时在状态栏显示旋转等待指示器
- 使用 QFutureWatcher + QtConcurrent 实现异步导入
- 导入完成后状态栏显示完成消息

### 2.3 视图截图

- 工具栏添加"截图"按钮（`copy-regular.svg` 图标）
- 使用 OSG 的 `ScreenCaptureHandler` 截取当前帧
- 弹出文件保存对话框，默认 PNG 格式
- 快捷键 Ctrl+Shift+S

## 第三批：界面样式

### 3.1 QSS 样式表

参考 GIS_TOOL 的 `style_constants.h`，采用一致的配色方案：

| 用途 | 色值 | 说明 |
|------|------|------|
| 窗口背景 | #E8ECF1 | 浅灰蓝 |
| 面板背景 | #F5F6F8 | 更浅灰 |
| 卡片/表单背景 | #FFFFFF | 白色 |
| 卡片边框 | #E2E5EA | 浅灰 |
| 主题色 | #2E7EC9 | 蓝色 |
| 主题色浅 | #E8F2FB | 浅蓝 |
| 主文字 | #15253A | 深色 |
| 次文字 | #4F637A | 灰蓝 |
| 弱文字 | #7E92A8 | 浅灰蓝 |
| 分隔线 | #E3EAF2 | 极浅灰 |
| 输入框边框 | #D3DDE8 | 浅灰蓝 |
| 输入框聚焦 | #2F7CF6 | 蓝色 |

混合风格样式表：

```css
QMainWindow { background: #E8ECF1; }
QMenuBar { background: #F5F6F8; border-bottom: 1px solid #E2E5EA; }
QMenuBar::item:selected { background: #E8F2FB; color: #2E7EC9; }
QToolBar { background: #F5F6F8; border-bottom: 1px solid #E2E5EA; spacing: 4px; padding: 2px; }
QToolBar QToolButton { padding: 4px 8px; border-radius: 4px; }
QToolBar QToolButton:checked { background: #E8F2FB; color: #2E7EC9; }
QToolBar QToolButton:hover { background: #E8F2FB; }
QDockWidget::title { background: #F5F6F8; padding: 6px; border-bottom: 1px solid #E2E5EA; }
QGroupBox { font-weight: bold; border: 1px solid #E2E5EA; border-radius: 6px; margin-top: 12px; padding-top: 18px; color: #15253A; }
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }
QStatusBar { background: #FFFFFF; border-top: 1px solid #E2E5EA; color: #4F637A; font-size: 12px; }
QTreeWidget { background: #FFFFFF; border: 1px solid #E2E5EA; border-radius: 4px; }
QTreeWidget::item:selected { background: #E8F2FB; color: #2E7EC9; }
QTreeWidget::item:hover { background: #F5F6F8; }
QSlider::groove:horizontal { height: 6px; background: #E3EAF2; border-radius: 3px; }
QSlider::handle:horizontal { width: 16px; height: 16px; background: #2E7EC9; border-radius: 8px; margin: -5px 0; }
QSlider::handle:horizontal:hover { background: #3590DA; }
QComboBox { border: 1px solid #D3DDE8; border-radius: 4px; padding: 4px 8px; background: #FDFEFF; }
QComboBox:focus { border-color: #2F7CF6; }
QTableWidget { background: #FFFFFF; border: 1px solid #E2E5EA; gridline-color: #E3EAF2; }
QTableWidget::item { padding: 4px; }
QHeaderView::section { background: #F5F6F8; border: 1px solid #E2E5EA; padding: 4px 8px; font-weight: bold; color: #4F637A; }
QScrollBar:vertical { background: transparent; width: 8px; }
QScrollBar::handle:vertical { background: #CBD6E2; border-radius: 4px; min-height: 28px; }
QScrollBar::handle:vertical:hover { background: #AFBFCE; }
```

### 3.2 属性面板美化

- 使用 QGroupBox 分组，每组有标题和边框
- 标签名使用粗体，颜色 `#4F637A`
- 数值颜色 `#15253A`
- 波段和字段信息使用 QTableWidget，有表头和交替行色
- 交替行色：`#FFFFFF` / `#F5F6F8`

## 不做的事情

- 不添加海图/NetCDF/高程数据支持
- 不添加矢量编辑功能
- 不添加打印/导出地图功能
- 不添加插件系统
- 不修改 osgEarth 渲染管线

## 文件变更预估

| 文件 | 变更类型 |
|------|---------|
| MainWindow.h/cpp | 新增快捷键、归位按钮、截图按钮、最近文件菜单 |
| ApplicationController.h/cpp | 新增 resetView、zoomToLayer、screenshot 方法 |
| PropertyDock.h/cpp | 重写为分组结构化面板 |
| LayerTreeDock.h/cpp | 新增缩放到图层、双击、类型图标 |
| SceneController.h/cpp | 新增 resetView、screenshot 方法 |
| StatusBarController.h/cpp | 新增进度指示器 |
| ui/IconManager.h/cpp | 新增图标管理器（参考 GIS_TOOL） |
| resources/icons/*.svg | 新增 SVG 图标文件（从 GIS_TOOL 复制） |
| resources/style.qss | 新增 QSS 样式表 |
| CMakeLists.txt | 添加图标和样式资源 |
