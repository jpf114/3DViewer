#include "data/ProjectConfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <spdlog/spdlog.h>

namespace {

QString normalizeProjectLayerPath(const QString &projectPath, const std::string &layerPath) {
    if (layerPath.empty()) {
        return {};
    }

    const QFileInfo projectInfo(projectPath);
    const QDir projectDir = projectInfo.dir();
    const QString qLayerPath = QString::fromStdString(layerPath);
    const QFileInfo layerInfo(qLayerPath);
    if (!layerInfo.isAbsolute()) {
        return QDir::cleanPath(qLayerPath);
    }

    const QString relativePath = projectDir.relativeFilePath(layerInfo.absoluteFilePath());
    if (!relativePath.startsWith("..")) {
        return QDir::cleanPath(relativePath);
    }

    return QDir::cleanPath(layerInfo.absoluteFilePath());
}

QJsonObject serializeBandMapping(const BandMappingEntry &bandMapping) {
    QJsonObject obj;
    obj["red"] = bandMapping.red;
    obj["green"] = bandMapping.green;
    obj["blue"] = bandMapping.blue;
    return obj;
}

BandMappingEntry parseBandMapping(const QJsonObject &obj) {
    BandMappingEntry bandMapping;
    bandMapping.red = obj["red"].toInt(1);
    bandMapping.green = obj["green"].toInt(2);
    bandMapping.blue = obj["blue"].toInt(3);
    return bandMapping;
}

QJsonObject serializeModelPlacement(const ModelPlacementEntry &placement) {
    QJsonObject obj;
    obj["longitude"] = placement.longitude;
    obj["latitude"] = placement.latitude;
    obj["height"] = placement.height;
    obj["scale"] = placement.scale;
    obj["heading"] = placement.heading;
    return obj;
}

ModelPlacementEntry parseModelPlacement(const QJsonObject &obj) {
    ModelPlacementEntry placement;
    placement.longitude = obj["longitude"].toDouble(0.0);
    placement.latitude = obj["latitude"].toDouble(0.0);
    placement.height = obj["height"].toDouble(0.0);
    placement.scale = obj["scale"].toDouble(1.0);
    placement.heading = obj["heading"].toDouble(0.0);
    return placement;
}

QJsonObject serializeCameraState(const ProjectCameraState &cameraState) {
    QJsonObject obj;
    obj["longitude"] = cameraState.longitude;
    obj["latitude"] = cameraState.latitude;
    obj["altitude"] = cameraState.altitude;
    obj["heading"] = cameraState.heading;
    obj["pitch"] = cameraState.pitch;
    obj["range"] = cameraState.range;
    return obj;
}

ProjectCameraState parseCameraState(const QJsonObject &obj) {
    ProjectCameraState cameraState;
    cameraState.longitude = obj["longitude"].toDouble(0.0);
    cameraState.latitude = obj["latitude"].toDouble(0.0);
    cameraState.altitude = obj["altitude"].toDouble(0.0);
    cameraState.heading = obj["heading"].toDouble(0.0);
    cameraState.pitch = obj["pitch"].toDouble(-90.0);
    cameraState.range = obj["range"].toDouble(0.0);
    return cameraState;
}

QJsonObject serializeMeasurementData(const ProjectMeasurementData &measurementData) {
    QJsonArray points;
    for (const auto &point : measurementData.points) {
        QJsonObject pointObj;
        pointObj["longitude"] = point.longitude;
        pointObj["latitude"] = point.latitude;
        points.append(pointObj);
    }

    QJsonObject obj;
    obj["kind"] = QString::fromStdString(measurementData.kind);
    obj["points"] = points;
    obj["lengthMeters"] = measurementData.lengthMeters;
    obj["areaSquareMeters"] = measurementData.areaSquareMeters;
    return obj;
}

ProjectMeasurementData parseMeasurementData(const QJsonObject &obj) {
    ProjectMeasurementData measurementData;
    measurementData.kind = obj["kind"].toString().toStdString();
    measurementData.lengthMeters = obj["lengthMeters"].toDouble(0.0);
    measurementData.areaSquareMeters = obj["areaSquareMeters"].toDouble(0.0);

    const QJsonArray points = obj["points"].toArray();
    measurementData.points.reserve(static_cast<std::size_t>(points.size()));
    for (const QJsonValue &value : points) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject pointObj = value.toObject();
        measurementData.points.push_back(ProjectMeasurementPoint{
            pointObj["longitude"].toDouble(0.0),
            pointObj["latitude"].toDouble(0.0)
        });
    }

    return measurementData;
}

QJsonObject serializeLayerEntry(const QString &projectPath, const ProjectLayerEntry &entry) {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(entry.id);
    obj["name"] = QString::fromStdString(entry.name);
    obj["path"] = normalizeProjectLayerPath(projectPath, entry.path);
    obj["kind"] = QString::fromStdString(entry.kind);
    obj["visible"] = entry.visible;
    obj["opacity"] = entry.opacity;

    if (entry.bandMapping.has_value()) {
        obj["bandMapping"] = serializeBandMapping(*entry.bandMapping);
    }
    if (entry.modelPlacement.has_value()) {
        obj["modelPlacement"] = serializeModelPlacement(*entry.modelPlacement);
    }
    if (entry.measurementData.has_value()) {
        obj["measurementData"] = serializeMeasurementData(*entry.measurementData);
    }

    return obj;
}

ProjectLayerEntry parseLayerEntry(const QJsonObject &obj) {
    ProjectLayerEntry entry;
    entry.id = obj["id"].toString().toStdString();
    entry.name = obj["name"].toString().toStdString();
    entry.path = QDir::cleanPath(obj["path"].toString()).toStdString();
    entry.kind = obj["kind"].toString().toStdString();
    entry.visible = obj["visible"].toBool(true);
    entry.opacity = obj["opacity"].toDouble(1.0);

    const QJsonObject bandMapping = obj["bandMapping"].toObject();
    if (!bandMapping.isEmpty()) {
        entry.bandMapping = parseBandMapping(bandMapping);
    }

    const QJsonObject modelPlacement = obj["modelPlacement"].toObject();
    if (!modelPlacement.isEmpty()) {
        entry.modelPlacement = parseModelPlacement(modelPlacement);
    }

    const QJsonObject measurementData = obj["measurementData"].toObject();
    if (!measurementData.isEmpty()) {
        entry.measurementData = parseMeasurementData(measurementData);
    }

    return entry;
}

} // namespace

std::optional<ProjectConfig> loadProjectConfig(const QString &projectPath) {
    QFile file(projectPath);
    if (!file.exists()) {
        spdlog::warn("ProjectConfig: project file not found '{}'", projectPath.toStdString());
        return std::nullopt;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::warn("ProjectConfig: failed to open '{}'", projectPath.toStdString());
        return std::nullopt;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        spdlog::warn("ProjectConfig: invalid JSON object '{}'", projectPath.toStdString());
        return std::nullopt;
    }

    const QJsonObject root = document.object();
    ProjectConfig config;
    config.version = root["version"].toInt(1);
    config.basemapId = root["basemapId"].toString().toStdString();

    const QJsonObject cameraState = root["cameraState"].toObject();
    if (!cameraState.isEmpty()) {
        config.cameraState = parseCameraState(cameraState);
    }

    const QJsonArray layers = root["layers"].toArray();
    config.layers.reserve(static_cast<std::size_t>(layers.size()));
    for (const QJsonValue &value : layers) {
        if (!value.isObject()) {
            continue;
        }
        config.layers.push_back(parseLayerEntry(value.toObject()));
    }

    return config;
}

bool saveProjectConfig(const QString &projectPath, const ProjectConfig &config) {
    QJsonObject root;
    root["version"] = config.version;
    root["basemapId"] = QString::fromStdString(config.basemapId);

    if (config.cameraState.has_value()) {
        root["cameraState"] = serializeCameraState(*config.cameraState);
    }

    QJsonArray layers;
    for (const auto &entry : config.layers) {
        layers.append(serializeLayerEntry(projectPath, entry));
    }
    root["layers"] = layers;

    const QFileInfo projectInfo(projectPath);
    QDir().mkpath(projectInfo.dir().absolutePath());

    QFile file(projectPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        spdlog::warn("ProjectConfig: failed to write '{}'", projectPath.toStdString());
        return false;
    }

    return file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) >= 0 && file.flush();
}
