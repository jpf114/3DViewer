#include "app/MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QMenuBar>
#include <QMimeData>
#include <QToolBar>
#include <QUrl>

#include "globe/GlobeWidget.h"
#include "layers/Layer.h"
#include "tools/ToolManager.h"
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
    setAcceptDrops(true);
    setCentralWidget(globeWidget_);
    addDockWidget(Qt::LeftDockWidgetArea, layerDock_);
    addDockWidget(Qt::RightDockWidgetArea, propertyDock_);

    auto *fileMenu = menuBar()->addMenu("&File");
    auto *importAction = fileMenu->addAction("Import Data...");
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

    auto *toolBar = addToolBar("Tools");
    toolBar->setMovable(false);

    toolGroup_ = new QActionGroup(this);
    panAction_ = toolBar->addAction("Pan");
    panAction_->setCheckable(true);
    panAction_->setChecked(true);
    panAction_->setActionGroup(toolGroup_);

    pickAction_ = toolBar->addAction("Pick");
    pickAction_->setCheckable(true);
    pickAction_->setActionGroup(toolGroup_);

    connect(toolGroup_, &QActionGroup::triggered, this, [this](QAction *action) {
        if (action == panAction_) {
            emit toolChanged(static_cast<int>(ToolId::Pan));
        } else if (action == pickAction_) {
            emit toolChanged(static_cast<int>(ToolId::Pick));
        }
    });

    connect(layerDock_, &LayerTreeDock::layerSelected, this, &MainWindow::layerSelected);
    connect(layerDock_, &LayerTreeDock::layerVisibilityChanged, this, &MainWindow::layerVisibilityChanged);
    connect(layerDock_, &LayerTreeDock::layerOrderChanged, this, &MainWindow::layerOrderChanged);
    connect(layerDock_, &LayerTreeDock::removeLayerRequested, this, &MainWindow::removeLayerRequested);
    connect(propertyDock_, &PropertyDock::opacityChanged, this, &MainWindow::layerOpacityChanged);
    connect(propertyDock_, &PropertyDock::bandMappingChanged, this, &MainWindow::bandMappingChanged);
    connect(globeWidget_, &GlobeWidget::cursorTextChanged, statusController_, &StatusBarController::setCursorText);
    propertyDock_->showText("No layer selected.");
}

GlobeWidget *MainWindow::globeWidget() const {
    return globeWidget_;
}

void MainWindow::addLayerRow(const Layer &layer) {
    layerDock_->addLayer(layer.id(), layer.name(), layer.visible());
}

void MainWindow::removeLayerRow(const std::string &layerId) {
    layerDock_->removeLayer(layerId);
}

void MainWindow::showLayerDetails(const QString &text) {
    propertyDock_->showText(text);
}

void MainWindow::showLayerProperties(const QString &layerId, const QString &infoText, double opacity, int bandCount) {
    propertyDock_->showLayerProperties(layerId, infoText, opacity, bandCount);
}

void MainWindow::clearLayerProperties() {
    propertyDock_->clearLayerProperties();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            emit importDataRequested(url.toLocalFile());
        }
    }
}
