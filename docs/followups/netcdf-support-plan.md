# NetCDF / 科学栅格支持跟进计划

目标：将多维科学数据（NetCDF、GRIB 等）纳入 `LayerKind::Scientific` 管线，首版以 **二维栅格叠加** 为主，推迟体渲染与粒子效果。

## 变量与时间步选择

1. **元数据阶段**（导入前或导入向导）  
   - 使用 GDAL 子数据集 API 或 NetCDF C 库列举 **变量、维度、坐标变量**（时间、高度、lat/lon）。  
   - UI：最小实现为 **模态对话框** 或属性坞内嵌表单：选择变量、单时间索引或时间范围、（可选）垂直层。

2. **与描述符的映射**  
   - `DataSourceDescriptor`（或 `ScientificSourceDescriptor`）保存：`path`、`subdataset`、`varName`、`timeIndex` 或 `timeValue`、`bandMapping` 等。  
   - `sourceUri` 可编码为内部 URI（如 `netcdf:path:var#time=3`），或仅存路径 + 侧车 JSON；**避免**在 GDAL 打开字符串中堆积不可见字符。

## 首版渲染：栅格化 2D 叠加

- 将选定 **(time, z)** 切片导出或虚拟为 **单波段/多波段 GDAL 数据集**，复用现有 **GDALImageLayer** 或内存 VRT 路径接入 `SceneController`（仍通过 `ScientificLayer` 应用类型，便于图层树分类）。  
- 若 osgEarth 对内存 GDAL 支持良好，可优先 **VRT + GDALImageLayer**；否则生成临时 GeoTIFF/COG（注意磁盘与清理策略）。

## 延后：体数据与粒子 / 流线

- **体渲染**（3D 纹理、体积云/洋流柱）需要独立渲染通道、着色器与裁剪体，与当前 `MapNode` 地形管线解耦；建议在 `ScientificLayer` 下增加 `ScientificRenderMode` 枚举，默认 `Overlay2D`。  
- **粒子/流线**：依赖时间序列与矢量场重采样，属于第二阶段；需单独性能预算与 UI（播放、速度）。

## 测试数据策略

1. **仓库内**：仅放置 **极小** 许可友好的示例 NetCDF（或生成脚本在 CI 中生成 synthetic 网格），避免大二进制进 Git。  
2. **本地/CI 外**：文档列出推荐公开数据集（名称、下载地址、变量名），供开发者手动回归。  
3. **自动化测试**：以 GDAL 只读打开 + 描述符字段断言为主；完整渲染仍保留一条 **手动** 烟测清单。

## 风险与依赖

- GDAL 需启用 **netCDF 驱动**；Windows 上 vcpkg feature 需显式打开。  
- **CF conventions** 与坐标变量命名多样，首版可只支持 **明确 lon/lat/time** 的常见布局，其余走「高级模式」手工指定维度映射。  
- 时间序列播放与磁盘缓存会触及应用状态机设计，宜在 2D 稳定后再做。

## 建议里程碑

1. 检测 `.nc` → 描述符 + `ScientificLayer`（无渲染，仅占位）。  
2. 单时间、单变量、2D lat/lon → VRT/GDAL 影像层接入场景。  
3. 简单时间步切换（无动画或仅手动步进）。  
4. 再评估体/流线与缓存架构。
