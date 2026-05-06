#pragma once

#include <QMetaType>
#include <string>

struct PickResult {
    bool hit = false;
    double longitude = 0.0;
    double latitude = 0.0;
    double elevation = 0.0;
    std::string layerId;
    std::string displayText;
};

Q_DECLARE_METATYPE(PickResult)
