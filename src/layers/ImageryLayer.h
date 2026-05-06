#pragma once

#include <string>
#include <utility>

#include "layers/Layer.h"

class ImageryLayer : public Layer {
public:
    ImageryLayer(std::string id, std::string name, std::string sourceUri)
        : Layer(std::move(id), std::move(name), std::move(sourceUri), LayerKind::Imagery) {}
};
