#pragma once

#include <string>
#include <utility>

#include "layers/Layer.h"

class ChartLayer : public Layer {
public:
    ChartLayer(std::string id, std::string name, std::string sourceUri)
        : Layer(std::move(id), std::move(name), std::move(sourceUri), LayerKind::Chart) {}
};
