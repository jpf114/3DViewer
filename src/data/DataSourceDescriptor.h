#pragma once

#include <optional>
#include <string>
#include <vector>

#include "layers/GeographicBounds.h"

enum class DataSourceKind {
    RasterImagery,
    RasterElevation,
    Vector,
    Model,
    Chart,
    Scientific,
};

struct ModelPlacement {
    double longitude = 0.0;
    double latitude = 0.0;
    double height = 0.0;
    double scale = 1.0;
    double heading = 0.0;
};

struct BandInfo {
    int index = 0;
    std::string description;
    std::string dataType;
    std::string colorInterpretation;
    double noDataValue = 0.0;
    bool hasNoDataValue = false;
    double min = 0.0;
    double max = 0.0;
    bool hasMinMax = false;
};

struct VectorFieldInfo {
    std::string name;
    std::string typeName;
};

struct VectorLayerInfo {
    std::string name;
    std::string geometryType;
    std::string epsgCode;
    int featureCount = 0;
    std::vector<VectorFieldInfo> fields;
};

struct RasterMetadata {
    int rasterXSize = 0;
    int rasterYSize = 0;
    int bandCount = 0;
    std::string driverName;
    std::string crsWkt;
    std::string epsgCode;
    double pixelSizeX = 0.0;
    double pixelSizeY = 0.0;
    std::vector<BandInfo> bands;
};

struct DataSourceDescriptor {
    std::string id;
    std::string name;
    std::string path;
    DataSourceKind kind;
    std::optional<GeographicBounds> geographicBounds;
    std::optional<RasterMetadata> rasterMetadata;
    std::optional<VectorLayerInfo> vectorMetadata;
    std::optional<ModelPlacement> modelPlacement;
};
