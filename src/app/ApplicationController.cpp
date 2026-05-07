#include "app/ApplicationController.h"

#include <QMessageBox>
#include <QObject>
#include <QStatusBar>
#include <QString>
#include <QStringList>
#include <spdlog/spdlog.h>
#include <vector>

#include "app/MainWindow.h"
#include "globe/GlobeWidget.h"
#include "data/DataImporter.h"
#include "globe/SceneController.h"
#include "layers/Layer.h"
#include "layers/LayerManager.h"
#include "tools/ToolManager.h"

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

    QObject::connect(&window_, &MainWindow::removeLayerRequested, [this](const QString &layerId) {
        removeLayer(layerId.toStdString());
    });

    QObject::connect(&window_, &MainWindow::layerOpacityChanged, [this](const QString &layerId, double opacity) {
        setLayerOpacity(layerId.toStdString(), opacity);
    });

    QObject::connect(&window_, &MainWindow::bandMappingChanged, [this](const QString &layerId, int red, int green, int blue) {
        setBandMapping(layerId.toStdString(), red, green, blue);
    });

    QObject::connect(&window_, &MainWindow::toolChanged, [this](int toolId) {
        window_.globeWidget()->toolManager().setActiveTool(static_cast<ToolId>(toolId));
    });

    QObject::connect(window_.globeWidget(), &GlobeWidget::terrainPickCompleted, [this](const PickResult &pick) {
        handleTerrainPick(pick);
    });
}

void ApplicationController::importFile(const std::string &path) {
    spdlog::info("ApplicationController: importing '{}'", path);
    window_.statusBar()->showMessage(QString("Importing %1...").arg(QString::fromStdString(path)));

    const auto layer = importer_.import(path);
    if (!layer) {
        spdlog::warn("ApplicationController: import failed for '{}'", path);
        window_.statusBar()->showMessage("Import failed.", 3000);
        QMessageBox::warning(&window_, "Import Failed",
                             QString("Unsupported or unreadable data source:\n%1\n\n"
                                     "Supported formats include GeoTIFF (.tif), IMG (.img), Shapefile (.shp),\n"
                                     "GeoJSON (.geojson), GeoPackage (.gpkg), KML (.kml), and more.")
                                 .arg(QString::fromStdString(path)));
        return;
    }

    if (!layerManager_.addLayer(layer)) {
        spdlog::warn("ApplicationController: duplicate layer for '{}'", path);
        window_.statusBar()->showMessage("Layer already exists.", 3000);
        QMessageBox::information(&window_, "Already Imported",
                                 QString("This file has already been imported:\n%1").arg(QString::fromStdString(path)));
        return;
    }

    sceneController_.addLayer(layer);
    window_.addLayerRow(*layer);
    spdlog::info("ApplicationController: layer '{}' imported successfully", layer->id());

    window_.statusBar()->showMessage(
        QString("Imported: %1 (%2)").arg(QString::fromStdString(layer->name())).arg(layerKindToText(layer->kind())),
        5000);

    if (const auto bounds = layer->geographicBounds(); bounds.has_value() && bounds->isValid()) {
        sceneController_.flyToGeographicBounds(*bounds);
    }
}

void ApplicationController::showLayerDetails(const std::string &layerId) {
    const auto layer = layerManager_.findById(layerId);
    if (!layer) {
        window_.showLayerDetails("Layer not found.");
        window_.clearLayerProperties();
        return;
    }

    QStringList lines;
    lines.append(QString("Name: %1").arg(QString::fromStdString(layer->name())));
    lines.append(QString("ID: %1").arg(QString::fromStdString(layer->id())));
    lines.append(QString("Type: %1").arg(layerKindToText(layer->kind())));
    lines.append(QString("Source: %1").arg(QString::fromStdString(layer->sourceUri())));
    lines.append(QString("Visible: %1").arg(layer->visible() ? "Yes" : "No"));
    lines.append(QString("Opacity: %1").arg(layer->opacity(), 0, 'f', 2));

    if (const auto &rm = layer->rasterMetadata()) {
        lines.append("");
        lines.append("--- Raster Info ---");
        lines.append(QString("Driver: %1").arg(QString::fromStdString(rm->driverName)));
        lines.append(QString("Size: %1 x %2").arg(rm->rasterXSize).arg(rm->rasterYSize));
        lines.append(QString("Bands: %1").arg(rm->bandCount));
        if (!rm->epsgCode.empty()) {
            lines.append(QString("CRS: %1").arg(QString::fromStdString(rm->epsgCode)));
        }
        if (rm->pixelSizeX > 0 || rm->pixelSizeY > 0) {
            lines.append(QString("Pixel Size: %1 x %2").arg(rm->pixelSizeX, 0, 'f', 6).arg(rm->pixelSizeY, 0, 'f', 6));
        }
        for (const auto &band : rm->bands) {
            lines.append(QString("  Band %1: %2 [%3] CI=%4")
                             .arg(band.index)
                             .arg(QString::fromStdString(band.dataType))
                             .arg(band.description.empty() ? "-" : QString::fromStdString(band.description))
                             .arg(QString::fromStdString(band.colorInterpretation)));
            if (band.hasMinMax) {
                lines.append(QString("    Range: [%1, %2]").arg(band.min, 0, 'f', 2).arg(band.max, 0, 'f', 2));
            }
            if (band.hasNoDataValue) {
                lines.append(QString("    NoData: %1").arg(band.noDataValue, 0, 'f', 2));
            }
        }
    }

    if (const auto &vm = layer->vectorMetadata()) {
        lines.append("");
        lines.append("--- Vector Info ---");
        lines.append(QString("Layer: %1").arg(QString::fromStdString(vm->name)));
        lines.append(QString("Geometry: %1").arg(QString::fromStdString(vm->geometryType)));
        lines.append(QString("Features: %1").arg(vm->featureCount));
        if (!vm->epsgCode.empty()) {
            lines.append(QString("CRS: %1").arg(QString::fromStdString(vm->epsgCode)));
        }
        if (!vm->fields.empty()) {
            lines.append(QString("Fields (%1):").arg(vm->fields.size()));
            for (const auto &field : vm->fields) {
                lines.append(QString("  %1 (%2)").arg(QString::fromStdString(field.name)).arg(QString::fromStdString(field.typeName)));
            }
        }
    }

    window_.showLayerProperties(QString::fromStdString(layerId), lines.join('\n'), layer->opacity(),
                                layer->rasterMetadata().has_value() ? layer->rasterMetadata()->bandCount : 0);
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
        lines.append(QString("Layer: %1").arg(QString::fromStdString(pick.displayText)));
    }

    if (!pick.featureAttributes.empty()) {
        lines.append("");
        lines.append("--- Feature Attributes ---");
        for (const auto &attr : pick.featureAttributes) {
            lines.append(QString("  %1: %2").arg(QString::fromStdString(attr.name)).arg(QString::fromStdString(attr.value)));
        }
    }

    window_.showLayerDetails(lines.join('\n'));
}

void ApplicationController::removeLayer(const std::string &layerId) {
    spdlog::info("ApplicationController: removing layer '{}'", layerId);
    sceneController_.removeLayer(layerId);
    layerManager_.removeLayer(layerId);
    window_.removeLayerRow(layerId);
    window_.showLayerDetails("No layer selected.");
    window_.clearLayerProperties();
}

void ApplicationController::setLayerOpacity(const std::string &layerId, double opacity) {
    const auto layer = layerManager_.findById(layerId);
    if (!layer) {
        return;
    }

    layer->setOpacity(opacity);
    sceneController_.syncLayerState(layer);
}

void ApplicationController::setBandMapping(const std::string &layerId, int red, int green, int blue) {
    const auto layer = layerManager_.findById(layerId);
    if (!layer || layer->kind() != LayerKind::Imagery) {
        return;
    }

    layer->setBandMapping(red, green, blue);
    sceneController_.updateImageLayerBands(layer);
    spdlog::info("ApplicationController: band mapping changed for '{}' to R={} G={} B={}", layerId, red, green, blue);
}
