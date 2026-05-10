#pragma once

#include <optional>
#include <QString>
#include <vector>

#include "data/LayerConfig.h"

struct ProjectCameraState {
    double longitude = 0.0;
    double latitude = 0.0;
    double altitude = 0.0;
    double heading = 0.0;
    double pitch = -90.0;
    double range = 0.0;
};

struct ProjectMeasurementPoint {
    double longitude = 0.0;
    double latitude = 0.0;
};

struct ProjectMeasurementData {
    std::string kind;
    std::vector<ProjectMeasurementPoint> points;
    double lengthMeters = 0.0;
    double areaSquareMeters = 0.0;
};

struct ProjectLayerEntry {
    std::string id;
    std::string name;
    std::string path;
    std::string kind;
    bool visible = true;
    double opacity = 1.0;
    std::optional<BandMappingEntry> bandMapping;
    std::optional<ModelPlacementEntry> modelPlacement;
    std::optional<ProjectMeasurementData> measurementData;
};

struct ProjectConfig {
    int version = 1;
    std::string basemapId;
    std::optional<ProjectCameraState> cameraState;
    std::vector<ProjectLayerEntry> layers;
};

std::optional<ProjectConfig> loadProjectConfig(const QString &projectPath);
bool saveProjectConfig(const QString &projectPath, const ProjectConfig &config);
