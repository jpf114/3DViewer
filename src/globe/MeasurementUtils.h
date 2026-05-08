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

inline double polygonAreaSquareMeters(const std::vector<MeasurementPoint> &points) {
    if (points.size() < 3) {
        return 0.0;
    }

    constexpr double kEarthRadiusMeters = 6371008.8;
    double centerLon = 0.0;
    double centerLat = 0.0;
    for (const auto &point : points) {
        centerLon += point.longitude;
        centerLat += point.latitude;
    }
    centerLon /= static_cast<double>(points.size());
    centerLat /= static_cast<double>(points.size());

    const double centerLonRad = degreesToRadians(centerLon);
    const double centerLatRad = degreesToRadians(centerLat);

    double twiceArea = 0.0;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto &a = points[i];
        const auto &b = points[(i + 1) % points.size()];

        const double ax = kEarthRadiusMeters * (degreesToRadians(a.longitude) - centerLonRad) * std::cos(centerLatRad);
        const double ay = kEarthRadiusMeters * (degreesToRadians(a.latitude) - centerLatRad);
        const double bx = kEarthRadiusMeters * (degreesToRadians(b.longitude) - centerLonRad) * std::cos(centerLatRad);
        const double by = kEarthRadiusMeters * (degreesToRadians(b.latitude) - centerLatRad);
        twiceArea += ax * by - bx * ay;
    }

    return std::abs(twiceArea) * 0.5;
}

} // namespace globe
