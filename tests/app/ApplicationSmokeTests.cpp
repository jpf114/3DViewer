#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <type_traits>

#include <QAction>
#include <QApplication>
#include <QDoubleSpinBox>
#include <QDir>
#include <QFile>
#include <QKeyEvent>
#include <QMenu>
#include <QMenuBar>
#include <QTemporaryDir>
#include <QTableWidget>
#include <QTextEdit>
#include <QTreeWidget>

#include "app/ApplicationController.h"
#include "app/MainWindow.h"
#include "data/DataImporter.h"
#include "data/DataSourceDescriptor.h"
#include "globe/GlobeWidget.h"
#include "layers/Layer.h"
#include "layers/LayerManager.h"
#include "layers/LayerTypes.h"
#include "ui/LayerTreeDock.h"
#include "ui/PropertyDock.h"

namespace {

bool require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    static_assert(std::is_base_of_v<QMainWindow, MainWindow>);

    MainWindow window;
    bool undoRequested = false;
    QObject::connect(&window, &MainWindow::undoMeasurementRequested, [&undoRequested]() {
        undoRequested = true;
    });
    QKeyEvent backspaceEvent(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
    QApplication::sendEvent(&window, &backspaceEvent);
    if (!require(undoRequested, "backspace should request undo")) {
        return EXIT_FAILURE;
    }

    bool editRequested = false;
    QObject::connect(&window, &MainWindow::editSelectedMeasurementRequested, [&editRequested]() {
        editRequested = true;
    });

    auto *editAction = window.findChild<QAction *>("editMeasureAction");
    auto *undoAction = window.findChild<QAction *>("undoMeasureAction");
    auto *clearAction = window.findChild<QAction *>("clearMeasureAction");
    if (!require(editAction != nullptr && undoAction != nullptr && clearAction != nullptr,
                 "measurement toolbar actions should exist")) {
        return EXIT_FAILURE;
    }
    if (!require(!editAction->isEnabled() && !undoAction->isEnabled() && !clearAction->isEnabled(),
                 "measurement toolbar actions should be disabled initially")) {
        return EXIT_FAILURE;
    }

    window.setActiveToolAction(1);
    if (!require(!undoAction->isEnabled() && !clearAction->isEnabled(),
                 "undo and clear should stay disabled in pick mode")) {
        return EXIT_FAILURE;
    }

    window.setActiveToolAction(2);
    if (!require(undoAction->isEnabled() && clearAction->isEnabled(),
                 "undo and clear should enable in measure mode")) {
        return EXIT_FAILURE;
    }

    MeasurementLayerData measurementData;
    measurementData.kind = MeasurementKind::Distance;
    measurementData.points.push_back({120.0, 30.0});
    measurementData.points.push_back({121.0, 31.0});
    window.showLayerProperties("measurement-1",
                               "Measure 1",
                               QString::fromUtf8(u8"量测"),
                               "memory",
                               true,
                               1.0,
                               std::nullopt,
                               std::nullopt,
                               std::nullopt,
                               measurementData);
    if (!require(editAction->isEnabled(), "edit action should enable for measurement layer")) {
        return EXIT_FAILURE;
    }
    editAction->trigger();
    if (!require(editRequested, "edit action should emit measurement edit request")) {
        return EXIT_FAILURE;
    }
    window.showLayerDetails(QString::fromUtf8(u8"经度：120.123456\n纬度：30.654321"));
    if (!require(!editAction->isEnabled(), "non-layer details should clear measurement edit state")) {
        return EXIT_FAILURE;
    }

    PropertyDock dock;
    auto *textEdit = dock.findChild<QTextEdit *>();
    if (!require(textEdit != nullptr, "property dock text edit should exist")) {
        return EXIT_FAILURE;
    }
    if (!require(textEdit->maximumHeight() != 64, "property dock text should not be clamped to old fixed height")) {
        return EXIT_FAILURE;
    }
    if (!require(textEdit->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff &&
                     textEdit->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff,
                 "property dock text should not show inner scrollbars")) {
        return EXIT_FAILURE;
    }

    ModelPlacement placement;
    placement.longitude = 120.123456;
    placement.latitude = 30.654321;
    placement.height = 88.5;
    placement.scale = 2.0;
    placement.heading = 45.0;

    QString emittedLayerId;
    ModelPlacement emittedPlacement;
    bool signalReceived = false;
    QObject::connect(&dock, &PropertyDock::modelPlacementChanged,
                     [&emittedLayerId, &emittedPlacement, &signalReceived](const QString &layerId,
                                                                           const ModelPlacement &updatedPlacement) {
        emittedLayerId = layerId;
        emittedPlacement = updatedPlacement;
        signalReceived = true;
    });

    dock.showLayerProperties("model-1",
                             "Building",
                             QString::fromUtf8(u8"三维模型"),
                             "D:/demo/building.glb",
                             true,
                             1.0,
                             std::nullopt,
                             std::nullopt,
                             placement);
    if (!require(textEdit->toPlainText().contains(QString::fromUtf8(u8"调整经纬度")),
                 "model summary text should mention placement editing")) {
        return EXIT_FAILURE;
    }

    auto *longitudeSpin = dock.findChild<QDoubleSpinBox *>("modelLongitudeSpin");
    auto *scaleSpin = dock.findChild<QDoubleSpinBox *>("modelScaleSpin");
    if (!require(longitudeSpin != nullptr && scaleSpin != nullptr, "model placement spin boxes should exist")) {
        return EXIT_FAILURE;
    }
    if (!require(std::abs(longitudeSpin->value() - placement.longitude) <= 1.0e-6 &&
                     std::abs(scaleSpin->value() - placement.scale) <= 1.0e-6,
                 "model placement values should be loaded into spin boxes")) {
        return EXIT_FAILURE;
    }

    scaleSpin->setValue(3.5);
    if (!require(signalReceived && emittedLayerId == "model-1",
                 "changing scale should emit model placement changed")) {
        return EXIT_FAILURE;
    }
    if (!require(std::abs(emittedPlacement.scale - 3.5) <= 1.0e-6 &&
                     std::abs(emittedPlacement.longitude - placement.longitude) <= 1.0e-6,
                 "emitted model placement should preserve untouched values")) {
        return EXIT_FAILURE;
    }

    MeasurementLayerData dockMeasurementData;
    dockMeasurementData.kind = MeasurementKind::Area;
    dockMeasurementData.points.push_back({120.0, 30.0});
    dockMeasurementData.points.push_back({121.0, 30.0});
    dockMeasurementData.points.push_back({121.0, 31.0});
    dockMeasurementData.lengthMeters = 3000.0;
    dockMeasurementData.areaSquareMeters = 250000.0;
    dock.showLayerProperties("measurement-2",
                             "Measure Area",
                             QString::fromUtf8(u8"量测"),
                             "memory",
                             true,
                             1.0,
                             std::nullopt,
                             std::nullopt,
                             std::nullopt,
                             dockMeasurementData);
    if (!require(textEdit->toPlainText().contains(QString::fromUtf8(u8"编辑量测")),
                 "measurement summary text should mention edit action")) {
        return EXIT_FAILURE;
    }

    QStringList summaryLines;
    summaryLines << QString::fromUtf8(u8"经度：120.123456") << QString::fromUtf8(u8"纬度：30.654321");
    QList<std::pair<QString, QString>> attributes;
    attributes.append({QString::fromUtf8(u8"名称"), "Feature A"});
    attributes.append({QString::fromUtf8(u8"类型"), QString::fromUtf8(u8"道路")});
    dock.showPickDetails(summaryLines, attributes);

    QTableWidget *visibleTable = nullptr;
    const auto tables = dock.findChildren<QTableWidget *>();
    for (auto *table : tables) {
        if (!table->isHidden()) {
            visibleTable = table;
            break;
        }
    }
    if (!require(visibleTable != nullptr, "pick details should show a table")) {
        return EXIT_FAILURE;
    }
    if (!require(visibleTable->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff,
                 "short pick result tables should fit without inner scrolling")) {
        return EXIT_FAILURE;
    }
    if (!require(!dock.findChild<QDoubleSpinBox *>("modelScaleSpin")->isVisible(),
                 "pick details should hide stale model placement controls")) {
        return EXIT_FAILURE;
    }

    dock.clearLayerProperties();
    if (!require(!dock.findChild<QDoubleSpinBox *>("modelScaleSpin")->isVisible(),
                 "model placement controls should hide after clearing properties")) {
        return EXIT_FAILURE;
    }

    LayerTreeDock layerDock;
    auto *tree = layerDock.tree();
    if (!require(tree != nullptr, "layer tree widget should exist")) {
        return EXIT_FAILURE;
    }
    if (!require(tree->topLevelItemCount() == 1, "layer tree should show placeholder when empty")) {
        return EXIT_FAILURE;
    }
    if (!require(layerDock.currentLayerId().isEmpty(), "placeholder row should not expose a layer id")) {
        return EXIT_FAILURE;
    }

    layerDock.addLayer("measurement-1", "Measure 1", true, LayerKind::Measurement);
    if (!require(tree->topLevelItemCount() == 1, "adding first layer should replace placeholder")) {
        return EXIT_FAILURE;
    }
    layerDock.selectLayer("measurement-1");
    if (!require(layerDock.currentLayerId() == "measurement-1", "selectLayer should focus the real layer item")) {
        return EXIT_FAILURE;
    }

    layerDock.removeLayer("measurement-1");
    if (!require(tree->topLevelItemCount() == 1, "removing last layer should restore placeholder")) {
        return EXIT_FAILURE;
    }
    if (!require(layerDock.currentLayerId().isEmpty(), "placeholder should clear current layer id after removal")) {
        return EXIT_FAILURE;
    }

    QTemporaryDir tempDir;
    if (!require(tempDir.isValid(), "temporary directory should be created")) {
        return EXIT_FAILURE;
    }

    const QString vectorDirPath = tempDir.filePath("vector");
    if (!require(QDir().mkpath(vectorDirPath), "vector fixture directory should be created")) {
        return EXIT_FAILURE;
    }

    QFile vectorFile(vectorDirPath + "/roads.geojson");
    if (!require(vectorFile.open(QIODevice::WriteOnly | QIODevice::Truncate), "vector fixture should open for writing")) {
        return EXIT_FAILURE;
    }
    vectorFile.write(R"({"type":"FeatureCollection","features":[{"type":"Feature","properties":{"name":"Road A"},"geometry":{"type":"Point","coordinates":[120.1,30.2]}}]})");
    vectorFile.close();

    QFile layerConfigFile(tempDir.filePath("layers.json"));
    if (!require(layerConfigFile.open(QIODevice::WriteOnly | QIODevice::Truncate), "layers config should open for writing")) {
        return EXIT_FAILURE;
    }
    layerConfigFile.write(R"({
  "layers": [
    {
      "id": "persisted-vector-1",
      "name": "恢复图层",
      "path": "vector/roads.geojson",
      "kind": "vector",
      "autoload": true,
      "visible": false,
      "opacity": 0.25
    }
  ]
})");
    layerConfigFile.close();

    MainWindow persistedWindow;
    LayerManager persistedLayerManager;
    DataImporter importer;
    ApplicationController persistedController(
        persistedWindow,
        persistedWindow.globeWidget()->sceneController(),
        persistedLayerManager,
        importer);
    persistedController.loadBasemapAndLayers(tempDir.path().toStdString());

    if (!require(persistedLayerManager.layers().size() == 1, "persisted layer should be restored into layer manager")) {
        return EXIT_FAILURE;
    }
    const auto &persistedLayer = persistedLayerManager.layers().front();
    if (!require(persistedLayer->id() == "persisted-vector-1", "persisted layer id should be preserved")) {
        return EXIT_FAILURE;
    }
    if (!require(persistedLayer->name() == "恢复图层", "persisted layer name should be preserved")) {
        return EXIT_FAILURE;
    }
    if (!require(!persistedLayer->visible() && std::abs(persistedLayer->opacity() - 0.25) <= 1.0e-6,
                 "persisted layer visibility and opacity should be restored")) {
        return EXIT_FAILURE;
    }

    auto *persistedTree = persistedWindow.findChild<QTreeWidget *>();
    if (!require(persistedTree != nullptr && persistedTree->topLevelItemCount() == 1,
                 "persisted layer should be shown in layer tree")) {
        return EXIT_FAILURE;
    }
    if (!require(persistedTree->topLevelItem(0)->text(0) == QString::fromUtf8(u8"恢复图层"),
                 "layer tree should show persisted layer name")) {
        return EXIT_FAILURE;
    }

    window.setRecentFiles({
        "D:/data/a.tif",
        "D:/data/b.tif",
        "D:/data/a.tif",
        "D:/data/c.tif",
        "D:/data/d.tif",
        "D:/data/e.tif",
        "D:/data/f.tif",
    });
    QAction *recentMenuAction = nullptr;
    for (QAction *action : window.menuBar()->actions()) {
        if (action && action->menu() && action->text().contains(QString::fromUtf8(u8"文件"))) {
            for (QAction *child : action->menu()->actions()) {
                if (child && child->menu() && child->text().contains(QString::fromUtf8(u8"最近打开"))) {
                    recentMenuAction = child;
                    break;
                }
            }
        }
    }
    if (!require(recentMenuAction != nullptr && recentMenuAction->menu() != nullptr,
                 "recent files submenu should exist")) {
        return EXIT_FAILURE;
    }
    const auto recentActions = recentMenuAction->menu()->actions();
    if (!require(recentActions.size() == 5, "recent files should be capped at five unique entries")) {
        return EXIT_FAILURE;
    }
    if (!require(recentActions[0]->text().contains("D:/data/a.tif") &&
                     recentActions[1]->text().contains("D:/data/b.tif") &&
                     recentActions[4]->text().contains("D:/data/e.tif"),
                 "recent files order should stay stable after sanitizing duplicates")) {
        return EXIT_FAILURE;
    }

    MainWindow removeWindow;
    LayerManager removeLayerManager;
    DataImporter removeImporter;
    ApplicationController removeController(
        removeWindow,
        removeWindow.globeWidget()->sceneController(),
        removeLayerManager,
        removeImporter);

    auto removableLayer = std::make_shared<Layer>(
        "measurement-remove-1",
        QString::fromUtf8(u8"待删除量测").toStdString(),
        "memory://measurement-remove-1",
        LayerKind::Measurement);
    MeasurementLayerData removableMeasurement;
    removableMeasurement.kind = MeasurementKind::Distance;
    removableMeasurement.points.push_back({120.0, 30.0});
    removableMeasurement.points.push_back({121.0, 31.0});
    removableMeasurement.lengthMeters = 1200.0;
    removableLayer->setMeasurementData(removableMeasurement);
    if (!require(removeLayerManager.addLayer(removableLayer), "removable layer should be added into layer manager")) {
        return EXIT_FAILURE;
    }
    removeWindow.addLayerRow(*removableLayer);
    removeWindow.selectLayerRow(removableLayer->id());
    removeWindow.showLayerProperties(
        QString::fromStdString(removableLayer->id()),
        QString::fromStdString(removableLayer->name()),
        QString::fromUtf8(u8"量测"),
        QString::fromStdString(removableLayer->sourceUri()),
        true,
        1.0,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        removableMeasurement);
    auto *removeEditAction = removeWindow.findChild<QAction *>("editMeasureAction");
    if (!require(removeEditAction != nullptr && removeEditAction->isEnabled(),
                 "measurement edit action should enable before removing selected layer")) {
        return EXIT_FAILURE;
    }

    removeController.removeLayer(removableLayer->id());
    if (!require(removeLayerManager.layers().empty(), "layer manager should remove selected layer")) {
        return EXIT_FAILURE;
    }
    if (!require(removeWindow.currentLayerId().isEmpty(),
                 "current selected layer should clear after removing selected layer")) {
        return EXIT_FAILURE;
    }
    if (!require(!removeEditAction->isEnabled(),
                 "measurement edit action should reset after removing selected layer")) {
        return EXIT_FAILURE;
    }
    auto *removeTextEdit = removeWindow.findChild<QTextEdit *>();
    if (!require(removeTextEdit != nullptr &&
                     removeTextEdit->toPlainText().contains(QString::fromUtf8(u8"未选择图层")),
                 "property dock should return to unselected placeholder after removing selected layer")) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
