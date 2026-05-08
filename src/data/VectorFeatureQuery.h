#pragma once

#include <optional>
#include <string>
#include <vector>

struct VectorFeatureAttribute {
    std::string name;
    std::string value;
};

struct VectorFeatureHit {
    std::string displayText;
    std::vector<VectorFeatureAttribute> attributes;
};

std::optional<VectorFeatureHit> queryVectorFeatureAt(const std::string &path,
                                                     double longitude,
                                                     double latitude);
