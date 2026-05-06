#include <memory>

#include "layers/ImageryLayer.h"
#include "layers/LayerManager.h"

int main() {
    LayerManager manager;
    auto layer = std::make_shared<ImageryLayer>("world", "World Imagery", "memory://world");
    auto second = std::make_shared<ImageryLayer>("roads", "Roads", "memory://roads");

    manager.addLayer(layer);
    manager.addLayer(second);

    manager.setVisibility("roads", false);
    manager.moveLayer(1, 0);

    const auto found = manager.findById("roads");
    if (!found || found->visible()) {
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

    return 0;
}
