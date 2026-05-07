#pragma once

#include <QMetaType>
#include <string>
#include <vector>

struct FeatureAttribute {
    std::string name;
    std::string value;
};

struct PickResult {
    bool hit = false;
    double longitude = 0.0;
    double latitude = 0.0;
    double elevation = 0.0;
    std::string layerId;
    std::string displayText;
    std::vector<FeatureAttribute> featureAttributes;
};

Q_DECLARE_METATYPE(PickResult)
