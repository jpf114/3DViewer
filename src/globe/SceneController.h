#pragma once

#include <memory>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include <QImage>

#include "layers/GeographicBounds.h"
#include "layers/Layer.h"

struct PickResult;

class SceneController {
public:
    SceneController();
    ~SceneController();

    SceneController(const SceneController &) = delete;
    SceneController &operator=(const SceneController &) = delete;
    SceneController(SceneController &&) noexcept;
    SceneController &operator=(SceneController &&) noexcept;

    void addLayer(const std::shared_ptr<Layer> &layer);
    void attachToNativeWindow(void *windowHandle, int width, int height);
    void frame();
    bool hasBaseLayer() const;
    bool hasMapNode() const;
    bool hasViewer() const;
    bool isLayerVisibleInScene(const std::string &id) const;
    bool isInitialized() const;
    void mouseMove(float x, float y);
    void mousePress(float x, float y, unsigned int button);
    void mouseRelease(float x, float y, unsigned int button);
    void mouseScroll(bool scrollUp);
    PickResult pickAt(int x, int y) const;
    void removeLayer(const std::string &id);
    std::size_t renderedLayerCount() const;
    void resize(int width, int height);
    void syncLayerState(const std::shared_ptr<Layer> &layer);
    void updateImageLayerBands(const std::shared_ptr<Layer> &layer);
    void initializeDefaultScene(int width, int height);
    void flyToGeographicBounds(const GeographicBounds &bounds, double durationSeconds = 1.5);
    void reorderUserLayers(const std::vector<std::shared_ptr<Layer>> &userLayersInOrder);
    void resetView();
    QImage captureImage();

private:
    void flushPendingLayers();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};
