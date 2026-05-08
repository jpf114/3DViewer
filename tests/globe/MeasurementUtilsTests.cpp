#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "globe/MeasurementUtils.h"

namespace {

bool nearlyEqual(double lhs, double rhs, double tolerance) {
    return std::fabs(lhs - rhs) <= tolerance;
}

}

int main() {
    const globe::MeasurementPoint origin{0.0, 0.0};
    const globe::MeasurementPoint eastOneDegree{1.0, 0.0};
    const globe::MeasurementPoint northOneDegree{0.0, 1.0};

    const double eastDistance = globe::greatCircleDistanceMeters(origin, eastOneDegree);
    if (!nearlyEqual(eastDistance, 111195.0, 500.0)) {
        std::cerr << "Expected one degree east at equator to be about 111.2 km.\n";
        return EXIT_FAILURE;
    }

    const double northDistance = globe::greatCircleDistanceMeters(origin, northOneDegree);
    if (!nearlyEqual(northDistance, 111195.0, 500.0)) {
        std::cerr << "Expected one degree north to be about 111.2 km.\n";
        return EXIT_FAILURE;
    }

    const std::vector<globe::MeasurementPoint> polyline{
        origin,
        eastOneDegree,
        northOneDegree,
    };
    const double polylineLength = globe::polylineLengthMeters(polyline);
    if (!nearlyEqual(polylineLength, eastDistance + 157249.0, 1000.0)) {
        std::cerr << "Expected polyline length to sum segment distances.\n";
        return EXIT_FAILURE;
    }

    if (globe::polylineLengthMeters({origin}) != 0.0) {
        std::cerr << "Expected single-point polyline length to be zero.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
