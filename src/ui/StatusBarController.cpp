#include "ui/StatusBarController.h"

#include <QLabel>
#include <QMainWindow>
#include <QStatusBar>
#include <QStringLiteral>

using namespace Qt::Literals::StringLiterals;

StatusBarController::StatusBarController(QMainWindow *window)
    : QObject(window),
      cursorLabel_(new QLabel(u"经度：-，纬度：-，高程：-"_s, window)),
      measurementLabel_(new QLabel(u"测距：未开始"_s, window)) {
    window->statusBar()->addPermanentWidget(cursorLabel_);
    window->statusBar()->addPermanentWidget(measurementLabel_);
}

void StatusBarController::setCursorText(const QString &text) {
    cursorLabel_->setText(text);
}

void StatusBarController::setMeasurementText(const QString &text) {
    measurementLabel_->setText(text);
}
