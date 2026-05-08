#pragma once

#include "layers/Layer.h"

class ModelLayer : public Layer {
public:
    ModelLayer(std::string id, std::string name, std::string sourceUri)
        : Layer(std::move(id), std::move(name), std::move(sourceUri), LayerKind::Model) {}
};
