#pragma once

#include <string>
#include <utility>

#include "layers/Layer.h"

class ElevationLayer : public Layer {
public:
    ElevationLayer(std::string id, std::string name, std::string sourceUri)
        : Layer(std::move(id), std::move(name), std::move(sourceUri), LayerKind::Elevation) {}
};
