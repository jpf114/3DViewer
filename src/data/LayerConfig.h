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
