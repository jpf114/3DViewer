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
