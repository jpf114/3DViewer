#include "globe/SceneController.h"

#include <osg/Camera>
#include <osg/Viewport>
#include <osgEarth/EarthManipulator>
#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarth/Profile>
#include <osgEarth/URI>
#include <osgEarth/XYZ>
#include <osgGA/EventQueue>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>

#include "globe/PickResult.h"
#include "layers/Layer.h"

struct SceneController::Impl {
    bool initialized = false;
    std::unordered_map<std::string, bool> layerVisibility;
    osg::ref_ptr<osgEarth::Map> map;
    osg::ref_ptr<osgEarth::MapNode> mapNode;
    osg::ref_ptr<osgViewer::Viewer> viewer;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> graphicsWindow;
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

void updateViewport(osgViewer::Viewer &viewer, osgViewer::GraphicsWindowEmbedded &graphicsWindow, int width, int height) {
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
    result.hit = false;
    result.longitude = static_cast<double>(x);
    result.latitude = static_cast<double>(y);
    result.displayText = "Pick stub";
    return result;
}

void SceneController::removeLayer(const std::string &id) {
    impl_->layerVisibility.erase(id);
}

void SceneController::resize(int width, int height) {
    if (!impl_->viewer || !impl_->graphicsWindow) {
        return;
    }

    updateViewport(*impl_->viewer, *impl_->graphicsWindow, width, height);
}

void SceneController::syncLayerState(const std::shared_ptr<Layer> &layer) {
    impl_->layerVisibility[layer->id()] = layer->visible();
}

void SceneController::initializeDefaultScene(int width, int height) {
    if (impl_->initialized) {
        resize(width, height);
        return;
    }

    impl_->map = new osgEarth::Map();
    impl_->baseLayer = createBaseLayer();
    impl_->map->addLayer(impl_->baseLayer.get());

    impl_->mapNode = new osgEarth::MapNode(impl_->map.get());
    impl_->viewer = new osgViewer::Viewer();
    impl_->viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
    impl_->viewer->setCameraManipulator(new osgEarth::EarthManipulator());
    impl_->viewer->setSceneData(impl_->mapNode.get());

    impl_->graphicsWindow = impl_->viewer->setUpViewerAsEmbeddedInWindow(0, 0, width, height);
    updateViewport(*impl_->viewer, *impl_->graphicsWindow, width, height);
    impl_->viewer->getCamera()->setProjectionMatrixAsPerspective(
        30.0,
        height > 0 ? static_cast<double>(width) / static_cast<double>(height) : 1.0,
        1.0,
        100000000.0);

    impl_->initialized = true;
}
