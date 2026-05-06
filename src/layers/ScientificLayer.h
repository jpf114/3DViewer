#pragma once

#include <string>
#include <utility>

#include "layers/Layer.h"

class ScientificLayer : public Layer {
public:
    ScientificLayer(std::string id, std::string name, std::string sourceUri)
        : Layer(std::move(id), std::move(name), std::move(sourceUri), LayerKind::Scientific) {}
};
