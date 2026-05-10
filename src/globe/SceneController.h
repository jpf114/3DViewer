#pragma once

#include <memory>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include <QImage>

#include "data/ProjectConfig.h"
#include "layers/MeasurementLayerData.h"
#include "layers/GeographicBounds.h"
#include "layers/Layer.h"

struct BasemapConfig;
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
    void setSelectedLayer(const std::string &id);
    void setMeasurementDraft(const MeasurementLayerData &data);
    void syncLayerState(const std::shared_ptr<Layer> &layer);
    void updateImageLayerBands(const std::shared_ptr<Layer> &layer);
    void initializeDefaultScene(int width, int height);
    void flyToGeographicBounds(const GeographicBounds &bounds, double durationSeconds = 1.5);
    void reorderUserLayers(const std::vector<std::shared_ptr<Layer>> &userLayersInOrder);
    void resetView();
    void clearMeasurementDraft();
    QImage captureImage();
    void setBasemapConfig(const BasemapConfig &config, const std::string &resourceDir);
    void setBasemapConfig(const BasemapConfig &config, const std::string &resourceDir, const std::string &preferredBasemapId);
    std::string currentBasemapId() const;
    std::optional<ProjectCameraState> currentCameraState() const;
    void applyCameraState(const ProjectCameraState &state, double durationSeconds = 0.0);

private:
    void flushPendingLayers();
    bool isMeasurementLayerSelected() const;
    void refreshMeasurementSelection();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};
