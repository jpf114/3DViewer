#include "globe/GlobeWidget.h"

#include <QTimer>
#include <QWheelEvent>

#include "tools/ToolManager.h"

GlobeWidget::GlobeWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      toolManager_(new ToolManager()),
      frameTimer_(new QTimer(this)) {
    frameTimer_->setInterval(16);
    connect(frameTimer_, &QTimer::timeout, this, QOverload<>::of(&GlobeWidget::update));
}

SceneController &GlobeWidget::sceneController() {
    return sceneController_;
}

ToolManager &GlobeWidget::toolManager() {
    return *toolManager_;
}

void GlobeWidget::initializeGL() {
    sceneController_.initializeDefaultScene(width(), height());
    frameTimer_->start();
}

void GlobeWidget::paintGL() {
    sceneController_.frame();
}

void GlobeWidget::resizeGL(int width, int height) {
    sceneController_.resize(width, height);
}

void GlobeWidget::mouseMoveEvent(QMouseEvent *event) {
    sceneController_.mouseMove(static_cast<float>(event->position().x()), flipY(static_cast<float>(event->position().y())));
    toolManager().mouseMoveEvent(*this, event);
    QOpenGLWidget::mouseMoveEvent(event);
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
    QOpenGLWidget::mousePressEvent(event);
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
    QOpenGLWidget::mouseReleaseEvent(event);
}

void GlobeWidget::wheelEvent(QWheelEvent *event) {
    sceneController_.mouseScroll(event->angleDelta().y() > 0);
    QOpenGLWidget::wheelEvent(event);
}

float GlobeWidget::flipY(float y) const {
    return static_cast<float>(height()) - y;
}
