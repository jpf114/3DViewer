#include "data/DataImporter.h"

#include <memory>

#include "layers/ElevationLayer.h"
#include "layers/ImageryLayer.h"
#include "layers/VectorLayer.h"

std::shared_ptr<Layer> DataImporter::import(const std::string &path) const {
    const auto descriptor = inspector_.inspect(path);
    if (!descriptor.has_value()) {
        return nullptr;
    }

    return import(*descriptor);
}

std::shared_ptr<Layer> DataImporter::import(const DataSourceDescriptor &descriptor) const {
    switch (descriptor.kind) {
    case DataSourceKind::RasterImagery:
        return std::make_shared<ImageryLayer>(descriptor.id, descriptor.name, descriptor.path);
    case DataSourceKind::RasterElevation:
        return std::make_shared<ElevationLayer>(descriptor.id, descriptor.name, descriptor.path);
    case DataSourceKind::Vector:
        return std::make_shared<VectorLayer>(descriptor.id, descriptor.name, descriptor.path);
    }

    return nullptr;
}
