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

    const QJsonObject mp = obj["modelPlacement"].toObject();
    if (!mp.isEmpty()) {
        ModelPlacementEntry placement;
        placement.longitude = mp["longitude"].toDouble(0.0);
        placement.latitude = mp["latitude"].toDouble(0.0);
        placement.height = mp["height"].toDouble(0.0);
        placement.scale = mp["scale"].toDouble(1.0);
        placement.heading = mp["heading"].toDouble(0.0);
        entry.modelPlacement = placement;
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

    if (entry.modelPlacement.has_value()) {
        QJsonObject mp;
        mp["longitude"] = entry.modelPlacement->longitude;
        mp["latitude"] = entry.modelPlacement->latitude;
        mp["height"] = entry.modelPlacement->height;
        mp["scale"] = entry.modelPlacement->scale;
        mp["heading"] = entry.modelPlacement->heading;
        obj["modelPlacement"] = mp;
    } else {
        obj["modelPlacement"] = QJsonValue::Null;
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
