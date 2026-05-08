#include "data/DataImporter.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>

#include "layers/ElevationLayer.h"
#include "layers/ImageryLayer.h"
#include "layers/ModelLayer.h"
#include "layers/VectorLayer.h"

namespace {

std::optional<DataSourceDescriptor> inspectModelSource(const std::string &path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        return std::nullopt;
    }

    const auto extension = std::filesystem::path(path).extension().string();
    std::string lowerExtension = extension;
    std::transform(lowerExtension.begin(), lowerExtension.end(), lowerExtension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (lowerExtension != ".gltf" && lowerExtension != ".glb") {
        return std::nullopt;
    }

    DataSourceDescriptor descriptor;
    descriptor.id = path;
    descriptor.name = std::filesystem::path(path).stem().string();
    descriptor.path = path;
    descriptor.kind = DataSourceKind::Model;
    descriptor.modelPlacement = ModelPlacement{};
    return descriptor;
}

bool isRenderableDataSourceKind(DataSourceKind kind) {
    switch (kind) {
    case DataSourceKind::RasterImagery:
    case DataSourceKind::RasterElevation:
    case DataSourceKind::Vector:
    case DataSourceKind::Model:
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
    case DataSourceKind::Model:
        return "Model";
    case DataSourceKind::Chart:
        return "Chart";
    case DataSourceKind::Scientific:
        return "Scientific";
    }

    return "Unknown";
}

} // namespace

std::shared_ptr<Layer> DataImporter::import(const std::string &path) const {
    if (const auto modelDescriptor = inspectModelSource(path); modelDescriptor.has_value()) {
        return import(*modelDescriptor);
    }

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
    case DataSourceKind::Model:
        layer = std::make_shared<ModelLayer>(descriptor.id, descriptor.name, descriptor.path);
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

    if (layer && descriptor.modelPlacement.has_value()) {
        layer->setModelPlacement(*descriptor.modelPlacement);
    }

    return layer;
}
