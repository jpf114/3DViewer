#include "layers/LayerManager.h"

#include <algorithm>

#include "layers/Layer.h"

void LayerManager::addLayer(const std::shared_ptr<Layer> &layer) {
    layers_.push_back(layer);
}

std::shared_ptr<Layer> LayerManager::findById(const std::string &id) const {
    const auto it = std::find_if(layers_.begin(), layers_.end(), [&id](const std::shared_ptr<Layer> &layer) {
        return layer->id() == id;
    });
    return it == layers_.end() ? nullptr : *it;
}

const std::vector<std::shared_ptr<Layer>> &LayerManager::layers() const {
    return layers_;
}

void LayerManager::moveLayer(std::size_t from, std::size_t to) {
    if (from >= layers_.size() || to >= layers_.size() || from == to) {
        return;
    }

    const auto layer = layers_[from];
    layers_.erase(layers_.begin() + static_cast<std::ptrdiff_t>(from));
    layers_.insert(layers_.begin() + static_cast<std::ptrdiff_t>(to), layer);
}

void LayerManager::setVisibility(const std::string &id, bool visible) {
    const auto layer = findById(id);
    if (layer) {
        layer->setVisible(visible);
    }
}
