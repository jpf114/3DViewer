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

    LayerEntry modelEntry;
    modelEntry.id = "model-1";
    modelEntry.name = "Building";
    modelEntry.path = "model/building.glb";
    modelEntry.kind = "model";
    modelEntry.modelPlacement = ModelPlacementEntry{120.12, 30.28, 15.0, 2.5, 45.0};
    config.layers.push_back(modelEntry);

    if (!saveLayerConfig(dir.path().toStdString(), config)) {
        return EXIT_FAILURE;
    }

    const auto loaded = loadLayerConfig(dir.path().toStdString());
    if (!loaded.has_value() || loaded->layers.size() != 3) {
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

    if (!loaded->layers[2].modelPlacement.has_value()) {
        return EXIT_FAILURE;
    }

    if (loaded->layers[2].modelPlacement->scale != 2.5 ||
        loaded->layers[2].modelPlacement->heading != 45.0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
