#include "app/ApplicationController.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <vector>

#include "app/MainWindow.h"
#include "app/ScreenshotSave.h"
#include "data/DataImporter.h"
#include "data/LayerConfig.h"
#include "data/MapConfig.h"
#include "data/ProjectConfig.h"
#include "globe/GlobeWidget.h"
#include "globe/SceneController.h"
#include "layers/Layer.h"
#include "layers/LayerManager.h"
#include "tools/ToolManager.h"

namespace {

constexpr int kMaxRecentFiles = 5;
constexpr const char *kRecentFilesKey = "recentFiles";

QString utf8(const std::string &text) {
    return QString::fromUtf8(text.c_str(), static_cast<int>(text.size()));
}

QString layerKindToText(LayerKind kind) {
    switch (kind) {
    case LayerKind::Imagery:
        return QString::fromUtf8(u8"影像");
    case LayerKind::Elevation:
        return QString::fromUtf8(u8"高程");
    case LayerKind::Vector:
        return QString::fromUtf8(u8"矢量");
    case LayerKind::Measurement:
        return QString::fromUtf8(u8"量测");
    case LayerKind::Model:
        return QString::fromUtf8(u8"三维模型");
    case LayerKind::Chart:
        return QString::fromUtf8(u8"海图");
    case LayerKind::Scientific:
        return QString::fromUtf8(u8"科学数据");
    }

    return QString::fromUtf8(u8"未知");
}

QString modelFormatText() {
    const auto runtimeExtensions = DataImporter::availableRuntimeModelExtensions();
    const auto declaredExtensions = DataImporter::supportedModelExtensions();
    const auto &extensions = runtimeExtensions.empty() ? declaredExtensions : runtimeExtensions;

    QStringList items;
    for (const auto &extension : extensions) {
        items.append(QString::fromStdString(extension).remove('.').toUpper());
    }
    return items.join('/');
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
    case LayerKind::Measurement:
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
    if (kind == "measurement") return LayerKind::Measurement;
    if (kind == "model") return LayerKind::Model;
    if (kind == "chart") return LayerKind::Chart;
    if (kind == "scientific") return LayerKind::Scientific;
    return std::nullopt;
}

std::string serializeLayerKind(LayerKind kind) {
    switch (kind) {
    case LayerKind::Imagery: return "imagery";
    case LayerKind::Elevation: return "elevation";
    case LayerKind::Vector: return "vector";
    case LayerKind::Measurement: return "measurement";
    case LayerKind::Model: return "model";
    case LayerKind::Chart: return "chart";
    case LayerKind::Scientific: return "scientific";
    }

    return "unknown";
}

ProjectMeasurementData toProjectMeasurementData(const MeasurementLayerData &data) {
    ProjectMeasurementData measurementData;
    measurementData.kind = data.kind == MeasurementKind::Area ? "area" : "distance";
    measurementData.lengthMeters = data.lengthMeters;
    measurementData.areaSquareMeters = data.areaSquareMeters;
    measurementData.points.reserve(data.points.size());
    for (const auto &point : data.points) {
        measurementData.points.push_back(ProjectMeasurementPoint{point.longitude, point.latitude});
    }
    return measurementData;
}

MeasurementLayerData toMeasurementLayerData(const ProjectMeasurementData &data) {
    MeasurementLayerData measurementData;
    measurementData.kind = data.kind == "area" ? MeasurementKind::Area : MeasurementKind::Distance;
    measurementData.lengthMeters = data.lengthMeters;
    measurementData.areaSquareMeters = data.areaSquareMeters;
    measurementData.points.reserve(data.points.size());
    for (const auto &point : data.points) {
        measurementData.points.push_back(globe::MeasurementPoint{point.longitude, point.latitude});
    }
    return measurementData;
}

std::string resolveProjectPath(const QString &projectPath, const std::string &storedPath) {
    if (storedPath.empty()) {
        return {};
    }

    const QString qStoredPath = QString::fromStdString(storedPath);
    const QFileInfo storedInfo(qStoredPath);
    if (storedInfo.isAbsolute()) {
        return QDir::cleanPath(qStoredPath).toStdString();
    }

    const QFileInfo projectInfo(projectPath);
    return QDir::cleanPath(projectInfo.dir().filePath(qStoredPath)).toStdString();
}

QString measurementLayerBaseName(MeasurementKind kind) {
    return kind == MeasurementKind::Area
        ? QString::fromUtf8(u8"测面结果")
        : QString::fromUtf8(u8"测距结果");
}

QString formatMeasurementDistance(double meters) {
    if (meters >= 1000.0) {
        return QString::fromUtf8(u8"%1 km").arg(meters / 1000.0, 0, 'f', 3);
    }
    return QString::fromUtf8(u8"%1 m").arg(meters, 0, 'f', 1);
}

QString formatMeasurementArea(double squareMeters) {
    if (squareMeters >= 1000000.0) {
        return QString::fromUtf8(u8"%1 km²").arg(squareMeters / 1000000.0, 0, 'f', 3);
    }
    return QString::fromUtf8(u8"%1 m²").arg(squareMeters, 0, 'f', 1);
}

int nextMeasurementIndex(const LayerManager &layerManager, MeasurementKind kind) {
    int count = 0;
    for (const auto &layer : layerManager.layers()) {
        if (!layer || layer->kind() != LayerKind::Measurement) {
            continue;
        }
        const auto measurementData = layer->measurementData();
        if (!measurementData.has_value() || measurementData->kind != kind) {
            continue;
        }
        ++count;
    }
    return count + 1;
}

int measurementDisplayIndex(const LayerManager &layerManager, const Layer &layer, MeasurementKind kind) {
    int count = 0;
    for (const auto &existingLayer : layerManager.layers()) {
        if (!existingLayer || existingLayer->kind() != LayerKind::Measurement) {
            continue;
        }
        const auto measurementData = existingLayer->measurementData();
        if (!measurementData.has_value() || measurementData->kind != kind) {
            continue;
        }
        ++count;
        if (existingLayer->id() == layer.id()) {
            return count;
        }
    }
    return nextMeasurementIndex(layerManager, kind);
}

QString measurementLayerSummary(const MeasurementLayerData &data) {
    return data.kind == MeasurementKind::Area
        ? formatMeasurementArea(data.areaSquareMeters)
        : formatMeasurementDistance(data.lengthMeters);
}

QString measurementLayerDisplayName(MeasurementKind kind, int index, const MeasurementLayerData &data) {
    return QString::fromUtf8(u8"%1 %2（%3）")
        .arg(measurementLayerBaseName(kind))
        .arg(index)
        .arg(measurementLayerSummary(data));
}

QString measurementLayerDisplayName(const LayerManager &layerManager,
                                    const Layer &layer,
                                    const MeasurementLayerData &data) {
    return measurementLayerDisplayName(data.kind, measurementDisplayIndex(layerManager, layer, data.kind), data);
}

std::optional<GeographicBounds> measurementBounds(const MeasurementLayerData &data) {
    if (data.points.empty()) {
        return std::nullopt;
    }

    GeographicBounds bounds{
        data.points.front().longitude,
        data.points.front().latitude,
        data.points.front().longitude,
        data.points.front().latitude,
    };

    for (const auto &point : data.points) {
        bounds.west = (std::min)(bounds.west, point.longitude);
        bounds.south = (std::min)(bounds.south, point.latitude);
        bounds.east = (std::max)(bounds.east, point.longitude);
        bounds.north = (std::max)(bounds.north, point.latitude);
    }

    return bounds;
}

QString measurementTempPath(const QString &layerId) {
    const QString dirPath = QDir::temp().filePath(QString::fromUtf8(u8"3dviewer_measurements"));
    QDir().mkpath(dirPath);
    return QDir(dirPath).filePath(layerId + QString::fromUtf8(u8".geojson"));
}

QString safeExportBaseName(const QString &name) {
    QString sanitized = name;
    const QString invalidChars = QString::fromUtf8(u8"\\/:*?\"<>|");
    for (const QChar ch : invalidChars) {
        sanitized.replace(ch, '_');
    }
    sanitized = sanitized.trimmed();
    if (sanitized.isEmpty()) {
        return QString::fromUtf8(u8"量测结果");
    }
    return sanitized;
}

bool writeMeasurementGeoJson(const QString &path, const MeasurementLayerData &data) {
    QJsonArray coordinates;
    for (const auto &point : data.points) {
        coordinates.append(QJsonArray{point.longitude, point.latitude});
    }

    QJsonObject geometry;
    geometry["type"] = data.kind == MeasurementKind::Area ? "Polygon" : "LineString";
    if (data.kind == MeasurementKind::Area) {
        if (!data.points.empty()) {
            const auto &first = data.points.front();
            const auto &last = data.points.back();
            if (first.longitude != last.longitude || first.latitude != last.latitude) {
                coordinates.append(QJsonArray{first.longitude, first.latitude});
            }
        }
        geometry["coordinates"] = QJsonArray{coordinates};
    } else {
        geometry["coordinates"] = coordinates;
    }

    QJsonObject properties;
    properties["kind"] = data.kind == MeasurementKind::Area ? "area" : "distance";
    properties["pointCount"] = static_cast<int>(data.points.size());
    properties["lengthMeters"] = data.lengthMeters;
    properties["areaSquareMeters"] = data.areaSquareMeters;

    QJsonObject feature;
    feature["type"] = "Feature";
    feature["properties"] = properties;
    feature["geometry"] = geometry;

    QJsonObject collection;
    collection["type"] = "FeatureCollection";
    collection["features"] = QJsonArray{feature};

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(collection).toJson(QJsonDocument::Compact));
    return file.flush();
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

    QObject::connect(&window_, &MainWindow::layerRenameRequested, [this](const QString &layerId, const QString &name) {
        renameLayer(layerId.toStdString(), name);
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
        window_.globeWidget()->sceneController().clearMeasurementDraft();
        window_.globeWidget()->toolManager().setActiveTool(static_cast<ToolId>(toolId));
    });

    QObject::connect(&window_, &MainWindow::undoMeasurementRequested, [this]() {
        window_.globeWidget()->toolManager().undoActiveToolState(*window_.globeWidget());
    });

    QObject::connect(&window_, &MainWindow::editSelectedMeasurementRequested, [this]() {
        editSelectedMeasurement();
    });

    QObject::connect(&window_, &MainWindow::exportSelectedMeasurementRequested, [this]() {
        exportSelectedMeasurement();
    });

    QObject::connect(&window_, &MainWindow::clearAllMeasurementsRequested, [this]() {
        clearAllMeasurements();
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

    QObject::connect(&window_, &MainWindow::openProjectRequested, [this]() {
        openProjectWithDialog();
    });

    QObject::connect(&window_, &MainWindow::saveProjectRequested, [this]() {
        saveProjectWithDialog();
    });

    QObject::connect(&window_, &MainWindow::saveProjectAsRequested, [this]() {
        saveProjectAsWithDialog();
    });

    QObject::connect(window_.globeWidget(), &GlobeWidget::terrainPickCompleted, [this](const PickResult &pick) {
        handleTerrainPick(pick);
    });

    QObject::connect(window_.globeWidget(), &GlobeWidget::measurementCommitted, [this](const MeasurementLayerData &data) {
        addMeasurementLayer(data);
    });
}

void ApplicationController::importFile(const std::string &path) {
    std::vector<std::string> pathsToImport;
    const QString qPath = utf8(path);
    const QDir dir(qPath);

    if (dir.exists()) {
        const QFileInfoList vectorEntries = dir.entryInfoList(supportedDirectoryImportFilters(), QDir::Files);
        if (vectorEntries.isEmpty()) {
            window_.statusBar()->showMessage(
                QString::fromUtf8(u8"目录中未找到可导入的矢量数据文件。"),
                3000);
            QMessageBox::warning(
                &window_,
                QString::fromUtf8(u8"导入失败"),
                QString::fromUtf8(u8"目录中未找到支持的矢量文件：\n%1\n\n支持格式：Shapefile (.shp)、GeoJSON (.geojson/.json)、GeoPackage (.gpkg)、KML (.kml)、GML (.gml)")
                    .arg(qPath));
            return;
        }
        for (const QFileInfo &fi : vectorEntries) {
            pathsToImport.push_back(fi.absoluteFilePath().toUtf8().toStdString());
        }
    } else {
        pathsToImport.push_back(path);
    }

    const bool batchImport = pathsToImport.size() > 1;
    int importedCount = 0;
    int skippedCount = 0;

    for (const auto &importPath : pathsToImport) {
        spdlog::info("ApplicationController: importing '{}'", importPath);
        window_.statusBar()->showMessage(
            QString::fromUtf8(u8"正在导入 %1...").arg(utf8(importPath)));

        const auto layer = importer_.import(importPath);
        if (!layer) {
            spdlog::warn("ApplicationController: import failed for '{}'", importPath);
            ++skippedCount;
            if (batchImport) {
                window_.statusBar()->showMessage(
                    QString::fromUtf8(u8"已跳过无法导入的文件：%1").arg(utf8(importPath)),
                    3000);
                continue;
            }
            window_.statusBar()->showMessage(QString::fromUtf8(u8"导入失败。"), 3000);
            QMessageBox::warning(
                &window_,
                QString::fromUtf8(u8"导入失败"),
                QString::fromUtf8(u8"无法导入或当前版本暂不支持显示：\n%1\n\n支持格式包括：GeoTIFF/TIFF、IMG、ASC、SRTM/HGT、DEM、VRT、Shapefile、GeoJSON/JSON、GeoPackage、KML、GML，以及 %2 三维模型。")
                    .arg(utf8(importPath), modelFormatText()));
            return;
        }

        if (!layerManager_.addLayer(layer)) {
            spdlog::warn("ApplicationController: duplicate layer for '{}'", importPath);
            ++skippedCount;
            if (batchImport) {
                window_.statusBar()->showMessage(
                    QString::fromUtf8(u8"已跳过重复图层：%1").arg(utf8(importPath)),
                    3000);
                continue;
            }
            window_.statusBar()->showMessage(QString::fromUtf8(u8"图层已存在。"), 3000);
            QMessageBox::information(
                &window_,
                QString::fromUtf8(u8"图层已存在"),
                QString::fromUtf8(u8"该文件已经导入：\n%1").arg(utf8(importPath)));
            return;
        }

        sceneController_.addLayer(layer);
        window_.addLayerRow(*layer);
        addToRecentFiles(utf8(importPath));
        spdlog::info("ApplicationController: layer '{}' imported successfully", layer->id());
        ++importedCount;

        window_.statusBar()->showMessage(
            QString::fromUtf8(u8"已导入：%1 (%2)").arg(utf8(layer->name())).arg(layerKindToText(layer->kind())),
            5000);

        if (const auto bounds = layer->geographicBounds(); bounds.has_value() && bounds->isValid()) {
            sceneController_.flyToGeographicBounds(*bounds);
        }
    }

    if (batchImport) {
        if (importedCount > 0 && skippedCount > 0) {
            window_.statusBar()->showMessage(
                QString::fromUtf8(u8"批量导入完成：成功 %1，跳过 %2。").arg(importedCount).arg(skippedCount),
                5000);
        } else if (importedCount > 0) {
            window_.statusBar()->showMessage(
                QString::fromUtf8(u8"批量导入完成：成功导入 %1 个图层。").arg(importedCount),
                5000);
        } else if (skippedCount > 0) {
            window_.statusBar()->showMessage(
                QString::fromUtf8(u8"批量导入完成：未导入新图层，已跳过 %1 个文件。").arg(skippedCount),
                5000);
        }
    }
}

void ApplicationController::showLayerDetails(const std::string &layerId) {
    const auto layer = layerManager_.findById(layerId);
    if (!layer) {
        sceneController_.setSelectedLayer({});
        window_.clearLayerSelection();
        window_.showLayerDetails(QString::fromUtf8(u8"未找到图层。"));
        window_.clearLayerProperties();
        return;
    }

    sceneController_.setSelectedLayer(layerId);
    window_.showLayerProperties(
        utf8(layerId),
        utf8(layer->name()),
        layerKindToText(layer->kind()),
        utf8(layer->sourceUri()),
        layer->visible(),
        layer->opacity(),
        layer->rasterMetadata(),
        layer->vectorMetadata(),
        layer->modelPlacement(),
        layer->measurementData());
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
        sceneController_.setSelectedLayer({});
        window_.clearLayerSelection();
        window_.showLayerDetails(QString::fromUtf8(u8"拾取：光标下无地形。"));
        return;
    }

    QStringList lines;
    lines.append(QString::fromUtf8(u8"经度：%1").arg(pick.longitude, 0, 'f', 6));
    lines.append(QString::fromUtf8(u8"纬度：%1").arg(pick.latitude, 0, 'f', 6));
    lines.append(QString::fromUtf8(u8"高程(近似)：%1").arg(pick.elevation, 0, 'f', 2));
    if (!pick.layerId.empty()) {
        sceneController_.setSelectedLayer(pick.layerId);
        lines.append(QString::fromUtf8(u8"图层：%1").arg(utf8(pick.displayText)));
    }

    if (!pick.featureAttributes.empty()) {
        lines.append("");
        lines.append(QString::fromUtf8(u8"--- 要素属性 ---"));
        for (const auto &attr : pick.featureAttributes) {
            lines.append(QString::fromUtf8(u8"  %1：%2").arg(utf8(attr.name)).arg(utf8(attr.value)));
        }
    }

    window_.showLayerDetails(lines.join('\n'));
}

void ApplicationController::removeLayer(const std::string &layerId) {
    spdlog::info("ApplicationController: removing layer '{}'", layerId);
    const auto layer = layerManager_.findById(layerId);
    sceneController_.setSelectedLayer({});
    sceneController_.removeLayer(layerId);
    layerManager_.removeLayer(layerId);
    window_.removeLayerRow(layerId);
    if (layer && layer->kind() == LayerKind::Measurement) {
        QFile::remove(utf8(layer->sourceUri()));
    }
    window_.showLayerDetails(QString::fromUtf8(u8"未选择图层。"));
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

bool ApplicationController::updateMeasurementLayer(const std::shared_ptr<Layer> &layer, const MeasurementLayerData &data) {
    if (!layer || layer->kind() != LayerKind::Measurement) {
        return false;
    }

    const auto existingMeasurementData = layer->measurementData();
    const std::string existingName = layer->name();

    if (!writeMeasurementGeoJson(utf8(layer->sourceUri()), data)) {
        return false;
    }

    MeasurementLayerData storedData = data;
    storedData.targetLayerId.clear();
    layer->setMeasurementData(storedData);
    std::string resolvedName = existingName;
    if (!existingMeasurementData.has_value() ||
        existingName == measurementLayerDisplayName(layerManager_, *layer, *existingMeasurementData).toUtf8().toStdString()) {
        resolvedName = measurementLayerDisplayName(layerManager_, *layer, storedData).toUtf8().toStdString();
    }
    layer->setName(resolvedName);
    if (const auto bounds = measurementBounds(storedData); bounds.has_value()) {
        layer->setGeographicBounds(*bounds);
    }

    sceneController_.removeLayer(layer->id());
    sceneController_.addLayer(layer);
    window_.renameLayerRow(layer->id(), layer->name());
    return true;
}

void ApplicationController::addMeasurementLayer(const MeasurementLayerData &data) {
    const std::size_t minPointCount = data.kind == MeasurementKind::Area ? 3u : 2u;
    if (data.points.size() < minPointCount) {
        window_.statusBar()->showMessage(QString::fromUtf8(u8"量测点数不足，未生成结果。"), 3000);
        return;
    }

    if (!data.targetLayerId.empty()) {
        const auto layer = layerManager_.findById(data.targetLayerId);
        if (!layer || layer->kind() != LayerKind::Measurement) {
            window_.statusBar()->showMessage(QString::fromUtf8(u8"量测编辑失败：未找到原结果。"), 3000);
            return;
        }

        if (!updateMeasurementLayer(layer, data)) {
            window_.statusBar()->showMessage(QString::fromUtf8(u8"量测编辑保存失败。"), 3000);
            return;
        }

        window_.selectLayerRow(layer->id());
        showLayerDetails(layer->id());
        window_.statusBar()->showMessage(QString::fromUtf8(u8"量测结果已更新。"), 3000);
        return;
    }

    const QString layerId = QString("measurement-%1").arg(QDateTime::currentMSecsSinceEpoch());
    const QString sourcePath = measurementTempPath(layerId);
    if (!writeMeasurementGeoJson(sourcePath, data)) {
        window_.statusBar()->showMessage(QString::fromUtf8(u8"量测结果保存失败。"), 3000);
        return;
    }

    const QString layerName = measurementLayerDisplayName(data.kind, nextMeasurementIndex(layerManager_, data.kind), data);

    auto layer = std::make_shared<Layer>(
        layerId.toStdString(),
        layerName.toUtf8().toStdString(),
        sourcePath.toUtf8().toStdString(),
        LayerKind::Measurement);
    MeasurementLayerData storedData = data;
    storedData.targetLayerId.clear();
    layer->setMeasurementData(storedData);
    if (const auto bounds = measurementBounds(storedData); bounds.has_value()) {
        layer->setGeographicBounds(*bounds);
    }

    if (!layerManager_.addLayer(layer)) {
        QFile::remove(sourcePath);
        window_.statusBar()->showMessage(QString::fromUtf8(u8"量测结果添加失败。"), 3000);
        return;
    }

    sceneController_.addLayer(layer);
    window_.addLayerRow(*layer);
    window_.selectLayerRow(layer->id());
    showLayerDetails(layer->id());
    window_.statusBar()->showMessage(
        data.kind == MeasurementKind::Area
            ? QString::fromUtf8(u8"测面结果已保留。")
            : QString::fromUtf8(u8"测距结果已保留。"),
        3000);
}

void ApplicationController::editSelectedMeasurement() {
    const QString currentLayerId = window_.currentLayerId();
    if (currentLayerId.isEmpty()) {
        window_.statusBar()->showMessage(QString::fromUtf8(u8"请先选择一条量测结果。"), 3000);
        return;
    }

    const auto layer = layerManager_.findById(currentLayerId.toStdString());
    if (!layer || layer->kind() != LayerKind::Measurement) {
        window_.statusBar()->showMessage(QString::fromUtf8(u8"当前选中的不是量测结果。"), 3000);
        return;
    }

    const auto measurementData = layer->measurementData();
    if (!measurementData.has_value()) {
        window_.statusBar()->showMessage(QString::fromUtf8(u8"该条量测数据不完整，无法继续编辑。"), 3000);
        return;
    }

    MeasurementLayerData editable = *measurementData;
    editable.targetLayerId = layer->id();
    window_.setActiveToolAction(editable.kind == MeasurementKind::Area
                                    ? static_cast<int>(ToolId::MeasureArea)
                                    : static_cast<int>(ToolId::Measure));
    window_.globeWidget()->toolManager().startMeasurementEditing(*window_.globeWidget(), editable);
    window_.statusBar()->showMessage(
        editable.kind == MeasurementKind::Area
            ? QString::fromUtf8(u8"已进入测面编辑状态。")
            : QString::fromUtf8(u8"已进入测距编辑状态。"),
        3000);
}

void ApplicationController::exportSelectedMeasurement() {
    const QString currentLayerId = window_.currentLayerId();
    if (currentLayerId.isEmpty()) {
        window_.statusBar()->showMessage(QString::fromUtf8(u8"请先选择一条量测结果。"), 3000);
        return;
    }

    const auto layer = layerManager_.findById(currentLayerId.toStdString());
    if (!layer || layer->kind() != LayerKind::Measurement) {
        window_.statusBar()->showMessage(QString::fromUtf8(u8"当前选中的不是量测结果。"), 3000);
        return;
    }

    const auto measurementData = layer->measurementData();
    if (!measurementData.has_value()) {
        window_.statusBar()->showMessage(QString::fromUtf8(u8"量测结果数据不完整，无法导出。"), 3000);
        return;
    }

    const QString defaultPath = QDir::home().filePath(safeExportBaseName(utf8(layer->name())) + ".geojson");
    const QString path = QFileDialog::getSaveFileName(
        &window_,
        QString::fromUtf8(u8"导出量测结果"),
        defaultPath,
        QString::fromUtf8(u8"GeoJSON (*.geojson)"));
    if (path.isEmpty()) {
        return;
    }

    QString normalizedPath = path;
    if (!normalizedPath.endsWith(".geojson", Qt::CaseInsensitive)) {
        normalizedPath += ".geojson";
    }

    if (!writeMeasurementGeoJson(normalizedPath, *measurementData)) {
        window_.statusBar()->showMessage(QString::fromUtf8(u8"量测结果导出失败。"), 3000);
        return;
    }

    window_.statusBar()->showMessage(
        QString::fromUtf8(u8"量测结果已导出：%1").arg(normalizedPath),
        5000);
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
        window_.statusBar()->showMessage(QString::fromUtf8(u8"截图失败：无法捕获画面。"), 3000);
        return;
    }

    QString selectedFilter = QString::fromUtf8(u8"PNG 图像 (*.png)");
    const QString path = QFileDialog::getSaveFileName(
        &window_,
        QString::fromUtf8(u8"保存截图"),
        screenshot::defaultSavePath(),
        QString::fromUtf8(u8"PNG 图像 (*.png);;JPEG 图像 (*.jpg);;BMP 图像 (*.bmp)"),
        &selectedFilter);
    if (path.isEmpty()) {
        return;
    }

    const QString normalizedPath = screenshot::normalizeSavePath(path, selectedFilter);
    if (image.save(normalizedPath)) {
        spdlog::info("ApplicationController: screenshot saved to '{}'", normalizedPath.toStdString());
        window_.statusBar()->showMessage(
            QString::fromUtf8(u8"截图已保存：%1").arg(normalizedPath),
            5000);
    } else {
        spdlog::warn("ApplicationController: failed to save screenshot to '{}'", normalizedPath.toStdString());
        window_.statusBar()->showMessage(QString::fromUtf8(u8"截图保存失败。"), 3000);
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
    resourceDir_ = resourceDir;
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
                QString::fromUtf8(u8"已跳过未支持的持久化图层：%1").arg(QString::fromStdString(entry.name)),
                5000);
            continue;
        }

        std::string resolvedPath = entry.path;
        if (!resolvedPath.empty() && !(resolvedPath.size() >= 2 && resolvedPath[1] == ':') &&
            resolvedPath[0] != '/' && resolvedPath[0] != '\\') {
            resolvedPath = resourceDir + "/" + resolvedPath;
        }

        spdlog::info("ApplicationController: loading persisted layer '{}' from '{}'", entry.id, resolvedPath);

        const auto layer = importer_.import(resolvedPath);
        if (!layer) {
            spdlog::warn("ApplicationController: failed to load persisted layer '{}'", entry.id);
            window_.statusBar()->showMessage(
                QString::fromUtf8(u8"无法加载持久化图层：%1").arg(QString::fromStdString(entry.name)),
                5000);
            continue;
        }

        if (!entry.id.empty()) {
            layer->setId(entry.id);
        }
        if (!entry.name.empty()) {
            layer->setName(entry.name);
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
        if (layer->kind() == LayerKind::Measurement) {
            continue;
        }

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
        case LayerKind::Measurement: break;
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

void ApplicationController::renameLayer(const std::string &layerId, const QString &name) {
    const auto layer = layerManager_.findById(layerId);
    if (!layer) {
        return;
    }

    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        window_.renameLayerRow(layerId, layer->name());
        return;
    }

    layer->setName(trimmed.toStdString());
    window_.renameLayerRow(layerId, layer->name());
    if (window_.currentLayerId().toStdString() == layerId) {
        showLayerDetails(layerId);
    }
}

bool ApplicationController::saveProject(const QString &path) {
    ProjectConfig config;
    config.version = 1;
    config.basemapId = sceneController_.currentBasemapId();
    config.cameraState = sceneController_.currentCameraState();

    for (const auto &layer : layerManager_.layers()) {
        if (!layer) {
            continue;
        }

        ProjectLayerEntry entry;
        entry.id = layer->id();
        entry.name = layer->name();
        entry.kind = serializeLayerKind(layer->kind());
        entry.visible = layer->visible();
        entry.opacity = layer->opacity();

        if (layer->kind() != LayerKind::Measurement) {
            entry.path = layer->sourceUri();
        }

        if (layer->kind() == LayerKind::Imagery) {
            const auto rm = layer->rasterMetadata();
            if (rm.has_value() && rm->bandCount >= 2) {
                entry.bandMapping = BandMappingEntry{layer->redBand(), layer->greenBand(), layer->blueBand()};
            }
        }

        if (layer->kind() == LayerKind::Model) {
            const auto placement = layer->modelPlacement();
            if (placement.has_value()) {
                entry.modelPlacement = ModelPlacementEntry{
                    placement->longitude,
                    placement->latitude,
                    placement->height,
                    placement->scale,
                    placement->heading
                };
            }
        }

        if (layer->kind() == LayerKind::Measurement) {
            const auto measurementData = layer->measurementData();
            if (measurementData.has_value()) {
                entry.measurementData = toProjectMeasurementData(*measurementData);
            }
        }

        config.layers.push_back(entry);
    }

    if (!saveProjectConfig(path, config)) {
        return false;
    }

    currentProjectPath_ = path;
    return true;
}

bool ApplicationController::openProject(const QString &path) {
    const auto projectConfig = loadProjectConfig(path);
    if (!projectConfig.has_value()) {
        return false;
    }

    clearCurrentProject();

    if (!resourceDir_.empty()) {
        auto basemapConfig = loadBasemapConfig(resourceDir_);
        if (basemapConfig.has_value()) {
            sceneController_.setBasemapConfig(*basemapConfig, resourceDir_, projectConfig->basemapId);
        }
    }

    for (const auto &entry : projectConfig->layers) {
        const auto kind = parseLayerKind(entry.kind);
        if (!kind.has_value()) {
            continue;
        }

        std::shared_ptr<Layer> layer;
        if (*kind == LayerKind::Measurement && entry.measurementData.has_value()) {
            const MeasurementLayerData measurementData = toMeasurementLayerData(*entry.measurementData);
            layer = std::make_shared<Layer>(
                entry.id,
                entry.name,
                measurementTempPath(QString::fromStdString(entry.id)).toStdString(),
                LayerKind::Measurement);
            if (!writeMeasurementGeoJson(QString::fromStdString(layer->sourceUri()), measurementData)) {
                continue;
            }
            layer->setMeasurementData(measurementData);
            if (const auto bounds = measurementBounds(measurementData); bounds.has_value()) {
                layer->setGeographicBounds(*bounds);
            }
        } else {
            const std::string resolvedPath = resolveProjectPath(path, entry.path);
            layer = importer_.import(resolvedPath);
            if (!layer) {
                continue;
            }
            layer->setId(entry.id);
            layer->setName(entry.name);
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

        if (!layerManager_.addLayer(layer)) {
            continue;
        }

        sceneController_.addLayer(layer);
        window_.addLayerRow(*layer);
    }

    if (projectConfig->cameraState.has_value()) {
        sceneController_.applyCameraState(*projectConfig->cameraState);
    }

    currentProjectPath_ = path;
    return true;
}

void ApplicationController::saveProjectWithDialog() {
    if (!currentProjectPath_.isEmpty()) {
        if (saveProject(currentProjectPath_)) {
            window_.statusBar()->showMessage(QString::fromUtf8(u8"工程已保存。"), 3000);
        }
        return;
    }

    saveProjectAsWithDialog();
}

void ApplicationController::saveProjectAsWithDialog() {
    const QString defaultPath = currentProjectPath_.isEmpty()
        ? QDir::home().filePath(QString::fromUtf8(u8"未命名工程.3dproj"))
        : currentProjectPath_;
    const QString path = QFileDialog::getSaveFileName(
        &window_,
        QString::fromUtf8(u8"保存工程"),
        defaultPath,
        QString::fromUtf8(u8"3DViewer 工程 (*.3dproj)"));
    if (path.isEmpty()) {
        return;
    }

    QString normalizedPath = path;
    if (!normalizedPath.endsWith(".3dproj", Qt::CaseInsensitive)) {
        normalizedPath += ".3dproj";
    }

    if (saveProject(normalizedPath)) {
        window_.statusBar()->showMessage(QString::fromUtf8(u8"工程已保存。"), 3000);
    }
}

void ApplicationController::openProjectWithDialog() {
    const QString path = QFileDialog::getOpenFileName(
        &window_,
        QString::fromUtf8(u8"打开工程"),
        currentProjectPath_.isEmpty() ? QDir::homePath() : QFileInfo(currentProjectPath_).absolutePath(),
        QString::fromUtf8(u8"3DViewer 工程 (*.3dproj)"));
    if (path.isEmpty()) {
        return;
    }

    if (openProject(path)) {
        window_.statusBar()->showMessage(QString::fromUtf8(u8"工程已打开。"), 3000);
    }
}

void ApplicationController::clearCurrentProject() {
    const auto layers = layerManager_.layers();
    std::vector<std::string> ids;
    ids.reserve(layers.size());
    for (const auto &layer : layers) {
        if (layer) {
            ids.push_back(layer->id());
        }
    }

    for (const auto &id : ids) {
        removeLayer(id);
    }
}

void ApplicationController::clearAllMeasurements() {
    const auto layers = layerManager_.layers();
    std::vector<std::string> measurementIds;
    measurementIds.reserve(layers.size());
    for (const auto &layer : layers) {
        if (layer && layer->kind() == LayerKind::Measurement) {
            measurementIds.push_back(layer->id());
        }
    }

    for (const auto &id : measurementIds) {
        removeLayer(id);
    }
}
