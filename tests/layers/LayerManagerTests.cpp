#include <memory>

#include "layers/ElevationLayer.h"
#include "layers/ImageryLayer.h"
#include "layers/LayerManager.h"
#include "layers/VectorLayer.h"

int main() {
    LayerManager manager;
    auto layer = std::make_shared<ImageryLayer>("world", "World Imagery", "memory://world");
    auto second = std::make_shared<ImageryLayer>("roads", "Roads", "memory://roads");

    if (!manager.addLayer(layer)) {
        return 1;
    }
    if (!manager.addLayer(second)) {
        return 1;
    }

    if (manager.addLayer(layer)) {
        return 1;
    }

    if (manager.addLayer(nullptr)) {
        return 1;
    }

    if (manager.layers().size() != 2) {
        return 1;
    }

    manager.setVisibility("roads", false);
    manager.moveLayer(1, 0);

    const auto found = manager.findById("roads");
    if (!found || found->visible()) {
        return 1;
    }

    if (manager.findById("nonexistent") != nullptr) {
        return 1;
    }

    if (manager.layers().size() != 2 || manager.layers().front()->id() != "roads") {
        return 1;
    }

    if (!manager.reorderLayers({"world", "roads"})) {
        return 2;
    }

    if (manager.layers()[0]->id() != "world" || manager.layers()[1]->id() != "roads") {
        return 3;
    }

    if (manager.reorderLayers({"world"})) {
        return 4;
    }

    if (!manager.removeLayer("world")) {
        return 5;
    }

    if (manager.layers().size() != 1 || manager.layers()[0]->id() != "roads") {
        return 5;
    }

    if (manager.removeLayer("nonexistent")) {
        return 6;
    }

    return 0;
}
