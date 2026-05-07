#include "globe/GlobeWidget.h"

#include <algorithm>
#include <QString>
#include <QStringLiteral>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QWheelEvent>
#include <QScreen>
#include <QWindow>

using namespace Qt::Literals::StringLiterals;

#include "globe/PickResult.h"
#include "tools/ToolManager.h"

namespace {

unsigned int mapQtButton(Qt::MouseButton button) {
    if (button == Qt::LeftButton) return 1;
    if (button == Qt::MiddleButton) return 2;
    if (button == Qt::RightButton) return 3;
    return 0;
}

qreal getDevicePixelRatio(const QWidget *widget) {
    if (widget && widget->windowHandle()) {
        return widget->windowHandle()->devicePixelRatio();
    }
    return widget ? widget->devicePixelRatioF() : 1.0;
}

}

GlobeWidget::GlobeWidget(QWidget *parent)
    : QWidget(parent),
      toolManager_(new ToolManager(this)),
      frameTimer_(new QTimer(this)),
      pickTimer_(new QTimer(this)) {
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setMouseTracking(true);
    frameTimer_->setInterval(16);
    connect(frameTimer_, &QTimer::timeout, this, [this]() {
        sceneController_.frame();
    });
    pickTimer_->setSingleShot(true);
    pickTimer_->setInterval(50);
    connect(pickTimer_, &QTimer::timeout, this, [this]() {
        performPick(lastPickX_, lastPickY_);
    });
}

SceneController &GlobeWidget::sceneController() {
    return sceneController_;
}

ToolManager &GlobeWidget::toolManager() {
    return *toolManager_;
}

void GlobeWidget::mouseMoveEvent(QMouseEvent *event) {
    const qreal dpr = getDevicePixelRatio(this);
    const float fx = static_cast<float>(event->position().x() * dpr);
    const float fy = static_cast<float>(event->position().y() * dpr);
    sceneController_.mouseMove(fx, fy);

    lastPickX_ = static_cast<int>(fx);
    lastPickY_ = static_cast<int>(fy);
    if (!pickTimer_->isActive()) {
        pickTimer_->start();
    }

    toolManager().mouseMoveEvent(*this, event);
    QWidget::mouseMoveEvent(event);
}

void GlobeWidget::mousePressEvent(QMouseEvent *event) {
    const qreal dpr = getDevicePixelRatio(this);
    const unsigned int button = mapQtButton(event->button());
    if (button != 0) {
        sceneController_.mousePress(
            static_cast<float>(event->position().x() * dpr),
            static_cast<float>(event->position().y() * dpr),
            button);
    }
    toolManager().mousePressEvent(*this, event);
    QWidget::mousePressEvent(event);
}

void GlobeWidget::mouseReleaseEvent(QMouseEvent *event) {
    const qreal dpr = getDevicePixelRatio(this);
    const unsigned int button = mapQtButton(event->button());
    if (button != 0) {
        sceneController_.mouseRelease(
            static_cast<float>(event->position().x() * dpr),
            static_cast<float>(event->position().y() * dpr),
            button);
    }
    toolManager().mouseReleaseEvent(*this, event);
    QWidget::mouseReleaseEvent(event);
}

void GlobeWidget::wheelEvent(QWheelEvent *event) {
    sceneController_.mouseScroll(event->angleDelta().y() > 0);
    QWidget::wheelEvent(event);
}

void GlobeWidget::resizeEvent(QResizeEvent *event) {
    const qreal dpr = getDevicePixelRatio(this);
    const int pw = static_cast<int>(event->size().width() * dpr);
    const int ph = static_cast<int>(event->size().height() * dpr);
    sceneController_.resize((std::max)(1, pw), (std::max)(1, ph));
    QWidget::resizeEvent(event);
}

void GlobeWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);

    const qreal dpr = getDevicePixelRatio(this);
    const int pw = static_cast<int>((std::max)(1, width()) * dpr);
    const int ph = static_cast<int>((std::max)(1, height()) * dpr);
    if (!sceneController_.isInitialized()) {
        sceneController_.initializeDefaultScene(pw, ph);
    }

    sceneController_.attachToNativeWindow(reinterpret_cast<void *>(winId()), pw, ph);
    if (!frameTimer_->isActive()) {
        frameTimer_->start();
    }
}

void GlobeWidget::performPick(int x, int y) {
    const PickResult pick = sceneController_.pickAt(x, y);
    if (pick.hit) {
        emit cursorTextChanged(QString(u"经度：%1，纬度：%2，高程：%3"_s)
                                   .arg(pick.longitude, 0, 'f', 6)
                                   .arg(pick.latitude, 0, 'f', 6)
                                   .arg(pick.elevation, 0, 'f', 2));
    } else {
        emit cursorTextChanged(u"经度：-，纬度：-，高程：-"_s);
    }
}
