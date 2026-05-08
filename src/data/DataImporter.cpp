#include "data/DataImporter.h"

#include <memory>
#include <spdlog/spdlog.h>

#include "layers/ElevationLayer.h"
#include "layers/ImageryLayer.h"
#include "layers/VectorLayer.h"

namespace {

bool isRenderableDataSourceKind(DataSourceKind kind) {
    switch (kind) {
    case DataSourceKind::RasterImagery:
    case DataSourceKind::RasterElevation:
    case DataSourceKind::Vector:
        return true;
    case DataSourceKind::Chart:
    case DataSourceKind::Scientific:
        return false;
    }

    return false;
}

const char *dataSourceKindName(DataSourceKind kind) {
    switch (kind) {
    case DataSourceKind::RasterImagery:
        return "RasterImagery";
    case DataSourceKind::RasterElevation:
        return "RasterElevation";
    case DataSourceKind::Vector:
        return "Vector";
    case DataSourceKind::Chart:
        return "Chart";
    case DataSourceKind::Scientific:
        return "Scientific";
    }

    return "Unknown";
}

} // namespace

std::shared_ptr<Layer> DataImporter::import(const std::string &path) const {
    const auto descriptor = inspector_.inspect(path);
    if (!descriptor.has_value()) {
        spdlog::warn("DataImporter: failed to inspect path: {}", path);
        return nullptr;
    }

    return import(*descriptor);
}

std::shared_ptr<Layer> DataImporter::import(const DataSourceDescriptor &descriptor) const {
    if (!isRenderableDataSourceKind(descriptor.kind)) {
        spdlog::warn("DataImporter: unsupported render kind '{}' for '{}'",
                     dataSourceKindName(descriptor.kind), descriptor.path);
        return nullptr;
    }

    std::shared_ptr<Layer> layer;
    switch (descriptor.kind) {
    case DataSourceKind::RasterImagery:
        layer = std::make_shared<ImageryLayer>(descriptor.id, descriptor.name, descriptor.path);
        break;
    case DataSourceKind::RasterElevation:
        layer = std::make_shared<ElevationLayer>(descriptor.id, descriptor.name, descriptor.path);
        break;
    case DataSourceKind::Vector:
        layer = std::make_shared<VectorLayer>(descriptor.id, descriptor.name, descriptor.path);
        break;
    case DataSourceKind::Chart:
    case DataSourceKind::Scientific:
        break;
    }

    if (layer && descriptor.geographicBounds.has_value() && descriptor.geographicBounds->isValid()) {
        layer->setGeographicBounds(*descriptor.geographicBounds);
    }

    if (layer && descriptor.rasterMetadata.has_value()) {
        layer->setRasterMetadata(*descriptor.rasterMetadata);
    }

    if (layer && descriptor.vectorMetadata.has_value()) {
        layer->setVectorMetadata(*descriptor.vectorMetadata);
    }

    return layer;
}
