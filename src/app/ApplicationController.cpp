#include "app/ApplicationController.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QMessageBox>
#include <QObject>
#include <QSettings>
#include <QStatusBar>
#include <QString>
#include <QStringList>
#include <QStringLiteral>
#include <spdlog/spdlog.h>
#include <vector>

using namespace Qt::Literals::StringLiterals;

#include "app/MainWindow.h"
#include "data/DataImporter.h"
#include "data/LayerConfig.h"
#include "data/MapConfig.h"
#include "globe/GlobeWidget.h"
#include "globe/SceneController.h"
#include "layers/Layer.h"
#include "layers/LayerManager.h"
#include "tools/ToolManager.h"

namespace {

constexpr int kMaxRecentFiles = 5;
constexpr const char *kRecentFilesKey = "recentFiles";

QString layerKindToText(LayerKind kind) {
    switch (kind) {
    case LayerKind::Imagery:
        return u"影像"_s;
    case LayerKind::Elevation:
        return u"高程"_s;
    case LayerKind::Vector:
        return u"矢量"_s;
    case LayerKind::Model:
        return u"三维模型"_s;
    case LayerKind::Chart:
        return u"海图"_s;
    case LayerKind::Scientific:
        return u"科学数据"_s;
    }

    return u"未知"_s;
}

QString utf8(const std::string &s) {
    return QString::fromUtf8(s.c_str(), static_cast<int>(s.size()));
}

QStringList supportedDirectoryImportFilters() {
    return {
        "*.shp", "*.SHP",
        "*.geojson", "*.GEOJSON",
        "*.gpkg", "*.GPKG",
        "*.kml", "*.KML",
        "*.gml", "*.GML",
        "*.json", "*.JSON"
    };
}

bool isRenderableLayerKind(LayerKind kind) {
    switch (kind) {
    case LayerKind::Imagery:
    case LayerKind::Elevation:
    case LayerKind::Vector:
    case LayerKind::Model:
        return true;
    case LayerKind::Chart:
    case LayerKind::Scientific:
        return false;
    }

    return false;
}

std::optional<LayerKind> parseLayerKind(const std::string &kind) {
    if (kind == "imagery") return LayerKind::Imagery;
    if (kind == "elevation") return LayerKind::Elevation;
    if (kind == "vector") return LayerKind::Vector;
    if (kind == "model") return LayerKind::Model;
    if (kind == "chart") return LayerKind::Chart;
    if (kind == "scientific") return LayerKind::Scientific;
    return std::nullopt;
}

} // namespace

ApplicationController::ApplicationController(MainWindow &window,
                                             SceneController &sceneController,
                                             LayerManager &layerManager,
                                             DataImporter &importer)
    : window_(window),
      sceneController_(sceneController),
      layerManager_(layerManager),
      importer_(importer) {
    QObject::connect(&window_, &MainWindow::importDataRequested, [this](const QString &path) {
        importFile(path.toUtf8().toStdString());
    });

    QObject::connect(&window_, &MainWindow::layerSelected, [this](const QString &layerId) {
        showLayerDetails(layerId.toStdString());
    });

    QObject::connect(&window_, &MainWindow::layerVisibilityChanged, [this](const QString &layerId, bool visible) {
        setLayerVisibility(layerId.toStdString(), visible);
    });

    QObject::connect(&window_, &MainWindow::layerOrderChanged, [this](const QStringList &orderedIds) {
        applyLayerOrderFromUi(orderedIds);
    });

    QObject::connect(&window_, &MainWindow::removeLayerRequested, [this](const QString &layerId) {
        removeLayer(layerId.toStdString());
    });

    QObject::connect(&window_, &MainWindow::layerOpacityChanged, [this](const QString &layerId, double opacity) {
        setLayerOpacity(layerId.toStdString(), opacity);
    });

    QObject::connect(&window_, &MainWindow::bandMappingChanged, [this](const QString &layerId, int red, int green, int blue) {
        setBandMapping(layerId.toStdString(), red, green, blue);
    });

    QObject::connect(&window_, &MainWindow::modelPlacementChanged, [this](const QString &layerId, const ModelPlacement &placement) {
        setModelPlacement(layerId.toStdString(), placement);
    });

    QObject::connect(&window_, &MainWindow::toolChanged, [this](int toolId) {
        window_.globeWidget()->toolManager().setActiveTool(static_cast<ToolId>(toolId));
    });

    QObject::connect(&window_, &MainWindow::resetViewRequested, [this]() {
        resetView();
    });

    QObject::connect(&window_, &MainWindow::zoomToLayerRequested, [this](const QString &layerId) {
        zoomToLayer(layerId.toStdString());
    });

    QObject::connect(&window_, &MainWindow::screenshotRequested, [this]() {
        captureScreenshot();
    });

    QObject::connect(window_.globeWidget(), &GlobeWidget::terrainPickCompleted, [this](const PickResult &pick) {
        handleTerrainPick(pick);
    });
}

void ApplicationController::importFile(const std::string &path) {
    std::vector<std::string> pathsToImport;
    const QString qPath = utf8(path);
    const QDir dir(qPath);

    if (dir.exists()) {
        const QFileInfoList vectorEntries = dir.entryInfoList(supportedDirectoryImportFilters(), QDir::Files);
        if (vectorEntries.isEmpty()) {
            window_.statusBar()->showMessage(u"目录中未找到可导入的矢量数据文件。"_s, 3000);
            QMessageBox::warning(
                &window_,
                u"导入失败"_s,
                QString(
                    u"目录中未找到支持的矢量文件：\n%1\n\n支持格式：Shapefile (.shp)、GeoJSON (.geojson/.json)、GeoPackage (.gpkg)、KML (.kml)、GML (.gml)"_s)
                    .arg(qPath));
            return;
        }
        for (const QFileInfo &fi : vectorEntries) {
            pathsToImport.push_back(fi.absoluteFilePath().toUtf8().toStdString());
        }
    } else {
        pathsToImport.push_back(path);
    }

    for (const auto &importPath : pathsToImport) {
        spdlog::info("ApplicationController: importing '{}'", importPath);
        window_.statusBar()->showMessage(QString(u"正在导入 %1..."_s).arg(utf8(importPath)));

        const auto layer = importer_.import(importPath);
        if (!layer) {
            spdlog::warn("ApplicationController: import failed for '{}'", importPath);
            window_.statusBar()->showMessage(u"导入失败。"_s, 3000);
            QMessageBox::warning(
                &window_,
                u"导入失败"_s,
                QString(
                    u"无法导入或当前版本暂不支持显示：\n%1\n\n支持格式包括：GeoTIFF/TIFF、IMG、ASC、SRTM/HGT、DEM、VRT、Shapefile、GeoJSON/JSON、GeoPackage、KML、GML，以及 glTF/glb 三维模型。"_s)
                    .arg(utf8(importPath)));
            return;
        }

        if (!layerManager_.addLayer(layer)) {
            spdlog::warn("ApplicationController: duplicate layer for '{}'", importPath);
            window_.statusBar()->showMessage(u"图层已存在。"_s, 3000);
            QMessageBox::information(&window_, u"已导入"_s,
                                     QString(u"该文件已经导入：\n%1"_s).arg(utf8(importPath)));
            return;
        }

        sceneController_.addLayer(layer);
        window_.addLayerRow(*layer);
        addToRecentFiles(utf8(importPath));
        spdlog::info("ApplicationController: layer '{}' imported successfully", layer->id());

        window_.statusBar()->showMessage(
            QString(u"已导入：%1 (%2)"_s).arg(utf8(layer->name())).arg(layerKindToText(layer->kind())),
            5000);

        if (const auto bounds = layer->geographicBounds(); bounds.has_value() && bounds->isValid()) {
            sceneController_.flyToGeographicBounds(*bounds);
        }
    }
}

void ApplicationController::showLayerDetails(const std::string &layerId) {
    const auto layer = layerManager_.findById(layerId);
    if (!layer) {
        window_.showLayerDetails(u"未找到图层。"_s);
        window_.clearLayerProperties();
        return;
    }

    window_.showLayerProperties(
        utf8(layerId),
        utf8(layer->name()),
        layerKindToText(layer->kind()),
        utf8(layer->sourceUri()),
        layer->visible(),
        layer->opacity(),
        layer->rasterMetadata(),
        layer->vectorMetadata(),
        layer->modelPlacement());
}

void ApplicationController::setLayerVisibility(const std::string &layerId, bool visible) {
    layerManager_.setVisibility(layerId, visible);
    const auto layer = layerManager_.findById(layerId);
    if (layer) {
        sceneController_.syncLayerState(layer);
        showLayerDetails(layerId);
    }
}

void ApplicationController::applyLayerOrderFromUi(const QStringList &orderedIds) {
    std::vector<std::string> ids;
    ids.reserve(static_cast<std::size_t>(orderedIds.size()));
    for (const QString &q : orderedIds) {
        ids.push_back(q.toStdString());
    }

    if (!layerManager_.reorderLayers(ids)) {
        return;
    }

    sceneController_.reorderUserLayers(layerManager_.layers());
}

void ApplicationController::handleTerrainPick(const PickResult &pick) {
    if (!pick.hit) {
        window_.showLayerDetails(u"拾取：光标下无地形。"_s);
        return;
    }

    QStringList lines;
    lines.append(QString(u"经度：%1"_s).arg(pick.longitude, 0, 'f', 6));
    lines.append(QString(u"纬度：%1"_s).arg(pick.latitude, 0, 'f', 6));
    lines.append(QString(u"高程(近似)：%1"_s).arg(pick.elevation, 0, 'f', 2));
    if (!pick.layerId.empty()) {
        lines.append(QString(u"图层：%1"_s).arg(utf8(pick.displayText)));
    }

    if (!pick.featureAttributes.empty()) {
        lines.append("");
        lines.append(u"--- 要素属性 ---"_s);
        for (const auto &attr : pick.featureAttributes) {
            lines.append(QString(u"  %1：%2"_s).arg(utf8(attr.name)).arg(utf8(attr.value)));
        }
    }

    window_.showLayerDetails(lines.join('\n'));
}

void ApplicationController::removeLayer(const std::string &layerId) {
    spdlog::info("ApplicationController: removing layer '{}'", layerId);
    sceneController_.removeLayer(layerId);
    layerManager_.removeLayer(layerId);
    window_.removeLayerRow(layerId);
    window_.showLayerDetails(u"未选择图层。"_s);
    window_.clearLayerProperties();
}

void ApplicationController::setLayerOpacity(const std::string &layerId, double opacity) {
    const auto layer = layerManager_.findById(layerId);
    if (!layer) {
        return;
    }

    layer->setOpacity(opacity);
    sceneController_.syncLayerState(layer);
}

void ApplicationController::setModelPlacement(const std::string &layerId, const ModelPlacement &placement) {
    const auto layer = layerManager_.findById(layerId);
    if (!layer || layer->kind() != LayerKind::Model) {
        return;
    }

    layer->setModelPlacement(placement);
    sceneController_.syncLayerState(layer);
    showLayerDetails(layerId);
}

void ApplicationController::setBandMapping(const std::string &layerId, int red, int green, int blue) {
    const auto layer = layerManager_.findById(layerId);
    if (!layer || layer->kind() != LayerKind::Imagery) {
        return;
    }

    layer->setBandMapping(red, green, blue);
    sceneController_.updateImageLayerBands(layer);
    spdlog::info("ApplicationController: band mapping changed for '{}' to R={} G={} B={}", layerId, red, green, blue);
}

void ApplicationController::zoomToLayer(const std::string &layerId) {
    const auto layer = layerManager_.findById(layerId);
    if (!layer) {
        return;
    }
    const auto bounds = layer->geographicBounds();
    if (bounds.has_value() && bounds->isValid()) {
        sceneController_.flyToGeographicBounds(*bounds);
    }
}

void ApplicationController::resetView() {
    sceneController_.resetView();
}

void ApplicationController::captureScreenshot() {
    const QImage image = sceneController_.captureImage();
    if (image.isNull()) {
        window_.statusBar()->showMessage(u"截图失败：无法捕获画面。"_s, 3000);
        return;
    }

    const QString defaultPath = QDir::homePath() + u"/3DViewer_screenshot.png"_s;
    const QString path = QFileDialog::getSaveFileName(
        &window_, u"保存截图"_s, defaultPath,
        u"PNG 图像 (*.png);;JPEG 图像 (*.jpg);;BMP 图像 (*.bmp)"_s);
    if (path.isEmpty()) {
        return;
    }

    if (image.save(path)) {
        spdlog::info("ApplicationController: screenshot saved to '{}'", path.toStdString());
        window_.statusBar()->showMessage(QString(u"截图已保存：%1"_s).arg(path), 5000);
    } else {
        spdlog::warn("ApplicationController: failed to save screenshot to '{}'", path.toStdString());
        window_.statusBar()->showMessage(u"截图保存失败。"_s, 3000);
    }
}

void ApplicationController::addToRecentFiles(const QString &path) {
    QSettings settings;
    QStringList recent = settings.value(kRecentFilesKey).toStringList();

    recent.removeAll(path);
    recent.prepend(path);
    while (recent.size() > kMaxRecentFiles) {
        recent.removeLast();
    }

    settings.setValue(kRecentFilesKey, recent);
    window_.setRecentFiles(recent);
}

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
        if (!entry.autoload) {
            continue;
        }

        if (const auto kind = parseLayerKind(entry.kind); kind.has_value() && !isRenderableLayerKind(*kind)) {
            spdlog::warn("ApplicationController: skipping unsupported persisted layer '{}' kind={}", entry.id, entry.kind);
            window_.statusBar()->showMessage(
                QString(u"已跳过未支持的持久化图层：%1"_s).arg(QString::fromStdString(entry.name)), 5000);
            continue;
        }

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
        if (entry.modelPlacement.has_value() && layer->kind() == LayerKind::Model) {
            ModelPlacement placement;
            placement.longitude = entry.modelPlacement->longitude;
            placement.latitude = entry.modelPlacement->latitude;
            placement.height = entry.modelPlacement->height;
            placement.scale = entry.modelPlacement->scale;
            placement.heading = entry.modelPlacement->heading;
            layer->setModelPlacement(placement);
        }

        sceneController_.addLayer(layer);
        window_.addLayerRow(*layer);
        spdlog::info("ApplicationController: persisted layer '{}' loaded", layer->id());
    }
}

void ApplicationController::saveLayerConfigOnExit(const std::string &resourceDir) {
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
        case LayerKind::Model: entry.kind = "model"; break;
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

        if (layer->kind() == LayerKind::Model) {
            const auto placement = layer->modelPlacement();
            if (placement.has_value()) {
                ModelPlacementEntry savedPlacement;
                savedPlacement.longitude = placement->longitude;
                savedPlacement.latitude = placement->latitude;
                savedPlacement.height = placement->height;
                savedPlacement.scale = placement->scale;
                savedPlacement.heading = placement->heading;
                entry.modelPlacement = savedPlacement;
            }
        }

        config.layers.push_back(entry);
    }

    saveLayerConfig(resourceDir, config);
}
