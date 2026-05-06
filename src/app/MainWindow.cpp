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
        const QString filter =
            "GIS Data (*.tif *.tiff *.img *.asc *.srtm *.hgt *.dem *.vrt);;"
            "Vector Data (*.shp *.geojson *.gpkg *.kml *.gml *.json);;"
            "All Files (*)";
        const QString path = QFileDialog::getOpenFileName(this, "Import Data", QString(), filter);
        if (!path.isEmpty()) {
            emit importDataRequested(path);
        }
    });

    connect(layerDock_, &LayerTreeDock::layerSelected, this, &MainWindow::layerSelected);
    connect(layerDock_, &LayerTreeDock::layerVisibilityChanged, this, &MainWindow::layerVisibilityChanged);
    connect(layerDock_, &LayerTreeDock::layerOrderChanged, this, &MainWindow::layerOrderChanged);
    connect(globeWidget_, &GlobeWidget::cursorTextChanged, statusController_, &StatusBarController::setCursorText);
    propertyDock_->showText("No layer selected.");
}

GlobeWidget *MainWindow::globeWidget() const {
    return globeWidget_;
}

void MainWindow::addLayerRow(const Layer &layer) {
    layerDock_->addLayer(layer.id(), layer.name(), layer.visible());
}

void MainWindow::showLayerDetails(const QString &text) {
    propertyDock_->showText(text);
}
