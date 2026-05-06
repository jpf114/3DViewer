#include "ui/StatusBarController.h"

#include <QLabel>
#include <QMainWindow>
#include <QStatusBar>

StatusBarController::StatusBarController(QMainWindow *window)
    : QObject(window), cursorLabel_(new QLabel("Lon: -, Lat: -, Elev: -", window)) {
    window->statusBar()->addPermanentWidget(cursorLabel_);
}

void StatusBarController::setCursorText(const QString &text) {
    cursorLabel_->setText(text);
}
