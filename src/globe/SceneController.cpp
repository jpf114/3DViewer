#include "globe/SceneController.h"

#include <osg/Camera>
#include <osg/GraphicsContext>
#include <osg/Viewport>
#include <osgEarth/Color>
#include <osgEarth/EarthManipulator>
#include <osgEarth/Viewpoint>
#include <osgEarth/FeatureModelLayer>
#include <osgEarth/Fill>
#include <osgEarth/GLUtils>
#include <osgEarth/GDAL>
#include <osgEarth/GeoData>
#include <osgEarth/LineSymbol>
#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarth/OGRFeatureSource>
#include <osgEarth/PointSymbol>
#include <osgEarth/PolygonSymbol>
#include <osgEarth/Profile>
#include <osgEarth/RenderSymbol>
#include <osgEarth/Stroke>
#include <osgEarth/Style>
#include <osgEarth/StyleSheet>
#include <osgEarth/URI>
#include <osgEarth/VisibleLayer>
#include <osgEarth/XYZ>
#include <algorithm>
#include <cmath>

#include <osgGA/EventQueue>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>
#include <osgViewer/api/Win32/GraphicsWindowWin32>

#include "globe/PickResult.h"
#include "layers/Layer.h"

struct SceneController::Impl {
    bool initialized = false;
    std::unordered_map<std::string, bool> layerVisibility;
    std::unordered_map<std::string, osg::ref_ptr<osgEarth::Layer>> renderLayers;
    osg::ref_ptr<osgEarth::Map> map;
    osg::ref_ptr<osgEarth::MapNode> mapNode;
    osg::ref_ptr<osgViewer::Viewer> viewer;
    osg::ref_ptr<osgViewer::GraphicsWindow> graphicsWindow;
    osg::ref_ptr<osgEarth::ImageLayer> baseLayer;
};

namespace {

osg::ref_ptr<osgEarth::XYZImageLayer> createBaseLayer() {
    auto layer = osg::ref_ptr<osgEarth::XYZImageLayer>(new osgEarth::XYZImageLayer());
    layer->setName("OpenStreetMap");
    layer->setURL(osgEarth::URI("https://tile.openstreetmap.org/{z}/{x}/{y}.png"));
    layer->setProfile(osgEarth::Profile::create(osgEarth::Profile::SPHERICAL_MERCATOR));
    return layer;
}

osg::ref_ptr<osgEarth::Layer> createSceneLayer(const Layer &layer) {
    switch (layer.kind()) {
    case LayerKind::Imagery: {
        auto imageLayer = osg::ref_ptr<osgEarth::GDALImageLayer>(new osgEarth::GDALImageLayer());
        imageLayer->setName(layer.name());
        imageLayer->setURL(osgEarth::URI(layer.sourceUri()));
        return imageLayer;
    }
    case LayerKind::Elevation: {
        auto elevationLayer = osg::ref_ptr<osgEarth::GDALElevationLayer>(new osgEarth::GDALElevationLayer());
        elevationLayer->setName(layer.name());
        elevationLayer->setURL(osgEarth::URI(layer.sourceUri()));
        return elevationLayer;
    }
    case LayerKind::Vector: {
        auto featureSource = osg::ref_ptr<osgEarth::OGRFeatureSource>(new osgEarth::OGRFeatureSource());
        featureSource->setName(layer.name() + " Source");
        featureSource->setURL(osgEarth::URI(layer.sourceUri()));

        osgEarth::Style style("default");
        style.getOrCreate<osgEarth::PointSymbol>()->fill() = osgEarth::Fill(osgEarth::Color(1.0f, 0.65f, 0.2f, 1.0f));
        style.getOrCreate<osgEarth::PointSymbol>()->size() = 6.0f;
        style.getOrCreate<osgEarth::LineSymbol>()->stroke() = osgEarth::Stroke(osgEarth::Color(0.15f, 0.85f, 1.0f, 1.0f));
        style.getOrCreate<osgEarth::PolygonSymbol>()->fill() = osgEarth::Fill(osgEarth::Color(0.15f, 0.55f, 0.95f, 0.35f));
        style.getOrCreate<osgEarth::RenderSymbol>()->depthTest() = true;

        auto styleSheet = osg::ref_ptr<osgEarth::StyleSheet>(new osgEarth::StyleSheet());
        styleSheet->addStyle(style);

        auto featureLayer = osg::ref_ptr<osgEarth::FeatureModelLayer>(new osgEarth::FeatureModelLayer());
        featureLayer->setName(layer.name());
        featureLayer->setFeatureSource(featureSource.get());
        featureLayer->setStyleSheet(styleSheet.get());
        return featureLayer;
    }
    case LayerKind::Chart:
    case LayerKind::Scientific:
        return nullptr;
    }

    return nullptr;
}

void syncVisibleState(osgEarth::Layer *sceneLayer, const Layer &layer) {
    auto *visibleLayer = dynamic_cast<osgEarth::VisibleLayer *>(sceneLayer);
    if (visibleLayer == nullptr) {
        return;
    }

    visibleLayer->setVisible(layer.visible());
    visibleLayer->setOpacity(static_cast<float>(layer.opacity()));
}

void updateViewport(osgViewer::Viewer &viewer, osgViewer::GraphicsWindow &graphicsWindow, int width, int height) {
    graphicsWindow.getEventQueue()->windowResize(0, 0, width, height);
    graphicsWindow.resized(0, 0, width, height);
    viewer.getCamera()->setViewport(new osg::Viewport(0, 0, width, height));
}

} // namespace

SceneController::SceneController() : impl_(std::make_unique<Impl>()) {}

SceneController::~SceneController() = default;

SceneController::SceneController(SceneController &&) noexcept = default;

SceneController &SceneController::operator=(SceneController &&) noexcept = default;

void SceneController::addLayer(const std::shared_ptr<Layer> &layer) {
    impl_->layerVisibility[layer->id()] = layer->visible();

    if (!impl_->map || impl_->renderLayers.contains(layer->id())) {
        return;
    }

    auto sceneLayer = createSceneLayer(*layer);
    if (!sceneLayer) {
        return;
    }

    syncVisibleState(sceneLayer.get(), *layer);
    impl_->map->addLayer(sceneLayer.get());
    impl_->renderLayers[layer->id()] = sceneLayer;
}

void SceneController::reorderUserLayers(const std::vector<std::shared_ptr<Layer>> &userLayersInOrder) {
    if (!impl_->map) {
        return;
    }

    constexpr unsigned kBaseLayerCount = 1u;
    impl_->map->beginUpdate();
    unsigned targetIndex = kBaseLayerCount;
    for (const auto &userLayer : userLayersInOrder) {
        const auto it = impl_->renderLayers.find(userLayer->id());
        if (it == impl_->renderLayers.end()) {
            continue;
        }

        osgEarth::Layer *ol = it->second.get();
        impl_->map->moveLayer(ol, targetIndex);
        ++targetIndex;
    }
    impl_->map->endUpdate();
}

void SceneController::attachToNativeWindow(void *windowHandle, int width, int height) {
    if (!impl_->viewer || windowHandle == nullptr) {
        return;
    }

    if (!impl_->graphicsWindow) {
        auto traits = osg::ref_ptr<osg::GraphicsContext::Traits>(new osg::GraphicsContext::Traits());
        traits->x = 0;
        traits->y = 0;
        traits->width = width;
        traits->height = height;
        traits->windowDecoration = false;
        traits->doubleBuffer = true;
        traits->sharedContext = nullptr;
        traits->inheritedWindowData =
            new osgViewer::GraphicsWindowWin32::WindowData(static_cast<HWND>(windowHandle), false);

        auto graphicsWindow = osg::ref_ptr<osgViewer::GraphicsWindowWin32>(new osgViewer::GraphicsWindowWin32(traits.get()));
        if (!graphicsWindow->valid()) {
            return;
        }

        impl_->graphicsWindow = graphicsWindow.get();
        impl_->viewer->getCamera()->setGraphicsContext(graphicsWindow.get());
        impl_->viewer->getCamera()->setViewport(new osg::Viewport(0, 0, width, height));
        impl_->viewer->getCamera()->setProjectionMatrixAsPerspective(
            30.0,
            height > 0 ? static_cast<double>(width) / static_cast<double>(height) : 1.0,
            1.0,
            100000000.0);
        impl_->viewer->realize();
    }

    resize(width, height);
}

void SceneController::frame() {
    if (!impl_->viewer) {
        return;
    }

    impl_->viewer->frame();
}

bool SceneController::hasBaseLayer() const {
    return impl_->baseLayer.valid();
}

bool SceneController::hasMapNode() const {
    return impl_->mapNode.valid();
}

bool SceneController::hasViewer() const {
    return impl_->viewer.valid();
}

bool SceneController::isLayerVisibleInScene(const std::string &id) const {
    const auto it = impl_->renderLayers.find(id);
    if (it == impl_->renderLayers.end()) {
        return false;
    }

    const auto *visibleLayer = dynamic_cast<osgEarth::VisibleLayer *>(it->second.get());
    return visibleLayer != nullptr && visibleLayer->getVisible();
}

bool SceneController::isInitialized() const {
    return impl_->initialized;
}

void SceneController::mouseMove(float x, float y) {
    if (!impl_->graphicsWindow) {
        return;
    }

    impl_->graphicsWindow->getEventQueue()->mouseMotion(x, y);
}

void SceneController::mousePress(float x, float y, unsigned int button) {
    if (!impl_->graphicsWindow) {
        return;
    }

    impl_->graphicsWindow->getEventQueue()->mouseButtonPress(x, y, button);
}

void SceneController::mouseRelease(float x, float y, unsigned int button) {
    if (!impl_->graphicsWindow) {
        return;
    }

    impl_->graphicsWindow->getEventQueue()->mouseButtonRelease(x, y, button);
}

void SceneController::mouseScroll(bool scrollUp) {
    if (!impl_->graphicsWindow) {
        return;
    }

    impl_->graphicsWindow->getEventQueue()->mouseScroll(
        scrollUp ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN);
}

PickResult SceneController::pickAt(int x, int y) const {
    PickResult result;
    if (!impl_->viewer || !impl_->mapNode) {
        return result;
    }

    osgEarth::GeoPoint point;
    if (!impl_->mapNode->getGeoPointUnderMouse(impl_->viewer.get(), static_cast<float>(x), static_cast<float>(y), point)) {
        return result;
    }

    if (!point.makeGeographic()) {
        return result;
    }

    result.hit = true;
    result.longitude = point.x();
    result.latitude = point.y();
    result.elevation = point.z();
    return result;
}

void SceneController::removeLayer(const std::string &id) {
    impl_->layerVisibility.erase(id);

    const auto it = impl_->renderLayers.find(id);
    if (it == impl_->renderLayers.end()) {
        return;
    }

    if (impl_->map) {
        impl_->map->removeLayer(it->second.get());
    }
    impl_->renderLayers.erase(it);
}

std::size_t SceneController::renderedLayerCount() const {
    return impl_->renderLayers.size();
}

void SceneController::resize(int width, int height) {
    if (!impl_->viewer || !impl_->graphicsWindow) {
        return;
    }

    updateViewport(*impl_->viewer, *impl_->graphicsWindow, width, height);
}

void SceneController::syncLayerState(const std::shared_ptr<Layer> &layer) {
    impl_->layerVisibility[layer->id()] = layer->visible();

    const auto it = impl_->renderLayers.find(layer->id());
    if (it == impl_->renderLayers.end()) {
        return;
    }

    syncVisibleState(it->second.get(), *layer);
}

void SceneController::flyToGeographicBounds(const GeographicBounds &bounds, double durationSeconds) {
    if (!impl_->viewer) {
        return;
    }

    auto *em = dynamic_cast<osgEarth::EarthManipulator *>(impl_->viewer->getCameraManipulator());
    if (em == nullptr) {
        return;
    }

    if (!bounds.isValid()) {
        return;
    }

    const double lat = 0.5 * (bounds.south + bounds.north);
    const double lon = 0.5 * (bounds.west + bounds.east);
    const double latRad = lat * 3.14159265358979323846 / 180.0;
    constexpr double kMetersPerDegLat = 111320.0;
    const double mPerDegLon = (std::max)(1e-6, kMetersPerDegLat * std::cos(latRad));
    const double widthM = (bounds.east - bounds.west) * mPerDegLon;
    const double heightM = (bounds.north - bounds.south) * kMetersPerDegLat;
    const double maxDim = (std::max)(widthM, heightM);
    const double range = std::clamp(maxDim * 1.75, 200.0, 4.0e7);

    osgEarth::Viewpoint vp("layer-extent", lon, lat, 0.0, 0.0, -90.0, range);
    em->setViewpoint(vp, durationSeconds);
}

void SceneController::initializeDefaultScene(int width, int height) {
    if (impl_->initialized) {
        return;
    }

    impl_->map = new osgEarth::Map();
    impl_->baseLayer = createBaseLayer();
    impl_->map->addLayer(impl_->baseLayer.get());

    impl_->mapNode = new osgEarth::MapNode(impl_->map.get());
    impl_->viewer = new osgViewer::Viewer();
    impl_->viewer->setRealizeOperation(new osgEarth::GL3RealizeOperation());
    impl_->viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
    impl_->viewer->setCameraManipulator(new osgEarth::EarthManipulator());
    impl_->viewer->setSceneData(impl_->mapNode.get());

    impl_->initialized = true;
}
