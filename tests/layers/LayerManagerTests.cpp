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

    return manager.layers().size() == 2 && manager.layers().front()->id() == "roads" ? 0 : 1;
}
