# 地图配置与图层持久化 实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 将硬编码的底图加载改为配置驱动（支持 GDAL/TMS/WMS/XYZ/ArcGIS/Bing），并将图层列表持久化到 JSON 文件，实现退出保存、启动自动加载。

**架构：** 新增 `MapConfig` 模块读取 `basemap.json` 并将解析结果传给 `SceneController` 创建底图；新增 `LayerConfig` 模块读写 `layers.json` 实现图层序列化/反序列化；`ApplicationController` 编排加载/保存流程，`MainWindow` 在 `closeEvent` 中触发保存。

**技术栈：** C++20, Qt6 (QJsonDocument/QFile/QDir), osgEarth 3.x (TMSImageLayer/WMSImageLayer/XYZImageLayer/ArcGISServerImageLayer/BingImageLayer), spdlog

---

## 文件结构

| 文件 | 职责 |
|------|------|
| `src/data/MapConfig.h` | `BasemapEntry` 结构体 + `loadBasemapConfig()` 声明 |
| `src/data/MapConfig.cpp` | 读取/解析 `basemap.json`，返回 `std::vector<BasemapEntry>` |
| `src/data/LayerConfig.h` | `LayerEntry` 结构体 + `loadLayerConfig()`/`saveLayerConfig()` 声明 |
| `src/data/LayerConfig.cpp` | 读取/保存 `layers.json`，序列化/反序列化图层条目 |
| `src/globe/SceneController.h` | 新增 `createBaseLayerFromConfig()` 方法声明 |
| `src/globe/SceneController.cpp` | 实现多种底图类型的 osgEarth 图层创建，替换硬编码 `createBaseLayer()` |
| `src/app/ApplicationController.h` | 新增 `loadPersistedLayers()` / `saveLayerConfig()` 方法声明 |
| `src/app/ApplicationController.cpp` | 启动时加载持久化图层，退出时保存 |
| `src/app/MainWindow.h` | 新增 `closeEvent` 重写 + `saveAndExitRequested` 信号 |
| `src/app/MainWindow.cpp` | `closeEvent` 触发保存 |
| `data/basemap.json` | 底图配置文件 |
| `data/layers.json` | 图层持久化文件（初始为空列表） |
| `CMakeLists.txt` | 添加新源文件，添加 JSON 文件部署规则 |

---

### 任务 1：创建 MapConfig 数据结构和 JSON 解析器

**文件：**
- 创建：`src/data/MapConfig.h`
- 创建：`src/data/MapConfig.cpp`

- [ ] **步骤 1：创建 MapConfig.h**

```cpp
#pragma once

#include <optional>
#include <string>
#include <vector>

enum class BasemapType {
    GDAL,
    TMS,
    WMS,
    XYZ,
    ArcGIS,
    Bing
};

struct BasemapEntry {
    std::string id;
    std::string name;
    BasemapType type = BasemapType::GDAL;
    bool enabled = false;

    std::string path;
    std::string url;
    std::string profile;
    std::string subdomains;
    std::string token;

    std::string wmsLayers;
    std::string wmsStyle;
    std::string wmsFormat;
    std::string wmsSrs;
    bool wmsTransparent = false;

    std::string bingKey;
    std::string bingImagerySet;
};

struct BasemapConfig {
    std::vector<BasemapEntry> basemaps;
};

std::optional<BasemapConfig> loadBasemapConfig(const std::string &basePath);
```

- [ ] **步骤 2：创建 MapConfig.cpp**

```cpp
#include "data/MapConfig.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <spdlog/spdlog.h>

namespace {

BasemapType parseBasemapType(const QString &type) {
    if (type == "gdal") return BasemapType::GDAL;
    if (type == "tms") return BasemapType::TMS;
    if (type == "wms") return BasemapType::WMS;
    if (type == "xyz") return BasemapType::XYZ;
    if (type == "arcgis") return BasemapType::ArcGIS;
    if (type == "bing") return BasemapType::Bing;
    return BasemapType::GDAL;
}

BasemapEntry parseBasemapEntry(const QJsonObject &obj) {
    BasemapEntry entry;
    entry.id = obj["id"].toString().toStdString();
    entry.name = obj["name"].toString().toStdString();
    entry.type = parseBasemapType(obj["type"].toString());
    entry.enabled = obj["enabled"].toBool(false);

    entry.path = obj["path"].toString().toStdString();
    entry.url = obj["url"].toString().toStdString();
    entry.profile = obj["profile"].toString().toStdString();
    entry.subdomains = obj["subdomains"].toString().toStdString();
    entry.token = obj["token"].toString().toStdString();

    entry.wmsLayers = obj["layers"].toString().toStdString();
    entry.wmsStyle = obj["style"].toString().toStdString();
    entry.wmsFormat = obj["format"].toString().toStdString();
    entry.wmsSrs = obj["srs"].toString().toStdString();
    entry.wmsTransparent = obj["transparent"].toBool(false);

    entry.bingKey = obj["key"].toString().toStdString();
    entry.bingImagerySet = obj["imagerySet"].toString().toStdString();

    return entry;
}

}

std::optional<BasemapConfig> loadBasemapConfig(const std::string &basePath) {
    const QString filePath = QString::fromStdString(basePath) + "/basemap.json";

    if (!QFile::exists(filePath)) {
        spdlog::info("MapConfig: basemap.json not found at '{}', using defaults", filePath.toStdString());
        return std::nullopt;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::warn("MapConfig: cannot open '{}'", filePath.toStdString());
        return std::nullopt;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        spdlog::warn("MapConfig: basemap.json is not a JSON object");
        return std::nullopt;
    }

    const QJsonObject root = doc.object();
    const QJsonArray basemaps = root["basemaps"].toArray();

    BasemapConfig config;
    for (const QJsonValue &val : basemaps) {
        if (val.isObject()) {
            config.basemaps.push_back(parseBasemapEntry(val.toObject()));
        }
    }

    spdlog::info("MapConfig: loaded {} basemap entries", config.basemaps.size());
    return config;
}
```

- [ ] **步骤 3：构建验证编译通过**

运行：`cmake --build build/debug --config Debug 2>&1 | head -30`
预期：编译通过（此时新文件尚未加入 CMakeLists.txt，此步骤仅验证头文件语法）

---

### 任务 2：创建 LayerConfig 数据结构和 JSON 读写器

**文件：**
- 创建：`src/data/LayerConfig.h`
- 创建：`src/data/LayerConfig.cpp`

- [ ] **步骤 1：创建 LayerConfig.h**

```cpp
#pragma once

#include <optional>
#include <string>
#include <vector>

struct BandMappingEntry {
    int red = 1;
    int green = 2;
    int blue = 3;
};

struct LayerEntry {
    std::string id;
    std::string name;
    std::string path;
    std::string kind;
    bool autoload = true;
    bool visible = true;
    double opacity = 1.0;
    std::optional<BandMappingEntry> bandMapping;
};

struct LayerConfig {
    std::vector<LayerEntry> layers;
};

std::optional<LayerConfig> loadLayerConfig(const std::string &basePath);
bool saveLayerConfig(const std::string &basePath, const LayerConfig &config);
```

- [ ] **步骤 2：创建 LayerConfig.cpp**

```cpp
#include "data/LayerConfig.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <spdlog/spdlog.h>

namespace {

LayerEntry parseLayerEntry(const QJsonObject &obj) {
    LayerEntry entry;
    entry.id = obj["id"].toString().toStdString();
    entry.name = obj["name"].toString().toStdString();
    entry.path = obj["path"].toString().toStdString();
    entry.kind = obj["kind"].toString().toStdString();
    entry.autoload = obj["autoload"].toBool(true);
    entry.visible = obj["visible"].toBool(true);
    entry.opacity = obj["opacity"].toDouble(1.0);

    const QJsonObject bm = obj["bandMapping"].toObject();
    if (!bm.isEmpty()) {
        BandMappingEntry bandMapping;
        bandMapping.red = bm["red"].toInt(1);
        bandMapping.green = bm["green"].toInt(2);
        bandMapping.blue = bm["blue"].toInt(3);
        entry.bandMapping = bandMapping;
    }

    return entry;
}

QJsonObject serializeLayerEntry(const LayerEntry &entry) {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(entry.id);
    obj["name"] = QString::fromStdString(entry.name);
    obj["path"] = QString::fromStdString(entry.path);
    obj["kind"] = QString::fromStdString(entry.kind);
    obj["autoload"] = entry.autoload;
    obj["visible"] = entry.visible;
    obj["opacity"] = entry.opacity;

    if (entry.bandMapping.has_value()) {
        QJsonObject bm;
        bm["red"] = entry.bandMapping->red;
        bm["green"] = entry.bandMapping->green;
        bm["blue"] = entry.bandMapping->blue;
        obj["bandMapping"] = bm;
    } else {
        obj["bandMapping"] = QJsonValue::Null;
    }

    return obj;
}

}

std::optional<LayerConfig> loadLayerConfig(const std::string &basePath) {
    const QString filePath = QString::fromStdString(basePath) + "/layers.json";

    if (!QFile::exists(filePath)) {
        spdlog::info("LayerConfig: layers.json not found at '{}'", filePath.toStdString());
        return std::nullopt;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::warn("LayerConfig: cannot open '{}'", filePath.toStdString());
        return std::nullopt;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        spdlog::warn("LayerConfig: layers.json is not a JSON object");
        return std::nullopt;
    }

    const QJsonObject root = doc.object();
    const QJsonArray layers = root["layers"].toArray();

    LayerConfig config;
    for (const QJsonValue &val : layers) {
        if (val.isObject()) {
            config.layers.push_back(parseLayerEntry(val.toObject()));
        }
    }

    spdlog::info("LayerConfig: loaded {} layer entries", config.layers.size());
    return config;
}

bool saveLayerConfig(const std::string &basePath, const LayerConfig &config) {
    const QString filePath = QString::fromStdString(basePath) + "/layers.json";

    QJsonArray layersArray;
    for (const auto &entry : config.layers) {
        layersArray.append(serializeLayerEntry(entry));
    }

    QJsonObject root;
    root["description"] = "3DViewer 图层持久化 - 启动时按顺序加载 autoload=true 的图层";
    root["layers"] = layersArray;

    QJsonDocument doc(root);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        spdlog::warn("LayerConfig: cannot write to '{}'", filePath.toStdString());
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    spdlog::info("LayerConfig: saved {} layer entries to '{}'", config.layers.size(), filePath.toStdString());
    return true;
}
```

---

### 任务 3：修改 SceneController 支持配置驱动的底图创建

**文件：**
- 修改：`src/globe/SceneController.h`
- 修改：`src/globe/SceneController.cpp`

- [ ] **步骤 1：在 SceneController.h 中添加新方法声明**

在 `SceneController` 类的 public 区域添加：

```cpp
void setBasemapConfig(const BasemapConfig &config, const std::string &resourceDir);
```

在文件顶部添加前向声明：

```cpp
struct BasemapConfig;
```

- [ ] **步骤 2：在 SceneController.cpp 中添加头文件引用**

在已有的 `#include` 区域添加：

```cpp
#include "data/MapConfig.h"
#include <osgEarth/TMS>
#include <osgEarth/WMS>
#include <osgEarth/XYZ>
#include <osgEarth/ArcGISServer>
#include <osgEarth/Bing>
#include <osgEarth/Profile>
```

- [ ] **步骤 3：在 SceneController.cpp 中实现 createBaseLayerFromConfig 函数**

在匿名命名空间中添加以下函数，替换原有的 `createBaseLayer()`：

```cpp
osg::ref_ptr<osgEarth::ImageLayer> createBaseLayerFromConfig(const BasemapEntry &entry, const std::string &resourceDir) {
    auto resolvePath = [&](const std::string &p) -> std::string {
        if (p.empty()) return p;
        if (p.size() >= 2 && (p[1] == ':' || (p[0] == '/' || p[0] == '\\'))) return p;
        return resourceDir + "/" + p;
    };

    switch (entry.type) {
    case BasemapType::GDAL: {
        auto layer = osg::ref_ptr<osgEarth::GDALImageLayer>(new osgEarth::GDALImageLayer());
        layer->setName(entry.name);
        layer->setURL(osgEarth::URI(resolvePath(entry.path)));
        return layer;
    }
    case BasemapType::TMS: {
        auto layer = osg::ref_ptr<osgEarth::TMSImageLayer>(new osgEarth::TMSImageLayer());
        layer->setName(entry.name);
        layer->setURL(osgEarth::URI(entry.url));
        if (!entry.profile.empty()) {
            if (entry.profile == "spherical-mercator" || entry.profile == "mercator") {
                layer->setProfile(osgEarth::Profile::create(osgEarth::Profile::SPHERICAL_MERCATOR));
            } else if (entry.profile == "geodetic" || entry.profile == "wgs84") {
                layer->setProfile(osgEarth::Profile::create(osgEarth::Profile::GLOBAL_GEODETIC));
            }
        }
        return layer;
    }
    case BasemapType::WMS: {
        auto layer = osg::ref_ptr<osgEarth::WMSImageLayer>(new osgEarth::WMSImageLayer());
        layer->setName(entry.name);
        layer->setURL(osgEarth::URI(entry.url));
        if (!entry.wmsLayers.empty()) layer->setLayers(entry.wmsLayers);
        if (!entry.wmsStyle.empty()) layer->setStyle(entry.wmsStyle);
        if (!entry.wmsFormat.empty()) layer->setFormat(entry.wmsFormat);
        if (!entry.wmsSrs.empty()) layer->setSRS(entry.wmsSrs);
        layer->setTransparent(entry.wmsTransparent);
        return layer;
    }
    case BasemapType::XYZ: {
        auto layer = osg::ref_ptr<osgEarth::XYZImageLayer>(new osgEarth::XYZImageLayer());
        layer->setName(entry.name);
        layer->setURL(osgEarth::URI(entry.url));
        if (!entry.profile.empty()) {
            if (entry.profile == "spherical-mercator" || entry.profile == "mercator") {
                layer->setProfile(osgEarth::Profile::create(osgEarth::Profile::SPHERICAL_MERCATOR));
            } else if (entry.profile == "geodetic" || entry.profile == "wgs84") {
                layer->setProfile(osgEarth::Profile::create(osgEarth::Profile::GLOBAL_GEODETIC));
            }
        }
        return layer;
    }
    case BasemapType::ArcGIS: {
        auto layer = osg::ref_ptr<osgEarth::ArcGISServerImageLayer>(new osgEarth::ArcGISServerImageLayer());
        layer->setName(entry.name);
        layer->setURL(osgEarth::URI(entry.url));
        if (!entry.token.empty()) layer->setToken(entry.token);
        return layer;
    }
    case BasemapType::Bing: {
        auto layer = osg::ref_ptr<osgEarth::BingImageLayer>(new osgEarth::BingImageLayer());
        layer->setName(entry.name);
        if (!entry.bingKey.empty()) layer->setAPIKey(entry.bingKey);
        if (!entry.bingImagerySet.empty()) layer->setImagerySet(entry.bingImagerySet);
        return layer;
    }
    }

    return nullptr;
}

osg::ref_ptr<osgEarth::ImageLayer> createFallbackBaseLayer(const std::string &resourceDir) {
    auto layer = osg::ref_ptr<osgEarth::GDALImageLayer>(new osgEarth::GDALImageLayer());
    layer->setName("Natural Earth");
    layer->setURL(osgEarth::URI(resourceDir + "/" + kDefaultBaseMapFilename));
    return layer;
}
```

- [ ] **步骤 4：添加 SceneController::setBasemapConfig 实现**

```cpp
void SceneController::setBasemapConfig(const BasemapConfig &config, const std::string &resourceDir) {
    for (const auto &entry : config.basemaps) {
        if (!entry.enabled) continue;

        spdlog::info("SceneController: trying basemap '{}' type={}", entry.id, static_cast<int>(entry.type));
        auto layer = createBaseLayerFromConfig(entry, resourceDir);
        if (layer) {
            impl_->baseLayer = layer;
            spdlog::info("SceneController: basemap '{}' selected", entry.id);
            return;
        }
    }

    spdlog::info("SceneController: no enabled basemap found, using fallback");
    impl_->baseLayer = createFallbackBaseLayer(resourceDir);
}
```

- [ ] **步骤 5：修改 initializeDefaultScene 使用配置驱动的底图**

修改 `initializeDefaultScene` 方法，将原来直接调用 `createBaseLayer()` 改为使用 `impl_->baseLayer`（此时已由 `setBasemapConfig` 设置，或回退到默认）：

将：
```cpp
impl_->baseLayer = createBaseLayer();
impl_->map->addLayer(impl_->baseLayer.get());
```

改为：
```cpp
if (!impl_->baseLayer) {
    impl_->baseLayer = createFallbackBaseLayer(
        std::string(kDefaultResourceDir));
}
impl_->map->addLayer(impl_->baseLayer.get());
```

- [ ] **步骤 6：删除旧的 createBaseLayer 函数**

删除匿名命名空间中的旧 `createBaseLayer()` 函数。

---

### 任务 4：修改 ApplicationController 集成配置加载和图层持久化

**文件：**
- 修改：`src/app/ApplicationController.h`
- 修改：`src/app/ApplicationController.cpp`

- [ ] **步骤 1：在 ApplicationController.h 中添加新方法**

```cpp
void loadBasemapAndLayers(const std::string &resourceDir);
void saveLayerConfig(const std::string &resourceDir);
```

- [ ] **步骤 2：在 ApplicationController.cpp 中添加头文件引用**

```cpp
#include "data/MapConfig.h"
#include "data/LayerConfig.h"
```

- [ ] **步骤 3：实现 loadBasemapAndLayers**

```cpp
void ApplicationController::loadBasemapAndLayers(const std::string &resourceDir) {
    auto basemapConfig = loadBasemapConfig(resourceDir);
    if (basemapConfig.has_value()) {
        sceneController_.setBasemapConfig(*basemapConfig, resourceDir);
    }

    auto layerConfig = loadLayerConfig(resourceDir);
    if (!layerConfig.has_value()) {
        return;
    }

    for (const auto &entry : layerConfig->layers) {
        if (!entry.autoload) continue;

        std::string resolvedPath = entry.path;
        if (!resolvedPath.empty() && !(resolvedPath.size() >= 2 && resolvedPath[1] == ':') && resolvedPath[0] != '/' && resolvedPath[0] != '\\') {
            resolvedPath = resourceDir + "/" + resolvedPath;
        }

        spdlog::info("ApplicationController: loading persisted layer '{}' from '{}'", entry.id, resolvedPath);

        const auto layer = importer_.import(resolvedPath);
        if (!layer) {
            spdlog::warn("ApplicationController: failed to load persisted layer '{}'", entry.id);
            window_.statusBar()->showMessage(
                QString(u"无法加载持久化图层：%1"_s).arg(QString::fromStdString(entry.name)), 5000);
            continue;
        }

        if (!layerManager_.addLayer(layer)) {
            spdlog::warn("ApplicationController: duplicate persisted layer '{}'", entry.id);
            continue;
        }

        layer->setVisible(entry.visible);
        layer->setOpacity(entry.opacity);
        if (entry.bandMapping.has_value() && layer->kind() == LayerKind::Imagery) {
            layer->setBandMapping(entry.bandMapping->red, entry.bandMapping->green, entry.bandMapping->blue);
        }

        sceneController_.addLayer(layer);
        window_.addLayerRow(*layer);
        spdlog::info("ApplicationController: persisted layer '{}' loaded", layer->id());
    }
}
```

- [ ] **步骤 4：实现 saveLayerConfig**

```cpp
void ApplicationController::saveLayerConfig(const std::string &resourceDir) {
    LayerConfig config;
    const auto &layers = layerManager_.layers();

    for (const auto &layer : layers) {
        LayerEntry entry;
        entry.id = layer->id();
        entry.name = layer->name();
        entry.path = layer->sourceUri();
        entry.autoload = true;
        entry.visible = layer->visible();
        entry.opacity = layer->opacity();

        switch (layer->kind()) {
        case LayerKind::Imagery: entry.kind = "imagery"; break;
        case LayerKind::Elevation: entry.kind = "elevation"; break;
        case LayerKind::Vector: entry.kind = "vector"; break;
        case LayerKind::Chart: entry.kind = "chart"; break;
        case LayerKind::Scientific: entry.kind = "scientific"; break;
        }

        if (layer->kind() == LayerKind::Imagery) {
            const auto rm = layer->rasterMetadata();
            if (rm.has_value() && rm->bandCount >= 2) {
                BandMappingEntry bm;
                bm.red = layer->redBand();
                bm.green = layer->greenBand();
                bm.blue = layer->blueBand();
                entry.bandMapping = bm;
            }
        }

        config.layers.push_back(entry);
    }

    saveLayerConfig(resourceDir, config);
}
```

---

### 任务 5：修改 MainWindow 集成 closeEvent 保存

**文件：**
- 修改：`src/app/MainWindow.h`
- 修改：`src/app/MainWindow.cpp`

- [ ] **步骤 1：在 MainWindow.h 中添加信号和 closeEvent 重写**

在 signals 区域添加：

```cpp
void saveAndExitRequested();
```

在 protected 区域（`keyPressEvent` 旁边）添加：

```cpp
void closeEvent(QCloseEvent *event) override;
```

- [ ] **步骤 2：在 MainWindow.cpp 中实现 closeEvent**

```cpp
void MainWindow::closeEvent(QCloseEvent *event) {
    emit saveAndExitRequested();
    event->accept();
}
```

---

### 任务 6：修改 main.cpp 编排启动流程

**文件：**
- 修改：`src/main.cpp`

- [ ] **步骤 1：读取当前 main.cpp 内容并修改**

在 `MainWindow window;` 创建之后、`window.show();` 之前，添加配置加载调用。

需要将 `ApplicationController` 的创建从 `MainWindow` 构造函数中移到 `main.cpp`（如果当前在那里的话），或者在 `main.cpp` 中直接调用 `loadBasemapAndLayers`。

具体修改取决于当前 `main.cpp` 的结构。核心是在 `window.show()` 之前调用：

```cpp
controller.loadBasemapAndLayers(resourceDir.toStdString());
```

其中 `resourceDir` 的解析逻辑为：优先使用环境变量 `THREEDVIEWER_RESOURCE_DIR`，否则使用 `QCoreApplication::applicationDirPath() + "/data"`。

- [ ] **步骤 2：在 ApplicationController 构造函数中连接 saveAndExitRequested 信号**

在 `ApplicationController` 构造函数的信号连接区域添加：

```cpp
QObject::connect(&window_, &MainWindow::saveAndExitRequested, this, [this]() {
    const char *envDir = std::getenv("THREEDVIEWER_RESOURCE_DIR");
    const std::string resourceDir = (envDir && envDir[0] != '\0')
        ? std::string(envDir)
        : QCoreApplication::applicationDirPath().toStdString() + "/data";
    saveLayerConfig(resourceDir);
});
```

---

### 任务 7：创建配置文件并更新 CMakeLists.txt

**文件：**
- 创建：`data/basemap.json`
- 创建：`data/layers.json`
- 修改：`CMakeLists.txt`

- [ ] **步骤 1：创建 data/basemap.json**

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
      "id": "osm_tms",
      "name": "OpenStreetMap (TMS)",
      "type": "tms",
      "enabled": false,
      "url": "https://tile.openstreetmap.org/{z}/{x}/{y}.png",
      "profile": "spherical-mercator"
    },
    {
      "id": "osm_xyz",
      "name": "OpenStreetMap (XYZ)",
      "type": "xyz",
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

- [ ] **步骤 2：创建 data/layers.json**

```json
{
  "description": "3DViewer 图层持久化 - 启动时按顺序加载 autoload=true 的图层",
  "layers": []
}
```

- [ ] **步骤 3：修改 CMakeLists.txt 添加新源文件**

在 `add_executable(ThreeDViewer ...)` 的源文件列表中添加：

```cmake
src/data/MapConfig.cpp
src/data/LayerConfig.cpp
```

- [ ] **步骤 4：修改 CMakeLists.txt 添加 JSON 文件部署**

在 Win32 的 POST_BUILD 命令区域，添加 basemap.json 和 layers.json 的复制规则。在已有的 `add_custom_command(TARGET ThreeDViewer POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data" ...)` 命令已经会复制整个 `data/` 目录，所以 JSON 文件会自动被包含。

但需要确保 install 规则也包含 JSON 文件。在 `install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/" DESTINATION data CONFIGURATIONS Release FILES_MATCHING ...)` 中添加 `PATTERN "*.json"`。

---

### 任务 8：构建验证和手动测试

**文件：** 无新文件

- [ ] **步骤 1：配置和构建 Debug**

运行：`cmake --preset debug`
运行：`cmake --build build/debug --config Debug`

预期：编译通过，无错误

- [ ] **步骤 2：手动验证底图加载**

运行：`build/debug/Debug/3DViewer.exe`

预期：
- 默认加载 Natural Earth 底图（`basemap.json` 中 `enabled=true`）
- 状态栏显示坐标信息

- [ ] **步骤 3：手动验证图层持久化**

1. 导入一个 Shapefile 或 GeoTIFF
2. 关闭程序
3. 重新启动程序
4. 预期：之前导入的图层自动加载并显示

- [ ] **步骤 4：手动验证在线底图切换**

1. 编辑 `data/basemap.json`，将 `natural_earth` 的 `enabled` 改为 `false`，将 `osm_xyz` 的 `enabled` 改为 `true`
2. 重新启动程序
3. 预期：底图切换为 OpenStreetMap 在线瓦片

- [ ] **步骤 5：手动验证回退机制**

1. 将所有底图的 `enabled` 改为 `false`
2. 重新启动程序
3. 预期：回退到内置 Natural Earth 底图

- [ ] **步骤 6：Commit**

```bash
git add src/data/MapConfig.h src/data/MapConfig.cpp src/data/LayerConfig.h src/data/LayerConfig.cpp src/globe/SceneController.h src/globe/SceneController.cpp src/app/ApplicationController.h src/app/ApplicationController.cpp src/app/MainWindow.h src/app/MainWindow.cpp src/main.cpp data/basemap.json data/layers.json CMakeLists.txt
git commit -m "feat: 添加地图配置与图层持久化支持"
```
