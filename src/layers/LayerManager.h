#pragma once

#include <memory>
#include <string>
#include <vector>

class Layer;

class LayerManager {
public:
    bool addLayer(const std::shared_ptr<Layer> &layer);
    std::shared_ptr<Layer> findById(const std::string &id) const;
    const std::vector<std::shared_ptr<Layer>> &layers() const;
    void moveLayer(std::size_t from, std::size_t to);
    bool reorderLayers(const std::vector<std::string> &orderedIds);
    void setVisibility(const std::string &id, bool visible);
    bool removeLayer(const std::string &id);

private:
    std::vector<std::shared_ptr<Layer>> layers_;
};
