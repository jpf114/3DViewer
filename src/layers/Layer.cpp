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

void Layer::setRasterMetadata(const RasterMetadata &meta) {
    rasterMetadata_ = meta;
}

std::optional<RasterMetadata> Layer::rasterMetadata() const {
    return rasterMetadata_;
}

void Layer::setVectorMetadata(const VectorLayerInfo &meta) {
    vectorMetadata_ = meta;
}

std::optional<VectorLayerInfo> Layer::vectorMetadata() const {
    return vectorMetadata_;
}

void Layer::setBandMapping(int redBand, int greenBand, int blueBand) {
    redBand_ = redBand;
    greenBand_ = greenBand;
    blueBand_ = blueBand;
}

int Layer::redBand() const { return redBand_; }
int Layer::greenBand() const { return greenBand_; }
int Layer::blueBand() const { return blueBand_; }
