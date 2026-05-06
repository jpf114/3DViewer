#include "globe/GlobeWidget.h"

#include <algorithm>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QWheelEvent>

#include "tools/ToolManager.h"

GlobeWidget::GlobeWidget(QWidget *parent)
    : QWidget(parent),
      toolManager_(new ToolManager()),
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
    sceneController_.mouseMove(static_cast<float>(event->position().x()), flipY(static_cast<float>(event->position().y())));
    toolManager().mouseMoveEvent(*this, event);
    QWidget::mouseMoveEvent(event);
}

void GlobeWidget::mousePressEvent(QMouseEvent *event) {
    unsigned int button = 0;
    if (event->button() == Qt::LeftButton) {
        button = 1;
    } else if (event->button() == Qt::MiddleButton) {
        button = 2;
    } else if (event->button() == Qt::RightButton) {
        button = 3;
    }
    if (button != 0) {
        sceneController_.mousePress(static_cast<float>(event->position().x()), flipY(static_cast<float>(event->position().y())), button);
    }
    toolManager().mousePressEvent(*this, event);
    QWidget::mousePressEvent(event);
}

void GlobeWidget::mouseReleaseEvent(QMouseEvent *event) {
    unsigned int button = 0;
    if (event->button() == Qt::LeftButton) {
        button = 1;
    } else if (event->button() == Qt::MiddleButton) {
        button = 2;
    } else if (event->button() == Qt::RightButton) {
        button = 3;
    }
    if (button != 0) {
        sceneController_.mouseRelease(static_cast<float>(event->position().x()), flipY(static_cast<float>(event->position().y())), button);
    }
    toolManager().mouseReleaseEvent(*this, event);
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

    const int sceneWidth = std::max(1, width());
    const int sceneHeight = std::max(1, height());
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
