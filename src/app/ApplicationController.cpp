#include "app/ApplicationController.h"

#include <QMessageBox>
#include <QObject>
#include <QString>
#include <QStringList>
#include <vector>

#include "app/MainWindow.h"
#include "globe/GlobeWidget.h"
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
    QObject::connect(&window_, &MainWindow::importDataRequested, [this](const QString &path) {
        importFile(path.toStdString());
    });

    QObject::connect(&window_, &MainWindow::layerSelected, [this](const QString &layerId) {
        showLayerDetails(layerId.toStdString());
    });

    QObject::connect(&window_, &MainWindow::layerVisibilityChanged, [this](const QString &layerId, bool visible) {
        setLayerVisibility(layerId.toStdString(), visible);
    });

    QObject::connect(&window_, &MainWindow::layerOrderChanged, [this](const QStringList &orderedIds) {
        applyLayerOrderFromUi(orderedIds);
    });

    QObject::connect(window_.globeWidget(), &GlobeWidget::terrainPickCompleted, [this](const PickResult &pick) {
        handleTerrainPick(pick);
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

    if (const auto bounds = layer->geographicBounds(); bounds.has_value() && bounds->isValid()) {
        sceneController_.flyToGeographicBounds(*bounds);
    }
}

void ApplicationController::showLayerDetails(const std::string &layerId) {
    const auto layer = layerManager_.findById(layerId);
    if (!layer) {
        window_.showLayerDetails("Layer not found.");
        return;
    }

    QStringList lines;
    lines.append(QString("Name: %1").arg(QString::fromStdString(layer->name())));
    lines.append(QString("ID: %1").arg(QString::fromStdString(layer->id())));
    lines.append(QString("Type: %1").arg(layerKindToText(layer->kind())));
    lines.append(QString("Source: %1").arg(QString::fromStdString(layer->sourceUri())));
    lines.append(QString("Visible: %1").arg(layer->visible() ? "Yes" : "No"));
    lines.append(QString("Opacity: %1").arg(layer->opacity(), 0, 'f', 2));
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

void ApplicationController::applyLayerOrderFromUi(const QStringList &orderedIds) {
    std::vector<std::string> ids;
    ids.reserve(static_cast<std::size_t>(orderedIds.size()));
    for (const QString &q : orderedIds) {
        ids.push_back(q.toStdString());
    }

    if (!layerManager_.reorderLayers(ids)) {
        return;
    }

    sceneController_.reorderUserLayers(layerManager_.layers());
}

void ApplicationController::handleTerrainPick(const PickResult &pick) {
    if (!pick.hit) {
        window_.showLayerDetails("Pick: no terrain hit under cursor.");
        return;
    }

    QStringList lines;
    lines.append(QString("Lon: %1").arg(pick.longitude, 0, 'f', 6));
    lines.append(QString("Lat: %1").arg(pick.latitude, 0, 'f', 6));
    lines.append(QString("Elev (approx): %1").arg(pick.elevation, 0, 'f', 2));
    if (!pick.layerId.empty()) {
        lines.append(QString("Layer ID: %1").arg(QString::fromStdString(pick.layerId)));
    }
    if (!pick.displayText.empty()) {
        lines.append(QString::fromStdString(pick.displayText));
    }
    window_.showLayerDetails(lines.join('\n'));
}
