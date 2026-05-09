#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <osgDB/Registry>

#include "data/DataImporter.h"
#include "data/DataSourceDescriptor.h"
#include "layers/Layer.h"

namespace {

bool verifyKind(DataImporter &importer,
                DataSourceKind descriptorKind,
                LayerKind expectedLayerKind,
                const char *label) {
    const DataSourceDescriptor descriptor{
        "sample-id",
        label,
        "memory://sample",
        descriptorKind,
    };

    const auto layer = importer.import(descriptor);
    if (!layer) {
        std::cerr << "Expected importer to create a layer for " << label << ".\n";
        return false;
    }

    if (layer->kind() != expectedLayerKind) {
        std::cerr << "Unexpected layer kind for " << label << ".\n";
        return false;
    }

    return true;
}

bool verifyUnsupported(DataImporter &importer,
                       DataSourceKind descriptorKind,
                       const char *label) {
    const DataSourceDescriptor descriptor{
        "sample-id",
        label,
        "memory://sample",
        descriptorKind,
    };

    if (importer.import(descriptor)) {
        std::cerr << "Expected importer to reject unsupported kind for " << label << ".\n";
        return false;
    }

    return true;
}

} // namespace

int main() {
    DataImporter importer;

    if (importer.import("definitely-missing-dataset.xyz")) {
        std::cerr << "Expected missing file import to fail.\n";
        return EXIT_FAILURE;
    }

    if (!verifyKind(importer, DataSourceKind::RasterImagery, LayerKind::Imagery, "imagery")) {
        return EXIT_FAILURE;
    }

    if (!verifyKind(importer, DataSourceKind::RasterElevation, LayerKind::Elevation, "elevation")) {
        return EXIT_FAILURE;
    }

    if (!verifyKind(importer, DataSourceKind::Vector, LayerKind::Vector, "vector")) {
        return EXIT_FAILURE;
    }

    if (!verifyKind(importer, DataSourceKind::Model, LayerKind::Model, "model")) {
        return EXIT_FAILURE;
    }

    const std::filesystem::path modelPath = std::filesystem::temp_directory_path() / "threeviewer-model-test.glb";
    {
        std::ofstream modelFile(modelPath, std::ios::binary);
        modelFile << "glb";
    }
    const auto modelLayer = importer.import(modelPath.string());
    std::filesystem::remove(modelPath);
    const bool glbSupported = osgDB::Registry::instance()->getReaderWriterForExtension("glb") != nullptr;
    if (glbSupported) {
        if (!modelLayer || modelLayer->kind() != LayerKind::Model) {
            std::cerr << "Expected .glb path import to create a model layer.\n";
            return EXIT_FAILURE;
        }
    } else if (modelLayer) {
        std::cerr << "Expected .glb path import to fail when runtime reader is unavailable.\n";
        return EXIT_FAILURE;
    }

    if (!verifyUnsupported(importer, DataSourceKind::Chart, "chart")) {
        return EXIT_FAILURE;
    }

    if (!verifyUnsupported(importer, DataSourceKind::Scientific, "scientific")) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
