#include "app/ApplicationController.h"

#include <QMessageBox>
#include <QObject>
#include <QString>

#include "app/MainWindow.h"
#include "data/DataImporter.h"
#include "globe/SceneController.h"
#include "layers/Layer.h"
#include "layers/LayerManager.h"

ApplicationController::ApplicationController(MainWindow &window,
                                             SceneController &sceneController,
                                             LayerManager &layerManager,
                                             DataImporter &importer)
    : window_(window),
      sceneController_(sceneController),
      layerManager_(layerManager),
      importer_(importer) {
    QObject::connect(&window_, &MainWindow::importDataRequested, &window_, [this](const QString &path) {
        importFile(path.toStdString());
    });

    QObject::connect(&window_, &MainWindow::layerVisibilityChanged, &window_, [this](const QString &layerId, bool visible) {
        setLayerVisibility(layerId.toStdString(), visible);
    });
}

void ApplicationController::importFile(const std::string &path) {
    const auto layer = importer_.import(path);
    if (!layer) {
        QMessageBox::warning(&window_, "Import Failed", QString("Unsupported or unreadable data source:\n%1").arg(QString::fromStdString(path)));
        return;
    }

    layerManager_.addLayer(layer);
    sceneController_.addLayer(layer);
    window_.addLayerRow(*layer);
}

void ApplicationController::setLayerVisibility(const std::string &layerId, bool visible) {
    layerManager_.setVisibility(layerId, visible);
    const auto layer = layerManager_.findById(layerId);
    if (layer) {
        sceneController_.syncLayerState(layer);
    }
}
