# 3DViewer 运行时架构

本文描述当前桌面端 3D 地球应用的组件边界、数据流与部署假设，与 `docs/2026-04-30-3dviewer-implementation-plan.md` 中的约束一致。

## 从导入到渲染的图层流

1. **文件选择**：`MainWindow` 通过菜单触发 `QFileDialog`，发出 `importDataRequested(path)`。
2. **导入与建模**：`ApplicationController::importFile` 调用 `DataImporter::import`；内部由 `GdalDatasetInspector` 打开数据集、分类（栅格影像 / 栅格高程 / 矢量），可选计算 WGS84 范围，构造 `DataSourceDescriptor`。
3. **应用层状态**：成功时创建具体 `Layer` 子类实例，写入 `LayerManager`（唯一真相源：可见性、顺序、元数据）。
4. **场景绑定**：同一路径上调用 `SceneController::addLayer`，将应用层映射为 `osgEarth::Layer`（GDAL 影像、GDAL 高程、OGR+FeatureModel 矢量等），并保存在 `SceneController` 内部的 `id → osgEarth::Layer` 映射中。
5. **UI 反馈**：`MainWindow::addLayerRow` 更新图层树；若存在有效地理范围则 `flyToGeographicBounds` 调整相机。
6. **持续同步**：图层树勾选变化经 `ApplicationController::setLayerVisibility` 更新 `LayerManager` 与 `SceneController::syncLayerState`。拖拽重排经 `reorderLayers` 与 `reorderUserLayers`，在 osgEarth `Map` 上从索引 1 起重排用户层（索引 0 为底图）。

**硬约束（与实现计划一致）**

- UI 类不直接创建或修改 osgEarth 图层对象。
- GDAL 检查/导入代码不直接改场景图。
- `LayerManager` 拥有应用可见的图层状态；`SceneController` 拥有「应用层 → osgEarth」的唯一桥接。

## 组件所有权

| 组件 | 职责 |
|------|------|
| `ApplicationController` | 连接信号：导入、图层选中、可见性、顺序、地图拾取；编排 `MainWindow`、`SceneController`、`LayerManager`、`DataImporter`。 |
| `MainWindow` | 窗口布局、菜单、停靠窗占位；转发子组件信号。 |
| `GlobeWidget` | Qt 原生窗口嵌入、帧循环、鼠标坐标翻转、将输入交给 `SceneController` 与 `ToolManager`；发出光标文本与地形拾取结果。 |
| `SceneController` | `osgEarth::Map` / `MapNode` / `Viewer`、底图、用户层映射、拾取、相机飞行、图层顺序。 |
| `LayerManager` | `std::vector<std::shared_ptr<Layer>>` 顺序与查询、`reorderLayers` / `moveLayer` / `setVisibility`。 |
| `DataImporter` / `GdalDatasetInspector` | 路径 → 描述符 → `Layer`；与场景无耦合。 |
| `ToolManager` | 当前 `MapTool` 分发（预留扩展）。 |
| `LayerTreeDock` / `PropertyDock` / `StatusBarController` | 纯展示与输入，不含业务 GIS 规则。 |

## 目录职责（`src/`）

- `app/`：应用级编排与主窗口。
- `ui/`：可停靠视图，通过信号与控制器通信。
- `globe/`：osgEarth/OSG 嵌入、场景、拾取数据结构。
- `layers/`：应用层类型、`LayerManager`、地理范围等共享模型。
- `data/`：GDAL 数据源探测与导入。
- `tools/`：地图工具抽象与实现。
- `common/`：日志等横切能力。

## 延后扩展点：海图（Chart）与科学网格（Scientific）

- `LayerKind` 已包含 `Chart`、`Scientific`；`SceneController::createSceneLayer` 对这两类返回空实现，避免在 MVP 中误接渲染路径。
- 后续应在 **独立设计文档**（见 `docs/followups/`）中定义数据源、符号化与 UI 交互，再增加 `SceneController` 分支与专用导入器，避免在 `DataImporter` 中一次性堆叠所有格式。

## 部署假设（vcpkg + 本地拷贝）

- 构建依赖通过 CMake `find_package` 解析（Qt6、OpenSceneGraph、osgEarth、GDAL、PROJ、GEOS、spdlog）。
- **开发/发布机**上建议使用全局或团队统一的 **vcpkg 前缀**，并通过 `THREEDVIEWER_RUNTIME_ROOT` 将 `share/gdal`、`share/proj` 与 `osgPlugins-*` 拷到可执行文件旁或安装树；详见 `docs/notes/runtime-deployment.md`。
- `windeployqt`（若可用）负责 Qt 运行库与 `platforms`；未找到时回退为仅复制 `qwindows.dll`。
- 干净环境运行需正确设置或拷贝：**GDAL_DATA**、**PROJ** 数据路径，以及 OSG 插件搜索路径（与 exe 相对位置或环境变量）。

## 与测试的对应关系

- `tests/`：图层管理、场景控制器烟测、GDAL 检查与导入、应用壳等可自动化部分。
- 渲染与部署变更仍建议在本机做一次手动启动验证。
