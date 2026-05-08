#include <cstdlib>
#include <iostream>

#include "globe/SceneController.h"
#include "layers/ElevationLayer.h"
#include "layers/ImageryLayer.h"
#include "layers/VectorLayer.h"

int main() {
    SceneController controller;
    controller.initializeDefaultScene(1280, 720);

    if (!controller.isInitialized()) {
        std::cerr << "Expected scene controller to initialize.\n";
        return EXIT_FAILURE;
    }

    if (!controller.hasViewer()) {
        std::cerr << "Expected scene controller to create an osgViewer instance.\n";
        return EXIT_FAILURE;
    }

    if (!controller.hasMapNode()) {
        std::cerr << "Expected scene controller to create an osgEarth map node.\n";
        return EXIT_FAILURE;
    }

    if (!controller.hasBaseLayer()) {
        std::cerr << "Expected scene controller to add a base imagery layer.\n";
        return EXIT_FAILURE;
    }

    const auto imageryLayer = std::make_shared<ImageryLayer>("imagery-1", "Imagery 1", "missing-imagery.tif");
    const auto elevationLayer = std::make_shared<ElevationLayer>("elevation-1", "Elevation 1", "missing-dem.tif");
    const auto vectorLayer = std::make_shared<VectorLayer>("vector-1", "Vector 1", "missing-vector.shp");
    controller.addLayer(imageryLayer);
    controller.addLayer(elevationLayer);
    controller.addLayer(vectorLayer);

    if (controller.renderedLayerCount() < 2) {
        std::cerr << "Expected scene controller to create at least two render layers.\n";
        return EXIT_FAILURE;
    }

    imageryLayer->setVisible(false);
    controller.syncLayerState(imageryLayer);
    if (controller.isLayerVisibleInScene("imagery-1")) {
        std::cerr << "Expected scene controller visibility to follow app layer state.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
