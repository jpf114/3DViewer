#pragma once

#include <string>

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
};
