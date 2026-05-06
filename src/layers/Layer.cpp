#include "layers/Layer.h"

#include <algorithm>
#include <utility>

Layer::Layer(std::string id, std::string name, std::string sourceUri, LayerKind kind)
    : id_(std::move(id)),
      name_(std::move(name)),
      sourceUri_(std::move(sourceUri)),
      kind_(kind) {}

const std::string &Layer::id() const {
    return id_;
}

const std::string &Layer::name() const {
    return name_;
}

const std::string &Layer::sourceUri() const {
    return sourceUri_;
}

LayerKind Layer::kind() const {
    return kind_;
}

bool Layer::visible() const {
    return visible_;
}

double Layer::opacity() const {
    return opacity_;
}

void Layer::setVisible(bool visible) {
    visible_ = visible;
}

void Layer::setOpacity(double opacity) {
    opacity_ = std::clamp(opacity, 0.0, 1.0);
}

void Layer::setGeographicBounds(const GeographicBounds &bounds) {
    geographicBounds_ = bounds;
}

std::optional<GeographicBounds> Layer::geographicBounds() const {
    return geographicBounds_;
}
