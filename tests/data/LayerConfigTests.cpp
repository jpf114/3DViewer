#include <cstdlib>

#include <QTemporaryDir>

#include "data/LayerConfig.h"

int main() {
    QTemporaryDir dir;
    if (!dir.isValid()) {
        return EXIT_FAILURE;
    }

    LayerConfig config;
    LayerEntry vectorEntry;
    vectorEntry.id = "roads-1";
    vectorEntry.name = "Roads";
    vectorEntry.path = "vector/roads.geojson";
    vectorEntry.kind = "vector";
    vectorEntry.visible = false;
    vectorEntry.opacity = 0.35;
    config.layers.push_back(vectorEntry);

    LayerEntry imageryEntry;
    imageryEntry.id = "image-1";
    imageryEntry.name = "Imagery";
    imageryEntry.path = "image/demo.tif";
    imageryEntry.kind = "imagery";
    imageryEntry.bandMapping = BandMappingEntry{4, 3, 2};
    config.layers.push_back(imageryEntry);

    if (!saveLayerConfig(dir.path().toStdString(), config)) {
        return EXIT_FAILURE;
    }

    const auto loaded = loadLayerConfig(dir.path().toStdString());
    if (!loaded.has_value() || loaded->layers.size() != 2) {
        return EXIT_FAILURE;
    }

    if (loaded->layers[0].id != "roads-1" || loaded->layers[0].visible != false) {
        return EXIT_FAILURE;
    }

    if (!loaded->layers[1].bandMapping.has_value()) {
        return EXIT_FAILURE;
    }

    if (loaded->layers[1].bandMapping->red != 4 ||
        loaded->layers[1].bandMapping->green != 3 ||
        loaded->layers[1].bandMapping->blue != 2) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
