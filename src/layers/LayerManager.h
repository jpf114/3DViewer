#pragma once

#include <memory>
#include <string>
#include <vector>

class Layer;

class LayerManager {
public:
    void addLayer(const std::shared_ptr<Layer> &layer);
    std::shared_ptr<Layer> findById(const std::string &id) const;
    const std::vector<std::shared_ptr<Layer>> &layers() const;
    void moveLayer(std::size_t from, std::size_t to);
    void setVisibility(const std::string &id, bool visible);

private:
    std::vector<std::shared_ptr<Layer>> layers_;
};
