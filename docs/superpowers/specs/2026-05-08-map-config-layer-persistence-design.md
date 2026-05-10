# 3DViewer 地图配置与图层持久化设计

## 目标

将硬编码的底图加载和临时的图层管理改为配置驱动，实现：

1. 底图可配置：支持本地文件（GDAL）、在线切片（TMS）、WMS、WMTS、ArcGIS REST、Bing 等多种底图源
2. 图层持久化：程序退出时保存当前图层列表，下次启动自动加载

## 配置文件

### basemap.json — 底图配置

位置：`data/basemap.json`（相对于 exe 目录）

```json
{
  "description": "3DViewer 底图配置 - 启动时加载第一个 enabled=true 的底图",
  "basemaps": [
    {
      "id": "natural_earth",
      "name": "Natural Earth",
      "type": "gdal",
      "enabled": true,
      "path": "image/NE1_LR_LC_SR_W.tif"
    },
    {
      "id": "osm_tiles",
      "name": "OpenStreetMap",
      "type": "tms",
      "enabled": false,
      "url": "https://tile.openstreetmap.org/{z}/{x}/{y}.png",
      "profile": "spherical-mercator"
    },
    {
      "id": "tdt_img",
      "name": "天地图影像",
      "type": "tms",
      "enabled": false,
      "url": "https://t{s}.tianditu.gov.cn/img_w/wmts?SERVICE=WMTS&REQUEST=GetTile&VERSION=1.0.0&LAYER=img&STYLE=default&TILEMATRIXSET=w&FORMAT=tiles&TILECOL={x}&TILEROW={y}&TILEMATRIX={z}&tk={token}",
      "subdomains": "01234567",
      "token": "",
      "profile": "spherical-mercator"
    },
    {
      "id": "tdt_vec",
      "name": "天地图矢量",
      "type": "tms",
      "enabled": false,
      "url": "https://t{s}.tianditu.gov.cn/vec_w/wmts?SERVICE=WMTS&REQUEST=GetTile&VERSION=1.0.0&LAYER=vec&STYLE=default&TILEMATRIXSET=w&FORMAT=tiles&TILECOL={x}&TILEROW={y}&TILEMATRIX={z}&tk={token}",
      "subdomains": "01234567",
      "token": "",
      "profile": "spherical-mercator"
    },
    {
      "id": "tdt_cia",
      "name": "天地图注记",
      "type": "tms",
      "enabled": false,
      "url": "https://t{s}.tianditu.gov.cn/cia_w/wmts?SERVICE=WMTS&REQUEST=GetTile&VERSION=1.0.0&LAYER=cia&STYLE=default&TILEMATRIXSET=w&FORMAT=tiles&TILECOL={x}&TILEROW={y}&TILEMATRIX={z}&tk={token}",
      "subdomains": "01234567",
      "token": "",
      "profile": "spherical-mercator"
    },
    {
      "id": "xyz_osm",
      "name": "OpenStreetMap (XYZ)",
      "type": "xyz",
      "enabled": false,
      "url": "https://tile.openstreetmap.org/{z}/{x}/{y}.png",
      "profile": "spherical-mercator"
    },
    {
      "id": "geoserver_wms",
      "name": "GeoServer WMS 示例",
      "type": "wms",
      "enabled": false,
      "url": "http://localhost:8080/geoserver/wms",
      "layers": "workspace:layer_name",
      "style": "",
      "format": "image/png",
      "srs": "EPSG:4326",
      "transparent": true
    },
    {
      "id": "arcgis_rest",
      "name": "ArcGIS World Imagery",
      "type": "arcgis",
      "enabled": false,
      "url": "https://services.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer"
    },
    {
      "id": "bing_aerial",
      "name": "Bing 卫星影像",
      "type": "bing",
      "enabled": false,
      "key": "",
      "imagerySet": "Aerial"
    }
  ]
}
```

### layers.json — 图层持久化

位置：`data/layers.json`（相对于 exe 目录）

```json
{
  "description": "3DViewer 图层持久化 - 启动时按顺序加载 autoload=true 的图层",
  "layers": [
    {
      "id": "province",
      "name": "全国省",
      "path": "vector/全国省/全国省.shp",
      "kind": "vector",
      "autoload": true,
      "visible": true,
      "opacity": 1.0,
      "bandMapping": null
    },
    {
      "id": "satellite_2024",
      "name": "卫星影像",
      "path": "D:/data/satellite_2024.tif",
      "kind": "imagery",
      "autoload": true,
      "visible": true,
      "opacity": 0.85,
      "bandMapping": { "red": 1, "green": 2, "blue": 3 }
    }
  ]
}
```

## 底图类型参数映射（osgEarth 3.x API）

| type | 必需字段 | 可选字段 | osgEarth 类 | 头文件 |
|------|---------|---------|------------|--------|
| `gdal` | `path` | — | `GDALImageLayer` | `<osgEarth/GDAL>` |
| `tms` | `url` | `profile`, `subdomains`, `token` | `TMSImageLayer` | `<osgEarth/TMS>` |
| `wms` | `url`, `layers` | `style`, `format`, `srs`, `transparent` | `WMSImageLayer` | `<osgEarth/WMS>` |
| `xyz` | `url` | `profile` | `XYZImageLayer` | `<osgEarth/XYZ>` |
| `arcgis` | `url` | `token` | `ArcGISServerImageLayer` | `<osgEarth/ArcGISServer>` |
| `bing` | `key`, `imagerySet` | — | `BingImageLayer` | `<osgEarth/Bing>` |

注意：osgEarth 3.x 不提供 `WMTSImageLayer`，WMTS 类型的在线服务可通过 `TMSImageLayer` 或 `XYZImageLayer` 接入（天地图 WMTS 即使用 TMS 兼容模式）。

## 行为规则

1. **底图加载**：遍历 `basemaps`，取第一个 `enabled=true` 的条目创建 osgEarth 图层；全部失败则回退内置 Natural Earth
2. **图层加载**：启动时遍历 `layers`，对 `autoload=true` 的条目按顺序调用 `DataImporter::import`，失败则跳过并在状态栏提示
3. **图层保存**：程序退出时，将 `LayerManager` 中所有图层序列化写入 `layers.json`（包括运行时新增的图层）
4. **路径解析**：不以 `/` 或盘符（如 `C:`）开头的路径视为相对于 exe 所在目录
5. **文件缺失**：`basemap.json` 不存在则用内置默认底图；`layers.json` 不存在则为空列表
6. **在线服务超时**：osgEarth 内部处理网络请求，底图加载失败时回退到内置底图

## 图层持久化字段说明

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | string | 图层唯一标识（文件名去扩展名） |
| `name` | string | 显示名称 |
| `path` | string | 相对 exe 目录或绝对路径 |
| `kind` | string | `imagery`/`elevation`/`vector`/`chart`/`scientific` |
| `autoload` | bool | 启动时是否加载到内存 |
| `visible` | bool | 加载后是否可见 |
| `opacity` | double | 不透明度 0.0~1.0 |
| `bandMapping` | object/null | 影像图层 RGB 波段映射 `{red, green, blue}` |

## 代码变更清单

| 变更类型 | 文件 | 说明 |
|---------|------|------|
| 新增 | `src/data/MapConfig.h/cpp` | 读取/解析 `basemap.json`，创建 osgEarth 底图 |
| 新增 | `src/data/LayerConfig.h/cpp` | 读取/保存 `layers.json`，序列化/反序列化图层 |
| 新增 | `data/basemap.json` | 底图配置文件 |
| 新增 | `data/layers.json` | 图层持久化文件（初始为空列表） |
| 修改 | `src/globe/SceneController.cpp` | `createBaseLayer()` 改为接受配置参数 |
| 修改 | `src/app/ApplicationController.h/cpp` | 启动时加载持久化图层，退出时保存 |
| 修改 | `src/app/MainWindow.h/cpp` | `closeEvent` 触发保存 |
| 修改 | `CMakeLists.txt` | 添加新源文件和 JSON 数据文件部署 |

## 架构约束

- `MapConfig` 只负责读取和解析 JSON，不直接创建 osgEarth 对象；osgEarth 对象的创建仍在 `SceneController` 中
- `LayerConfig` 只负责 JSON 序列化/反序列化，不直接操作 `LayerManager` 或 `SceneController`
- `ApplicationController` 编排加载/保存流程，保持现有的「UI 不直接操作场景」约束
- 底图配置文件随项目分发，程序不修改 `basemap.json`；图层持久化文件由程序自动维护

## 不做的事情

- 不提供 UI 编辑底图配置（用户手工编辑 JSON）
- 不支持多底图叠加
- 不修改 osgEarth 渲染管线
- 不处理在线服务的认证流程（token/key 由用户在 JSON 中填写）
