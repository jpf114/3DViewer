#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <type_traits>

#include <QAction>
#include <QApplication>
#include <QDoubleSpinBox>
#include <QKeyEvent>
#include <QTableWidget>
#include <QTextEdit>
#include <QTreeWidget>

#include "app/MainWindow.h"
#include "data/DataSourceDescriptor.h"
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

    return EXIT_SUCCESS;
}
