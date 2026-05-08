#include "globe/SceneController.h"

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

#include <osg/Camera>
#include <osg/GraphicsContext>
#include <osg/Image>
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
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <numbers>
#include <spdlog/spdlog.h>

#include <osgGA/EventQueue>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>
#include <osgViewer/api/Win32/GraphicsWindowWin32>

#include "globe/PickResult.h"
#include "layers/Layer.h"


#include <osgEarth/Registry>

struct SceneController::Impl {
    bool initialized = false;
    HWND hwnd = nullptr;
    std::unordered_map<std::string, osg::ref_ptr<osgEarth::Layer>> renderLayers;
    std::vector<std::shared_ptr<Layer>> pendingLayers;
    osg::ref_ptr<osgEarth::Map> map;
    osg::ref_ptr<osgEarth::MapNode> mapNode;
    osg::ref_ptr<osgViewer::Viewer> viewer;
    osg::ref_ptr<osgViewer::GraphicsWindow> graphicsWindow;
    osg::ref_ptr<osgEarth::ImageLayer> baseLayer;
};

namespace {

constexpr const char *kDefaultBaseMapFilename = "image/NE1_LR_LC_SR_W.tif";
constexpr const char *kResourceDirEnvVar = "THREEDVIEWER_RESOURCE_DIR";
constexpr const char *kDefaultResourceDir = "data";
constexpr double kDefaultFovDegrees = 30.0;
constexpr double kDefaultNearPlane = 1.0;
constexpr double kDefaultFarPlane = 1.0e8;
constexpr double kFlyToRangeMultiplier = 1.75;
constexpr double kFlyToMinRange = 200.0;
constexpr double kFlyToMaxRange = 4.0e7;
constexpr double kMetersPerDegLat = 111320.0;

std::string escapeXml(const std::string &input) {
    std::string output;
    output.reserve(input.size());
    for (char c : input) {
        switch (c) {
        case '&':  output += "&amp;"; break;
        case '<':  output += "&lt;"; break;
        case '>':  output += "&gt;"; break;
        case '"':  output += "&quot;"; break;
        case '\'': output += "&apos;"; break;
        default:   output += c; break;
        }
    }
    return output;
}

osg::ref_ptr<osgEarth::ImageLayer> createBaseLayer() {
    auto layer = osg::ref_ptr<osgEarth::GDALImageLayer>(new osgEarth::GDALImageLayer());
    layer->setName("Natural Earth");

    const char *envDir = std::getenv(kResourceDirEnvVar);
    if (envDir && envDir[0] != '\0') {
        std::string path = std::string(envDir) + "/" + kDefaultBaseMapFilename;
        layer->setURL(osgEarth::URI(path));
    } else {
        layer->setURL(osgEarth::URI(std::string(kDefaultResourceDir) + "/" + kDefaultBaseMapFilename));
    }

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

        osgEarth::Style lineStyle("line");
        osgEarth::Stroke lineStroke(osgEarth::Color(0.15f, 0.85f, 1.0f, 1.0f));
        lineStroke.width() = osgEarth::Expression<osgEarth::Distance>("2.5px");
        lineStyle.getOrCreate<osgEarth::LineSymbol>()->stroke() = lineStroke;
        lineStyle.getOrCreate<osgEarth::LineSymbol>()->tessellation() = 20;
        lineStyle.getOrCreate<osgEarth::RenderSymbol>()->depthTest() = true;

        osgEarth::Style polygonStyle("polygon");
        polygonStyle.getOrCreate<osgEarth::PolygonSymbol>()->fill() = osgEarth::Fill(osgEarth::Color(0.15f, 0.55f, 0.95f, 0.45f));
        osgEarth::Stroke polyStroke(osgEarth::Color(0.1f, 0.4f, 0.85f, 1.0f));
        polyStroke.width() = osgEarth::Expression<osgEarth::Distance>("1.5px");
        polygonStyle.getOrCreate<osgEarth::LineSymbol>()->stroke() = polyStroke;
        polygonStyle.getOrCreate<osgEarth::RenderSymbol>()->depthTest() = true;

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

void SceneController::frame() {
    if (!impl_->viewer) {
        return;
    }

    __try {
        impl_->viewer->frame();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        spdlog::error("SceneController::frame caught SEH exception (code={:#x})",
                     GetExceptionCode());
    }
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
            break;
        }
    }

    return result;
}

void SceneController::removeLayer(const std::string &id) {
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

void SceneController::resize(int /*width*/, int /*height*/) {
    if (!impl_->viewer || !impl_->graphicsWindow) {
        return;
    }

    int pixelW = 1;
    int pixelH = 1;
    if (impl_->hwnd) {
        RECT rc{};
        if (GetClientRect(impl_->hwnd, &rc)) {
            pixelW = (std::max)(1, static_cast<int>(rc.right - rc.left));
            pixelH = (std::max)(1, static_cast<int>(rc.bottom - rc.top));
        }
    }

    if (pixelW <= 1 && pixelH <= 1) {
        return;
    }

    updateViewport(*impl_->viewer, *impl_->graphicsWindow, pixelW, pixelH);
}

void SceneController::syncLayerState(const std::shared_ptr<Layer> &layer) {
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

    const std::string dtR = (rm->bandCount >= r && !rm->bands[r - 1].dataType.empty()) ? rm->bands[r - 1].dataType : "Byte";
    const std::string dtG = (rm->bandCount >= g && !rm->bands[g - 1].dataType.empty()) ? rm->bands[g - 1].dataType : "Byte";
    const std::string dtB = (rm->bandCount >= b && !rm->bands[b - 1].dataType.empty()) ? rm->bands[b - 1].dataType : "Byte";

    const std::string escapedUri = escapeXml(layer->sourceUri());

    const std::string vrtXml = std::string(
        "<VRTDataset rasterXSize=\"") + std::to_string(rm->rasterXSize) +
        "\" rasterYSize=\"" + std::to_string(rm->rasterYSize) + "\">"
        "<VRTRasterBand dataType=\"" + dtR + "\" band=\"1\">"
        "<SimpleSource><SourceFilename>" + escapedUri + "</SourceFilename>"
        "<SourceBand>" + std::to_string(r) + "</SourceBand>"
        "</SimpleSource>"
        "</VRTRasterBand>"
        "<VRTRasterBand dataType=\"" + dtG + "\" band=\"2\">"
        "<SimpleSource><SourceFilename>" + escapedUri + "</SourceFilename>"
        "<SourceBand>" + std::to_string(g) + "</SourceBand>"
        "</SimpleSource>"
        "</VRTRasterBand>"
        "<VRTRasterBand dataType=\"" + dtB + "\" band=\"3\">"
        "<SimpleSource><SourceFilename>" + escapedUri + "</SourceFilename>"
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
    const double mPerDegLon = (std::max)(1e-6, kMetersPerDegLat * std::cos(latRad));
    const double widthM = (bounds.east - bounds.west) * mPerDegLon;
    const double heightM = (bounds.north - bounds.south) * kMetersPerDegLat;
    const double maxDim = (std::max)(widthM, heightM);
    const double range = std::clamp(maxDim * kFlyToRangeMultiplier, kFlyToMinRange, kFlyToMaxRange);

    osgEarth::Viewpoint vp("layer-extent", lon, lat, 0.0, 0.0, -90.0, range);
    em->setViewpoint(vp, durationSeconds);
}

void SceneController::initializeDefaultScene(int width, int height) {
    spdlog::info("SceneController::initializeDefaultScene width={} height={}", width, height);
    if (!impl_ || impl_->initialized) {
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

    if (width > 0 && height > 0) {
        impl_->viewer->getCamera()->setProjectionMatrixAsPerspective(
            kDefaultFovDegrees,
            static_cast<double>(width) / static_cast<double>(height),
            kDefaultNearPlane,
            kDefaultFarPlane);
    }

    impl_->initialized = true;
    spdlog::info("SceneController: default scene initialized (graphics window deferred)");
    flushPendingLayers();
}

void SceneController::attachToNativeWindow(void *windowHandle, int width, int height) {
    if (!impl_->viewer || windowHandle == nullptr || width <= 0 || height <= 0) {
        return;
    }

    const auto hwnd = static_cast<HWND>(windowHandle);

    if (!impl_->graphicsWindow) {
        RECT rc{};
        int pixelW = width;
        int pixelH = height;
        if (GetClientRect(hwnd, &rc)) {
            pixelW = rc.right - rc.left;
            pixelH = rc.bottom - rc.top;
            if (pixelW <= 0) pixelW = width;
            if (pixelH <= 0) pixelH = height;
        }

        auto traits = osg::ref_ptr<osg::GraphicsContext::Traits>(new osg::GraphicsContext::Traits());
        traits->x = 0;
        traits->y = 0;
        traits->width = pixelW;
        traits->height = pixelH;
        traits->windowDecoration = false;
        traits->doubleBuffer = true;
        traits->sharedContext = nullptr;
        traits->inheritedWindowData =
            new osgViewer::GraphicsWindowWin32::WindowData(hwnd, true);

        auto graphicsWindow = osg::ref_ptr<osgViewer::GraphicsWindowWin32>(new osgViewer::GraphicsWindowWin32(traits.get()));
        if (!graphicsWindow->valid()) {
            spdlog::warn("SceneController: GraphicsWindowWin32 validation failed for HWND {}",
                         reinterpret_cast<ptrdiff_t>(windowHandle));
            return;
        }

        impl_->hwnd = hwnd;
        impl_->graphicsWindow = graphicsWindow.get();
        impl_->viewer->getCamera()->setGraphicsContext(graphicsWindow.get());
        impl_->viewer->getCamera()->setViewport(new osg::Viewport(0, 0, pixelW, pixelH));
        impl_->viewer->getCamera()->setProjectionMatrixAsPerspective(
            kDefaultFovDegrees,
            pixelH > 0 ? static_cast<double>(pixelW) / static_cast<double>(pixelH) : 1.0,
            kDefaultNearPlane,
            kDefaultFarPlane);
        impl_->viewer->realize();
        spdlog::info("SceneController: GraphicsWindowWin32 created, logical={}x{} pixel={}x{}", width, height, pixelW, pixelH);
    }

    const int sceneWidth = (std::max)(1, width);
    const int sceneHeight = (std::max)(1, height);
    resize(sceneWidth, sceneHeight);
}

void SceneController::flushPendingLayers() {
    auto pending = std::move(impl_->pendingLayers);
    impl_->pendingLayers.clear();
    for (const auto &layer : pending) {
        addLayer(layer);
    }
}

void SceneController::resetView() {
    if (!impl_->viewer) {
        return;
    }

    auto *em = dynamic_cast<osgEarth::EarthManipulator *>(impl_->viewer->getCameraManipulator());
    if (!em) {
        return;
    }

    constexpr double kDefaultRange = 3.0 * 6371000.0;
    osgEarth::Viewpoint vp("home", 0.0, 0.0, 0.0, 0.0, -90.0, kDefaultRange);
    em->setViewpoint(vp, 1.5);
}

QImage SceneController::captureImage() {
    if (!impl_->viewer || !impl_->graphicsWindow) {
        return QImage();
    }

    auto *camera = impl_->viewer->getCamera();
    osg::ref_ptr<osg::Image> osgImage = new osg::Image;
    camera->attach(osg::Camera::COLOR_BUFFER, osgImage.get());

    impl_->viewer->frame();

    camera->detach(osg::Camera::COLOR_BUFFER);

    if (!osgImage->valid()) {
        return QImage();
    }

    const int w = static_cast<int>(osgImage->s());
    const int h = static_cast<int>(osgImage->t());
    if (w <= 0 || h <= 0) {
        return QImage();
    }

    const unsigned char *data = osgImage->data();
    const GLint pixelFormat = osgImage->getPixelFormat();

    if (pixelFormat == GL_RGB || pixelFormat == GL_BGR) {
        QImage result(w, h, QImage::Format_RGB888);
        for (int y = 0; y < h; ++y) {
            const unsigned char *srcRow = data + y * osgImage->getRowSizeInBytes();
            unsigned char *dstRow = result.scanLine(h - 1 - y);
            if (pixelFormat == GL_RGB) {
                std::memcpy(dstRow, srcRow, static_cast<std::size_t>(w * 3));
            } else {
                for (int x = 0; x < w; ++x) {
                    dstRow[x * 3 + 0] = srcRow[x * 3 + 2];
                    dstRow[x * 3 + 1] = srcRow[x * 3 + 1];
                    dstRow[x * 3 + 2] = srcRow[x * 3 + 0];
                }
            }
        }
        return result;
    }

    if (pixelFormat == GL_RGBA || pixelFormat == GL_BGRA) {
        QImage result(w, h, QImage::Format_RGBA8888);
        for (int y = 0; y < h; ++y) {
            const unsigned char *srcRow = data + y * osgImage->getRowSizeInBytes();
            unsigned char *dstRow = result.scanLine(h - 1 - y);
            if (pixelFormat == GL_RGBA) {
                std::memcpy(dstRow, srcRow, static_cast<std::size_t>(w * 4));
            } else {
                for (int x = 0; x < w; ++x) {
                    dstRow[x * 4 + 0] = srcRow[x * 4 + 2];
                    dstRow[x * 4 + 1] = srcRow[x * 4 + 1];
                    dstRow[x * 4 + 2] = srcRow[x * 4 + 0];
                    dstRow[x * 4 + 3] = srcRow[x * 4 + 3];
                }
            }
        }
        return result;
    }

    return QImage();
}
