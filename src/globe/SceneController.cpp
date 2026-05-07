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
#include <osgEarth/TextSymbol>
#include <osgEarth/URI>
#include <osgEarth/VisibleLayer>
#include <osgEarth/XYZ>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <spdlog/spdlog.h>

#include <osgGA/EventQueue>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>
#include <osgViewer/api/Win32/GraphicsWindowWin32>

#include "globe/PickResult.h"
#include "layers/Layer.h"

#include <osgEarth/FeatureSourceIndexNode>

struct SceneController::Impl {
    bool initialized = false;
    std::unordered_map<std::string, bool> layerVisibility;
    std::unordered_map<std::string, osg::ref_ptr<osgEarth::Layer>> renderLayers;
    std::vector<std::shared_ptr<Layer>> pendingLayers;
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

        osgEarth::Style pointStyle("point");
        pointStyle.getOrCreate<osgEarth::PointSymbol>()->fill() = osgEarth::Fill(osgEarth::Color(1.0f, 0.35f, 0.15f, 1.0f));
        pointStyle.getOrCreate<osgEarth::PointSymbol>()->size() = 7.0f;
        pointStyle.getOrCreate<osgEarth::PointSymbol>()->smooth() = true;
        auto *pointText = pointStyle.getOrCreate<osgEarth::TextSymbol>();
        pointText->fill() = osgEarth::Fill(osgEarth::Color::White);
        pointText->halo() = osgEarth::Stroke(osgEarth::Color::Black);
        pointText->size() = osgEarth::Expression<float>("14");
        pointText->content() = osgEarth::StringExpression("[name]");

        osgEarth::Style lineStyle("line");
        osgEarth::Stroke lineStroke(osgEarth::Color(0.15f, 0.85f, 1.0f, 1.0f));
        lineStroke.width() = osgEarth::Expression<osgEarth::Distance>("2.5px");
        lineStyle.getOrCreate<osgEarth::LineSymbol>()->stroke() = lineStroke;
        lineStyle.getOrCreate<osgEarth::LineSymbol>()->tessellation() = 20;
        lineStyle.getOrCreate<osgEarth::RenderSymbol>()->depthTest() = true;
        auto *lineText = lineStyle.getOrCreate<osgEarth::TextSymbol>();
        lineText->fill() = osgEarth::Fill(osgEarth::Color::White);
        lineText->halo() = osgEarth::Stroke(osgEarth::Color::Black);
        lineText->size() = osgEarth::Expression<float>("14");
        lineText->content() = osgEarth::StringExpression("[name]");

        osgEarth::Style polygonStyle("polygon");
        polygonStyle.getOrCreate<osgEarth::PolygonSymbol>()->fill() = osgEarth::Fill(osgEarth::Color(0.15f, 0.55f, 0.95f, 0.45f));
        osgEarth::Stroke polyStroke(osgEarth::Color(0.1f, 0.4f, 0.85f, 1.0f));
        polyStroke.width() = osgEarth::Expression<osgEarth::Distance>("1.5px");
        polygonStyle.getOrCreate<osgEarth::LineSymbol>()->stroke() = polyStroke;
        polygonStyle.getOrCreate<osgEarth::RenderSymbol>()->depthTest() = true;
        auto *polyText = polygonStyle.getOrCreate<osgEarth::TextSymbol>();
        polyText->fill() = osgEarth::Fill(osgEarth::Color::White);
        polyText->halo() = osgEarth::Stroke(osgEarth::Color::Black);
        polyText->size() = osgEarth::Expression<float>("14");
        polyText->content() = osgEarth::StringExpression("[name]");

        auto styleSheet = osg::ref_ptr<osgEarth::StyleSheet>(new osgEarth::StyleSheet());
        styleSheet->addStyle(pointStyle);
        styleSheet->addStyle(lineStyle);
        styleSheet->addStyle(polygonStyle);

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

    if (impl_->renderLayers.contains(layer->id())) {
        spdlog::warn("SceneController: layer '{}' already in scene, skipping", layer->id());
        return;
    }

    if (!impl_->map) {
        spdlog::info("SceneController: scene not initialized, queueing layer '{}'", layer->id());
        impl_->pendingLayers.push_back(layer);
        return;
    }

    auto sceneLayer = createSceneLayer(*layer);
    if (!sceneLayer) {
        spdlog::warn("SceneController: unsupported layer kind for '{}'", layer->id());
        return;
    }

    syncVisibleState(sceneLayer.get(), *layer);
    impl_->map->addLayer(sceneLayer.get());
    impl_->renderLayers[layer->id()] = sceneLayer;
    spdlog::info("SceneController: added layer '{}' to scene", layer->id());
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

    for (const auto &[id, sceneLayer] : impl_->renderLayers) {
        auto *visibleLayer = dynamic_cast<osgEarth::VisibleLayer *>(sceneLayer.get());
        if (visibleLayer && visibleLayer->getVisible()) {
            result.layerId = id;
            result.displayText = visibleLayer->getName();
        }
    }

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
    spdlog::info("SceneController: removed layer '{}' from scene", id);
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

void SceneController::updateImageLayerBands(const std::shared_ptr<Layer> &layer) {
    if (!layer || layer->kind() != LayerKind::Imagery) {
        return;
    }

    const auto it = impl_->renderLayers.find(layer->id());
    if (it == impl_->renderLayers.end()) {
        return;
    }

    if (!impl_->map) {
        return;
    }

    const auto &rm = layer->rasterMetadata();
    if (!rm || rm->bandCount < 2) {
        return;
    }

    impl_->map->removeLayer(it->second.get());

    const int r = std::clamp(layer->redBand(), 1, rm->bandCount);
    const int g = std::clamp(layer->greenBand(), 1, rm->bandCount);
    const int b = std::clamp(layer->blueBand(), 1, rm->bandCount);

    const std::string vrtXml = std::string(
        "<VRTDataset rasterXSize=\"") + std::to_string(rm->rasterXSize) +
        "\" rasterYSize=\"" + std::to_string(rm->rasterYSize) + "\">"
        "<VRTRasterBand dataType=\"Byte\" band=\"1\">"
        "<SimpleSource><SourceFilename>" + layer->sourceUri() + "</SourceFilename>"
        "<SourceBand>" + std::to_string(r) + "</SourceBand>"
        "</SimpleSource>"
        "</VRTRasterBand>"
        "<VRTRasterBand dataType=\"Byte\" band=\"2\">"
        "<SimpleSource><SourceFilename>" + layer->sourceUri() + "</SourceFilename>"
        "<SourceBand>" + std::to_string(g) + "</SourceBand>"
        "</SimpleSource>"
        "</VRTRasterBand>"
        "<VRTRasterBand dataType=\"Byte\" band=\"3\">"
        "<SimpleSource><SourceFilename>" + layer->sourceUri() + "</SourceFilename>"
        "<SourceBand>" + std::to_string(b) + "</SourceBand>"
        "</SimpleSource>"
        "</VRTRasterBand>"
        "</VRTDataset>";

    auto imageLayer = osg::ref_ptr<osgEarth::GDALImageLayer>(new osgEarth::GDALImageLayer());
    imageLayer->setName(layer->name());
    imageLayer->setURL(osgEarth::URI(vrtXml));
    syncVisibleState(imageLayer.get(), *layer);
    impl_->map->addLayer(imageLayer.get());
    impl_->renderLayers[layer->id()] = imageLayer;
    spdlog::info("SceneController: updated band mapping for '{}' to R={} G={} B={}", layer->id(), r, g, b);
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
    const double latRad = lat * std::numbers::pi / 180.0;
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
    spdlog::info("SceneController: default scene initialized");
    flushPendingLayers();
}

void SceneController::flushPendingLayers() {
    auto pending = std::move(impl_->pendingLayers);
    impl_->pendingLayers.clear();
    for (const auto &layer : pending) {
        addLayer(layer);
    }
}
