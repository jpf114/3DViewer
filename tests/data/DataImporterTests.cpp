#include <cstdlib>
#include <iostream>

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

    if (!verifyKind(importer, DataSourceKind::Chart, LayerKind::Chart, "chart")) {
        return EXIT_FAILURE;
    }

    if (!verifyKind(importer, DataSourceKind::Scientific, LayerKind::Scientific, "scientific")) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
