#pragma once

#include <string>
#include <vector>

#include "globe/MeasurementUtils.h"

enum class MeasurementKind {
    Distance,
    Area,
};

struct MeasurementLayerData {
    MeasurementKind kind = MeasurementKind::Distance;
    std::string targetLayerId;
    std::vector<globe::MeasurementPoint> points;
    double lengthMeters = 0.0;
    double areaSquareMeters = 0.0;
};
