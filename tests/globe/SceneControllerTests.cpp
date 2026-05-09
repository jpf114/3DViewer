#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <osgDB/Registry>

#include "globe/SceneController.h"
#include "layers/ElevationLayer.h"
#include "layers/ImageryLayer.h"
#include "layers/ModelLayer.h"
#include "layers/VectorLayer.h"

namespace {

std::filesystem::path resolveSampleModelPath() {
    const std::filesystem::path cwd = std::filesystem::current_path();
    const std::filesystem::path candidates[] = {
        cwd / "data" / "model" / "Box.glb",
        cwd / ".." / "data" / "model" / "Box.glb",
        cwd / ".." / ".." / "data" / "model" / "Box.glb",
        cwd / ".." / ".." / ".." / "data" / "model" / "Box.glb",
    };

    for (const auto &candidate : candidates) {
        const auto normalized = candidate.lexically_normal();
        if (std::filesystem::exists(normalized)) {
            return normalized;
        }
    }

    return (cwd / ".." / ".." / ".." / "data" / "model" / "Box.glb").lexically_normal();
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
    const std::filesystem::path modelPath = resolveSampleModelPath();
    if (!std::filesystem::exists(modelPath)) {
        std::cerr << "Expected Box.glb sample model at: " << modelPath.string() << "\n";
        return EXIT_FAILURE;
    }

    const auto modelLayer = std::make_shared<ModelLayer>("model-1", "Model 1", modelPath.lexically_normal().string());
    const bool glbSupported = osgDB::Registry::instance()->getReaderWriterForExtension("glb") != nullptr;
    controller.addLayer(imageryLayer);
    controller.addLayer(elevationLayer);
    controller.addLayer(vectorLayer);
    controller.addLayer(modelLayer);

    const std::size_t minimumRenderedCount = glbSupported ? 3u : 2u;
    if (controller.renderedLayerCount() < minimumRenderedCount) {
        std::cerr << "Expected scene controller to create the expected number of render layers.\n";
        return EXIT_FAILURE;
    }

    if (glbSupported) {
        if (!modelLayer->geographicBounds().has_value() || !modelLayer->geographicBounds()->isValid()) {
            std::cerr << "Expected real Box.glb model layer to produce valid geographic bounds.\n";
            return EXIT_FAILURE;
        }

        if (!controller.isLayerVisibleInScene("model-1")) {
            std::cerr << "Expected real Box.glb model layer to be visible in scene.\n";
            return EXIT_FAILURE;
        }
    } else if (controller.isLayerVisibleInScene("model-1")) {
        std::cerr << "Expected unsupported Box.glb model layer to stay out of scene.\n";
        return EXIT_FAILURE;
    }

    imageryLayer->setVisible(false);
    controller.syncLayerState(imageryLayer);
    if (controller.isLayerVisibleInScene("imagery-1")) {
        std::cerr << "Expected scene controller visibility to follow app layer state.\n";
        return EXIT_FAILURE;
    }

    if (glbSupported) {
        modelLayer->setVisible(false);
        controller.syncLayerState(modelLayer);
        if (controller.isLayerVisibleInScene("model-1")) {
            std::cerr << "Expected model visibility to follow app layer state.\n";
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
