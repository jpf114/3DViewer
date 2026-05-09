#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
#include <osgDB/Registry>

#include "globe/SceneController.h"
#include "layers/ElevationLayer.h"
#include "layers/ImageryLayer.h"
#include "layers/ModelLayer.h"
#include "layers/VectorLayer.h"

namespace {

std::filesystem::path findModelSample(const char *filename) {
    const std::filesystem::path cwd = std::filesystem::current_path();
    const std::filesystem::path candidates[] = {
        cwd / "data" / "model" / filename,
        cwd / ".." / "data" / "model" / filename,
        cwd / ".." / ".." / "data" / "model" / filename,
        cwd / ".." / ".." / ".." / "data" / "model" / filename,
    };

    for (const auto &candidate : candidates) {
        const auto normalized = candidate.lexically_normal();
        if (std::filesystem::exists(normalized)) {
            return normalized;
        }
    }

    return {};
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
    const bool objSupported = osgDB::Registry::instance()->getReaderWriterForExtension("obj") != nullptr;
    const bool stlSupported = osgDB::Registry::instance()->getReaderWriterForExtension("stl") != nullptr;
    const std::filesystem::path modelPath = objSupported ? findModelSample("Box.obj") : findModelSample("Box.stl");
    if ((objSupported || stlSupported) && modelPath.empty()) {
        std::cerr << "Expected model sample resource for current runtime-supported extension.\n";
        return EXIT_FAILURE;
    }

    const auto modelLayer = std::make_shared<ModelLayer>("model-1", "Model 1", modelPath.lexically_normal().string());
    ModelPlacement initialPlacement;
    initialPlacement.longitude = 120.25;
    initialPlacement.latitude = 30.5;
    initialPlacement.height = 150.0;
    initialPlacement.scale = 2.0;
    initialPlacement.heading = 30.0;
    modelLayer->setModelPlacement(initialPlacement);
    controller.addLayer(imageryLayer);
    controller.addLayer(elevationLayer);
    controller.addLayer(vectorLayer);
    controller.addLayer(modelLayer);

    const std::size_t minimumRenderedCount = objSupported ? 3u : 2u;
    if (controller.renderedLayerCount() < minimumRenderedCount) {
        std::cerr << "Expected scene controller to create the expected number of render layers.\n";
        return EXIT_FAILURE;
    }

    if (objSupported) {
        if (!modelLayer->geographicBounds().has_value() || !modelLayer->geographicBounds()->isValid()) {
            std::cerr << "Expected real OBJ model layer to produce valid geographic bounds.\n";
            return EXIT_FAILURE;
        }
        const auto initialBounds = *modelLayer->geographicBounds();
        const double initialCenterLon = (initialBounds.west + initialBounds.east) * 0.5;
        const double initialCenterLat = (initialBounds.south + initialBounds.north) * 0.5;
        if (std::abs(initialCenterLon - initialPlacement.longitude) > 0.01 ||
            std::abs(initialCenterLat - initialPlacement.latitude) > 0.01) {
            std::cerr << "Expected model bounds center to follow initial placement.\n";
            return EXIT_FAILURE;
        }

        if (!controller.isLayerVisibleInScene("model-1")) {
            std::cerr << "Expected real OBJ model layer to be visible in scene.\n";
            return EXIT_FAILURE;
        }
    } else if (stlSupported) {
        if (!modelLayer->geographicBounds().has_value() || !modelLayer->geographicBounds()->isValid()) {
            std::cerr << "Expected real STL model layer to produce valid geographic bounds.\n";
            return EXIT_FAILURE;
        }

        if (!controller.isLayerVisibleInScene("model-1")) {
            std::cerr << "Expected real STL model layer to be visible in scene.\n";
            return EXIT_FAILURE;
        }
        const auto initialBounds = *modelLayer->geographicBounds();
        const double initialCenterLon = (initialBounds.west + initialBounds.east) * 0.5;
        const double initialCenterLat = (initialBounds.south + initialBounds.north) * 0.5;
        if (std::abs(initialCenterLon - initialPlacement.longitude) > 0.01 ||
            std::abs(initialCenterLat - initialPlacement.latitude) > 0.01) {
            std::cerr << "Expected model bounds center to follow initial placement.\n";
            return EXIT_FAILURE;
        }
    } else if (controller.isLayerVisibleInScene("model-1")) {
        std::cerr << "Expected unsupported model layer to stay out of scene.\n";
        return EXIT_FAILURE;
    }

    imageryLayer->setVisible(false);
    controller.syncLayerState(imageryLayer);
    if (controller.isLayerVisibleInScene("imagery-1")) {
        std::cerr << "Expected scene controller visibility to follow app layer state.\n";
        return EXIT_FAILURE;
    }

    if (objSupported || stlSupported) {
        ModelPlacement updatedPlacement = initialPlacement;
        updatedPlacement.longitude = 121.75;
        updatedPlacement.latitude = 31.25;
        updatedPlacement.scale = 3.5;
        modelLayer->setModelPlacement(updatedPlacement);
        controller.syncLayerState(modelLayer);

        if (!modelLayer->geographicBounds().has_value() || !modelLayer->geographicBounds()->isValid()) {
            std::cerr << "Expected model placement sync to keep valid bounds.\n";
            return EXIT_FAILURE;
        }

        const auto updatedBounds = *modelLayer->geographicBounds();
        const double updatedCenterLon = (updatedBounds.west + updatedBounds.east) * 0.5;
        const double updatedCenterLat = (updatedBounds.south + updatedBounds.north) * 0.5;
        if (std::abs(updatedCenterLon - updatedPlacement.longitude) > 0.01 ||
            std::abs(updatedCenterLat - updatedPlacement.latitude) > 0.01) {
            std::cerr << "Expected model bounds center to follow updated placement.\n";
            return EXIT_FAILURE;
        }

        modelLayer->setVisible(false);
        controller.syncLayerState(modelLayer);
        if (controller.isLayerVisibleInScene("model-1")) {
            std::cerr << "Expected model visibility to follow app layer state.\n";
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
