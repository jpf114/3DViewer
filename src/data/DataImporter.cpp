#include "data/DataImporter.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <osgDB/Registry>
#include <ranges>
#include <spdlog/spdlog.h>

#include "layers/ElevationLayer.h"
#include "layers/ImageryLayer.h"
#include "layers/ModelLayer.h"
#include "layers/VectorLayer.h"

namespace {

constexpr const char *kSupportedModelExtensions[] = {
    ".obj",
    ".stl",
    ".3ds",
};

std::string normalizedLowerExtension(const std::string &path) {
    const auto extension = std::filesystem::path(path).extension().string();
    std::string lowerExtension = extension;
    std::transform(lowerExtension.begin(), lowerExtension.end(), lowerExtension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lowerExtension;
}

bool isModelExtension(const std::string &path) {
    const auto lowerExtension = normalizedLowerExtension(path);
    return std::ranges::any_of(kSupportedModelExtensions, [&lowerExtension](const char *extension) {
        return lowerExtension == extension;
    });
}

bool isModelRuntimeSupportedExtension(const std::string &lowerExtension) {
    const std::string extensionWithoutDot = lowerExtension.starts_with('.')
        ? lowerExtension.substr(1)
        : lowerExtension;
    return !extensionWithoutDot.empty() &&
           osgDB::Registry::instance()->getReaderWriterForExtension(extensionWithoutDot) != nullptr;
}

bool isModelRuntimeSupported(const std::string &path) {
    return isModelRuntimeSupportedExtension(normalizedLowerExtension(path));
}

std::optional<DataSourceDescriptor> inspectModelSource(const std::string &path, bool runtimeSupported) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        return std::nullopt;
    }

    if (!isModelExtension(path) || !runtimeSupported) {
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

std::vector<std::string> DataImporter::supportedModelExtensions() {
    std::vector<std::string> extensions;
    extensions.reserve(std::size(kSupportedModelExtensions));
    for (const char *extension : kSupportedModelExtensions) {
        extensions.emplace_back(extension);
    }
    return extensions;
}

std::vector<std::string> DataImporter::availableRuntimeModelExtensions() {
    std::vector<std::string> extensions;
    for (const char *extension : kSupportedModelExtensions) {
        if (isModelRuntimeSupportedExtension(extension)) {
            extensions.emplace_back(extension);
        }
    }
    return extensions;
}

std::shared_ptr<Layer> DataImporter::import(const std::string &path) const {
    const bool modelPath = isModelExtension(path);
    const bool modelRuntimeSupported = modelPath && isModelRuntimeSupported(path);

    if (const auto modelDescriptor = inspectModelSource(path, modelRuntimeSupported); modelDescriptor.has_value()) {
        return import(*modelDescriptor);
    }

    if (modelPath && !modelRuntimeSupported) {
        spdlog::warn("DataImporter: model runtime reader is unavailable for '{}'", path);
        return nullptr;
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
