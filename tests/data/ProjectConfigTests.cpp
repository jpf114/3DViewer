#include <cstdlib>
#include <cmath>

#include <QDir>
#include <QTemporaryDir>

#include "data/ProjectConfig.h"

namespace {

bool nearlyEqual(double lhs, double rhs) {
    return std::abs(lhs - rhs) <= 1.0e-9;
}

}

int main() {
    QTemporaryDir dir;
    if (!dir.isValid()) {
        return EXIT_FAILURE;
    }

    const QString projectPath = dir.filePath("sample.3dproj");
    const QString imageryDir = dir.filePath("imagery");
    const QString modelDir = dir.filePath("models");
    if (!QDir().mkpath(imageryDir) || !QDir().mkpath(modelDir)) {
        return EXIT_FAILURE;
    }

    ProjectConfig config;
    config.version = 1;
    config.basemapId = "tdt_img";
    config.cameraState = ProjectCameraState{
        120.123456, 30.654321, 1234.5, 15.0, -45.0, 6789.0
    };

    ProjectLayerEntry imagery;
    imagery.id = "image-1";
    imagery.name = "Imagery";
    imagery.kind = "imagery";
    imagery.path = imageryDir.toStdString() + "/sample.tif";
    imagery.visible = false;
    imagery.opacity = 0.45;
    imagery.bandMapping = BandMappingEntry{4, 3, 2};
    config.layers.push_back(imagery);

    ProjectLayerEntry model;
    model.id = "model-1";
    model.name = "Building";
    model.kind = "model";
    model.path = modelDir.toStdString() + "/building.glb";
    model.modelPlacement = ModelPlacementEntry{120.12, 30.28, 15.0, 2.5, 45.0};
    config.layers.push_back(model);

    ProjectLayerEntry measurement;
    measurement.id = "measurement-1";
    measurement.name = "Measure 1";
    measurement.kind = "measurement";
    measurement.visible = true;
    measurement.opacity = 1.0;
    ProjectMeasurementData measurementData;
    measurementData.kind = "distance";
    measurementData.points.push_back(ProjectMeasurementPoint{120.0, 30.0});
    measurementData.points.push_back(ProjectMeasurementPoint{121.0, 31.0});
    measurementData.lengthMeters = 1234.0;
    measurement.measurementData = measurementData;
    config.layers.push_back(measurement);

    if (!saveProjectConfig(projectPath, config)) {
        return EXIT_FAILURE;
    }

    const auto loaded = loadProjectConfig(projectPath);
    if (!loaded.has_value()) {
        return EXIT_FAILURE;
    }

    if (loaded->version != 1 || loaded->basemapId != "tdt_img") {
        return EXIT_FAILURE;
    }

    if (!loaded->cameraState.has_value()) {
        return EXIT_FAILURE;
    }

    if (!nearlyEqual(loaded->cameraState->longitude, 120.123456) ||
        !nearlyEqual(loaded->cameraState->latitude, 30.654321) ||
        !nearlyEqual(loaded->cameraState->altitude, 1234.5) ||
        !nearlyEqual(loaded->cameraState->heading, 15.0) ||
        !nearlyEqual(loaded->cameraState->pitch, -45.0) ||
        !nearlyEqual(loaded->cameraState->range, 6789.0)) {
        return EXIT_FAILURE;
    }

    if (loaded->layers.size() != 3) {
        return EXIT_FAILURE;
    }

    const auto &loadedImagery = loaded->layers[0];
    if (loadedImagery.path != "imagery/sample.tif" ||
        loadedImagery.visible != false ||
        !loadedImagery.bandMapping.has_value() ||
        loadedImagery.bandMapping->red != 4 ||
        loadedImagery.bandMapping->green != 3 ||
        loadedImagery.bandMapping->blue != 2) {
        return EXIT_FAILURE;
    }

    const auto &loadedModel = loaded->layers[1];
    if (loadedModel.path != "models/building.glb" ||
        !loadedModel.modelPlacement.has_value() ||
        !nearlyEqual(loadedModel.modelPlacement->scale, 2.5) ||
        !nearlyEqual(loadedModel.modelPlacement->heading, 45.0)) {
        return EXIT_FAILURE;
    }

    const auto &loadedMeasurement = loaded->layers[2];
    if (!loadedMeasurement.measurementData.has_value() ||
        loadedMeasurement.measurementData->kind != "distance" ||
        loadedMeasurement.measurementData->points.size() != 2 ||
        !nearlyEqual(loadedMeasurement.measurementData->lengthMeters, 1234.0)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
