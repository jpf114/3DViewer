#pragma once

#include <string>

#include "layers/LayerTypes.h"

class Layer {
public:
    Layer(std::string id, std::string name, std::string sourceUri, LayerKind kind);
    virtual ~Layer() = default;

    const std::string &id() const;
    const std::string &name() const;
    const std::string &sourceUri() const;
    LayerKind kind() const;
    bool visible() const;
    double opacity() const;

    void setVisible(bool visible);
    void setOpacity(double opacity);

private:
    std::string id_;
    std::string name_;
    std::string sourceUri_;
    LayerKind kind_;
    bool visible_ = true;
    double opacity_ = 1.0;
};
