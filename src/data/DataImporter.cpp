#include "data/DataImporter.h"

#include <memory>
#include <spdlog/spdlog.h>

#include "layers/ElevationLayer.h"
#include "layers/ImageryLayer.h"
#include "layers/VectorLayer.h"

std::shared_ptr<Layer> DataImporter::import(const std::string &path) const {
    const auto descriptor = inspector_.inspect(path);
    if (!descriptor.has_value()) {
        spdlog::warn("DataImporter: failed to inspect path: {}", path);
        return nullptr;
    }

    return import(*descriptor);
}

std::shared_ptr<Layer> DataImporter::import(const DataSourceDescriptor &descriptor) const {
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
    }

    if (layer && descriptor.geographicBounds.has_value() && descriptor.geographicBounds->isValid()) {
        layer->setGeographicBounds(*descriptor.geographicBounds);
    }

    return layer;
}
