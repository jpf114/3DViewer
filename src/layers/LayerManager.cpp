#include "layers/LayerManager.h"

#include <algorithm>
#include <unordered_map>

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

bool LayerManager::reorderLayers(const std::vector<std::string> &orderedIds) {
    if (orderedIds.size() != layers_.size()) {
        return false;
    }

    std::unordered_map<std::string, std::shared_ptr<Layer>> byId;
    byId.reserve(layers_.size());
    for (const auto &layer : layers_) {
        byId[layer->id()] = layer;
    }

    if (byId.size() != layers_.size()) {
        return false;
    }

    std::vector<std::shared_ptr<Layer>> next;
    next.reserve(orderedIds.size());
    for (const auto &id : orderedIds) {
        const auto it = byId.find(id);
        if (it == byId.end()) {
            return false;
        }
        next.push_back(it->second);
    }

    layers_ = std::move(next);
    return true;
}

void LayerManager::setVisibility(const std::string &id, bool visible) {
    const auto layer = findById(id);
    if (layer) {
        layer->setVisible(visible);
    }
}
