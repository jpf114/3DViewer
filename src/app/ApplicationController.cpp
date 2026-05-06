#include "app/ApplicationController.h"

#include <QMessageBox>
#include <QObject>
#include <QString>
#include <QStringList>

#include "app/MainWindow.h"
#include "data/DataImporter.h"
#include "globe/SceneController.h"
#include "layers/Layer.h"
#include "layers/LayerManager.h"

namespace {

QString layerKindToText(LayerKind kind) {
    switch (kind) {
    case LayerKind::Imagery:
        return "Imagery";
    case LayerKind::Elevation:
        return "Elevation";
    case LayerKind::Vector:
        return "Vector";
    case LayerKind::Chart:
        return "Chart";
    case LayerKind::Scientific:
        return "Scientific";
    }

    return "Unknown";
}

}

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

    QObject::connect(&window_, &MainWindow::layerSelected, &window_, [this](const QString &layerId) {
        showLayerDetails(layerId.toStdString());
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

void ApplicationController::showLayerDetails(const std::string &layerId) {
    const auto layer = layerManager_.findById(layerId);
    if (!layer) {
        window_.showLayerDetails("Layer not found.");
        return;
    }

    const QStringList lines = {
        QString("Name: %1").arg(QString::fromStdString(layer->name())),
        QString("ID: %1").arg(QString::fromStdString(layer->id())),
        QString("Type: %1").arg(layerKindToText(layer->kind())),
        QString("Source: %1").arg(QString::fromStdString(layer->sourceUri())),
        QString("Visible: %1").arg(layer->visible() ? "Yes" : "No"),
        QString("Opacity: %1").arg(layer->opacity(), 0, 'f', 2)};
    window_.showLayerDetails(lines.join('\n'));
}

void ApplicationController::setLayerVisibility(const std::string &layerId, bool visible) {
    layerManager_.setVisibility(layerId, visible);
    const auto layer = layerManager_.findById(layerId);
    if (layer) {
        sceneController_.syncLayerState(layer);
        showLayerDetails(layerId);
    }
}
