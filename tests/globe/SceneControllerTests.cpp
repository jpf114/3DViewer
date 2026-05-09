#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <osgDB/Registry>

#include "globe/SceneController.h"
#include "layers/ElevationLayer.h"
#include "layers/ImageryLayer.h"
#include "layers/ModelLayer.h"
#include "layers/VectorLayer.h"

namespace {

std::filesystem::path createTempObjModel() {
    const std::filesystem::path modelPath = std::filesystem::temp_directory_path() / "threeviewer-scene-test.obj";
    std::ofstream modelFile(modelPath);
    modelFile << "o quad\n";
    modelFile << "v 0 0 0\n";
    modelFile << "v 1 0 0\n";
    modelFile << "v 1 1 0\n";
    modelFile << "v 0 1 0\n";
    modelFile << "f 1 2 3\n";
    modelFile << "f 1 3 4\n";
    return modelPath;
}

} // namespace

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
    const std::filesystem::path modelPath = createTempObjModel();
    const auto modelLayer = std::make_shared<ModelLayer>("model-1", "Model 1", modelPath.lexically_normal().string());
    const bool objSupported = osgDB::Registry::instance()->getReaderWriterForExtension("obj") != nullptr;
    controller.addLayer(imageryLayer);
    controller.addLayer(elevationLayer);
    controller.addLayer(vectorLayer);
    controller.addLayer(modelLayer);

    const std::size_t minimumRenderedCount = objSupported ? 3u : 2u;
    if (controller.renderedLayerCount() < minimumRenderedCount) {
        std::cerr << "Expected scene controller to create the expected number of render layers.\n";
        std::filesystem::remove(modelPath);
        return EXIT_FAILURE;
    }

    if (objSupported) {
        if (!modelLayer->geographicBounds().has_value() || !modelLayer->geographicBounds()->isValid()) {
            std::cerr << "Expected real OBJ model layer to produce valid geographic bounds.\n";
            std::filesystem::remove(modelPath);
            return EXIT_FAILURE;
        }

        if (!controller.isLayerVisibleInScene("model-1")) {
            std::cerr << "Expected real OBJ model layer to be visible in scene.\n";
            std::filesystem::remove(modelPath);
            return EXIT_FAILURE;
        }
    } else if (controller.isLayerVisibleInScene("model-1")) {
        std::cerr << "Expected unsupported OBJ model layer to stay out of scene.\n";
        std::filesystem::remove(modelPath);
        return EXIT_FAILURE;
    }

    imageryLayer->setVisible(false);
    controller.syncLayerState(imageryLayer);
    if (controller.isLayerVisibleInScene("imagery-1")) {
        std::cerr << "Expected scene controller visibility to follow app layer state.\n";
        std::filesystem::remove(modelPath);
        return EXIT_FAILURE;
    }

    if (objSupported) {
        modelLayer->setVisible(false);
        controller.syncLayerState(modelLayer);
        if (controller.isLayerVisibleInScene("model-1")) {
            std::cerr << "Expected model visibility to follow app layer state.\n";
            std::filesystem::remove(modelPath);
            return EXIT_FAILURE;
        }
    }

    std::filesystem::remove(modelPath);
    return EXIT_SUCCESS;
}
