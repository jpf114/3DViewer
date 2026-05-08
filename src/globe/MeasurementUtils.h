#pragma once

#include <cmath>
#include <vector>

namespace globe {

struct MeasurementPoint {
    double longitude = 0.0;
    double latitude = 0.0;
};

inline double degreesToRadians(double degrees) {
    return degrees * 3.14159265358979323846 / 180.0;
}

inline double greatCircleDistanceMeters(const MeasurementPoint &lhs, const MeasurementPoint &rhs) {
    constexpr double kEarthRadiusMeters = 6371008.8;

    const double lat1 = degreesToRadians(lhs.latitude);
    const double lat2 = degreesToRadians(rhs.latitude);
    const double dLat = lat2 - lat1;
    const double dLon = degreesToRadians(rhs.longitude - lhs.longitude);

    const double sinHalfLat = std::sin(dLat * 0.5);
    const double sinHalfLon = std::sin(dLon * 0.5);
    const double a = sinHalfLat * sinHalfLat +
        std::cos(lat1) * std::cos(lat2) * sinHalfLon * sinHalfLon;
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return kEarthRadiusMeters * c;
}

inline double polylineLengthMeters(const std::vector<MeasurementPoint> &points) {
    if (points.size() < 2) {
        return 0.0;
    }

    double total = 0.0;
    for (std::size_t i = 1; i < points.size(); ++i) {
        total += greatCircleDistanceMeters(points[i - 1], points[i]);
    }
    return total;
}

} // namespace globe
