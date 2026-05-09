#include "ui/StatusBarController.h"

#include <QLabel>
#include <QMainWindow>
#include <QStatusBar>

StatusBarController::StatusBarController(QMainWindow *window)
    : QObject(window),
      cursorLabel_(new QLabel(QString::fromUtf8(u8"经度：-，纬度：-，高程：-"), window)),
      measurementLabel_(new QLabel(QString::fromUtf8(u8"量测：未激活"), window)) {
    window->statusBar()->addPermanentWidget(cursorLabel_);
    window->statusBar()->addPermanentWidget(measurementLabel_);
}

void StatusBarController::setCursorText(const QString &text) {
    cursorLabel_->setText(text);
}

void StatusBarController::setMeasurementText(const QString &text) {
    measurementLabel_->setText(text);
}
