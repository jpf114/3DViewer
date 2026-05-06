#include "globe/GlobeWidget.h"

#include <algorithm>
#include <QString>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QWheelEvent>

#include "globe/PickResult.h"
#include "tools/ToolManager.h"

namespace {

unsigned int mapQtButton(Qt::MouseButton button) {
    if (button == Qt::LeftButton) return 1;
    if (button == Qt::MiddleButton) return 2;
    if (button == Qt::RightButton) return 3;
    return 0;
}

}

GlobeWidget::GlobeWidget(QWidget *parent)
    : QWidget(parent),
      toolManager_(new ToolManager(this)),
      frameTimer_(new QTimer(this)) {
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setMouseTracking(true);
    frameTimer_->setInterval(16);
    connect(frameTimer_, &QTimer::timeout, this, [this]() {
        sceneController_.frame();
    });
}

SceneController &GlobeWidget::sceneController() {
    return sceneController_;
}

ToolManager &GlobeWidget::toolManager() {
    return *toolManager_;
}

void GlobeWidget::mouseMoveEvent(QMouseEvent *event) {
    const float fx = static_cast<float>(event->position().x());
    const float fy = flipY(static_cast<float>(event->position().y()));
    sceneController_.mouseMove(fx, fy);

    const PickResult pick = sceneController_.pickAt(static_cast<int>(fx), static_cast<int>(fy));
    if (pick.hit) {
        emit cursorTextChanged(QString("Lon: %1, Lat: %2, Elev: %3")
                                   .arg(pick.longitude, 0, 'f', 6)
                                   .arg(pick.latitude, 0, 'f', 6)
                                   .arg(pick.elevation, 0, 'f', 2));
    } else {
        emit cursorTextChanged("Lon: -, Lat: -, Elev: -");
    }
    toolManager().mouseMoveEvent(*this, event);
    QWidget::mouseMoveEvent(event);
}

void GlobeWidget::mousePressEvent(QMouseEvent *event) {
    const unsigned int button = mapQtButton(event->button());
    if (button != 0) {
        sceneController_.mousePress(
            static_cast<float>(event->position().x()),
            flipY(static_cast<float>(event->position().y())),
            button);
    }
    toolManager().mousePressEvent(*this, event);
    QWidget::mousePressEvent(event);
}

void GlobeWidget::mouseReleaseEvent(QMouseEvent *event) {
    const unsigned int button = mapQtButton(event->button());
    if (button != 0) {
        sceneController_.mouseRelease(
            static_cast<float>(event->position().x()),
            flipY(static_cast<float>(event->position().y())),
            button);
    }
    toolManager().mouseReleaseEvent(*this, event);
    if (event->button() == Qt::LeftButton) {
        const float fx = static_cast<float>(event->position().x());
        const float fy = flipY(static_cast<float>(event->position().y()));
        const PickResult pick = sceneController_.pickAt(static_cast<int>(fx), static_cast<int>(fy));
        emit terrainPickCompleted(pick);
    }
    QWidget::mouseReleaseEvent(event);
}

void GlobeWidget::wheelEvent(QWheelEvent *event) {
    sceneController_.mouseScroll(event->angleDelta().y() > 0);
    QWidget::wheelEvent(event);
}

void GlobeWidget::resizeEvent(QResizeEvent *event) {
    sceneController_.resize(event->size().width(), event->size().height());
    QWidget::resizeEvent(event);
}

void GlobeWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);

    const int sceneWidth = (std::max)(1, width());
    const int sceneHeight = (std::max)(1, height());
    if (!sceneController_.isInitialized()) {
        sceneController_.initializeDefaultScene(sceneWidth, sceneHeight);
    }

    sceneController_.attachToNativeWindow(reinterpret_cast<void *>(winId()), sceneWidth, sceneHeight);
    if (!frameTimer_->isActive()) {
        frameTimer_->start();
    }
}

float GlobeWidget::flipY(float y) const {
    return static_cast<float>(height()) - y;
}
