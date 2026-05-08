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
