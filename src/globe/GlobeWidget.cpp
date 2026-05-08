#include "globe/GlobeWidget.h"

#include <algorithm>
#include <QString>
#include <QStringLiteral>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QWheelEvent>

using namespace Qt::Literals::StringLiterals;

#include "globe/PickResult.h"
#include "globe/InputMapping.h"
#include "tools/ToolManager.h"

namespace {

unsigned int mapQtButton(Qt::MouseButton button) {
    if (button == Qt::LeftButton) return 1;
    if (button == Qt::MiddleButton) return 2;
    if (button == Qt::RightButton) return 3;
    return 0;
}

QSize pixelSceneSize(const QWidget &widget) {
    const qreal dpr = widget.devicePixelRatioF();
    const int pixelWidth = (std::max)(1, static_cast<int>(std::lround(widget.width() * dpr)));
    const int pixelHeight = (std::max)(1, static_cast<int>(std::lround(widget.height() * dpr)));
    return QSize(pixelWidth, pixelHeight);
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
    const QPointF scenePos = globe::mapToScenePoint(event->position(), size(), pixelSceneSize(*this));
    sceneController_.mouseMove(static_cast<float>(scenePos.x()), static_cast<float>(scenePos.y()));

    if (globe::shouldScheduleHoverPick(event->buttons(), toolManager().activeToolId())) {
        lastPickX_ = static_cast<int>(std::lround(scenePos.x()));
        lastPickY_ = static_cast<int>(std::lround(scenePos.y()));
        pickTimer_->start();
    } else if (pickTimer_->isActive()) {
        pickTimer_->stop();
    }

    toolManager().mouseMoveEvent(*this, event);
    QWidget::mouseMoveEvent(event);
}

void GlobeWidget::mousePressEvent(QMouseEvent *event) {
    const unsigned int button = mapQtButton(event->button());
    if (button != 0) {
        const QPointF scenePos = globe::mapToScenePoint(event->position(), size(), pixelSceneSize(*this));
        sceneController_.mousePress(
            static_cast<float>(scenePos.x()),
            static_cast<float>(scenePos.y()),
            button);
    }
    pickTimer_->stop();
    toolManager().mousePressEvent(*this, event);
    QWidget::mousePressEvent(event);
}

void GlobeWidget::mouseReleaseEvent(QMouseEvent *event) {
    const unsigned int button = mapQtButton(event->button());
    if (button != 0) {
        const QPointF scenePos = globe::mapToScenePoint(event->position(), size(), pixelSceneSize(*this));
        sceneController_.mouseRelease(
            static_cast<float>(scenePos.x()),
            static_cast<float>(scenePos.y()),
            button);
        lastPickX_ = static_cast<int>(std::lround(scenePos.x()));
        lastPickY_ = static_cast<int>(std::lround(scenePos.y()));
    }
    toolManager().mouseReleaseEvent(*this, event);
    if (globe::shouldScheduleHoverPick(event->buttons(), toolManager().activeToolId())) {
        pickTimer_->start();
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
