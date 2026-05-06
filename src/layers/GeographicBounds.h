#pragma once

#include <cmath>

struct GeographicBounds {
    double west{};
    double south{};
    double east{};
    double north{};

    bool isValid() const {
        if (!std::isfinite(west) || !std::isfinite(south) || !std::isfinite(east) || !std::isfinite(north)) {
            return false;
        }
        if (west > east || south > north) {
            return false;
        }
        const double maxSpan = 360.0;
        if ((east - west) > maxSpan || (north - south) > 180.0) {
            return false;
        }
        return true;
    }
};
