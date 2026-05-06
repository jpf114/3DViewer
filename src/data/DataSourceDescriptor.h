#pragma once

#include <optional>
#include <string>

#include "layers/GeographicBounds.h"

enum class DataSourceKind {
    RasterImagery,
    RasterElevation,
    Vector,
};

struct DataSourceDescriptor {
    std::string id;
    std::string name;
    std::string path;
    DataSourceKind kind;
    std::optional<GeographicBounds> geographicBounds;
};
