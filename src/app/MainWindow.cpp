#include "app/MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QMenuBar>
#include <QString>

#include "globe/GlobeWidget.h"
#include "layers/Layer.h"
#include "ui/LayerTreeDock.h"
#include "ui/PropertyDock.h"
#include "ui/StatusBarController.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      globeWidget_(new GlobeWidget(this)),
      layerDock_(new LayerTreeDock(this)),
      propertyDock_(new PropertyDock(this)),
      statusController_(new StatusBarController(this)) {
    setWindowTitle("3DViewer");
    resize(1600, 900);
    setCentralWidget(globeWidget_);
    addDockWidget(Qt::LeftDockWidgetArea, layerDock_);
    addDockWidget(Qt::RightDockWidgetArea, propertyDock_);

    auto *importAction = menuBar()->addAction("Import Data...");
    connect(importAction, &QAction::triggered, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this, "Import Data");
        if (!path.isEmpty()) {
            emit importDataRequested(path);
        }
    });

    connect(layerDock_, &LayerTreeDock::layerVisibilityChanged, this, &MainWindow::layerVisibilityChanged);
}

GlobeWidget *MainWindow::globeWidget() const {
    return globeWidget_;
}

void MainWindow::addLayerRow(const Layer &layer) {
    layerDock_->addLayer(layer.id(), layer.name(), layer.visible());
}
