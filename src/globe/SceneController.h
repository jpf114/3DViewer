#pragma once

#include <memory>
#include <cstddef>
#include <string>
#include <unordered_map>

struct PickResult;
class Layer;

class SceneController {
public:
    SceneController();
    ~SceneController();

    SceneController(const SceneController &) = delete;
    SceneController &operator=(const SceneController &) = delete;
    SceneController(SceneController &&) noexcept;
    SceneController &operator=(SceneController &&) noexcept;

    void addLayer(const std::shared_ptr<Layer> &layer);
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
    void initializeDefaultScene(int width, int height);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
