# 海图（Chart / ENC）支持跟进计划

目标：在现有「应用层 + SceneController」架构下，引入 S-57 等海图数据，作为 `LayerKind::Chart` 的可渲染路径，而不破坏当前影像/高程/矢量 MVP。

## S-57 / ENC 摄取策略

1. **数据源入口**  
   - 新增专用描述符（例如 `ChartSourceDescriptor`）或扩展 `DataSourceDescriptor`，明确 driver（如 `S57`）、数据集路径、交换集（CATALOG）、可选单元选择。  
   - 使用 **GDAL/OGR** 读取 S-57：OGR 对 `.000` 等文件提供矢量层；大测区可通过 VRT 或目录级打开策略控制内存与打开时间。

2. **与 `DataImporter` 的边界**  
   - 检测逻辑放在独立 `ChartDatasetInspector`（或 `GdalDatasetInspector` 的分支）中，返回 `DataSourceKind::Chart`（需在枚举中新增或与 `Vector` 区分）。  
   - 导入结果映射为 `ChartLayer`，`sourceUri` 可为文件路径或 OGR 连接串；复杂参数（物标筛选、比例尺）放在图层扩展属性或侧车 JSON 中，避免把所有选项塞进 URI。

3. **与 osgEarth 的对接**  
   - 首选：**OGRFeatureSource + FeatureModelLayer**（与当前矢量路径一致），样式表按 S-57 物标编码（OBJL）做初版规则。  
   - 备选：osgEarth 若提供专用海图层或第三方符号化库，再评估替换，以控制首版范围。

## 首版符号化范围（刻意收窄）

- 仅支持 **点/线/面** 的基础几何与有限 OBJL（如陆地、等深线、助航设施中的子集）。  
- **复杂符号**（灯塔灯质、扇形灯、禁区填充图案）首版可降级为简单点线面 + 文本注记，或仅几何高亮。  
- 提供 **单一默认样式表** + 可切换的「简绘 / 标准（受限）」两档，避免首版承担完整 IHO 符号库。

## 属性与查询

- 拾取命中时，将 OGR **要素属性** 序列化为 `PickResult::displayText`（或结构化 JSON 再在属性坞格式化）。  
- 属性坞需能展示 **物标类、属性表、关联 ACCOT** 等；与 `ApplicationController::showLayerDetails` 并行：图层元数据 vs 要素属性分两栏或 Tab。

## 兼容性与风险

| 风险 | 说明 | 缓解 |
|------|------|------|
| 投影与 datum | ENC 常为 WGS84；与当前球面墨卡托底图叠加需确认变换链 | 统一经 `OGRSpatialReference` 到场景 SRS；对异常 CRS 在导入阶段拒绝并提示 |
| 数据集体量 | 全港口交换集可能极大 | 单元过滤、按需加载、进度 UI |
| 许可与更新 | ENC 常受分发许可约束 | 文档与 UI 明确「用户自备数据」；不提供受版权保护的数据包 |
| GDAL/OGR 版本差异 | S-57 驱动行为随版本变化 | 锁定测试用 ENC 子集；CI 固定 GDAL 小版本 |
| 性能 | 密矢量海图拖慢帧率 | LOD、按视距简化、可选关闭海图层 |

## 建议里程碑

1. 只读打开 S-57 单元 → 描述符 + `ChartLayer` + 图层树（无符号或仅单色线）。  
2. OGR → FeatureModel + 按 OBJL 的样式表 v1。  
3. 拾取与属性坞联动。  
4. 符号与性能迭代（可拆分为后续 PR）。
