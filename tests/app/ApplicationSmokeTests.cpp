#include <cstdlib>
#include <cmath>
#include <type_traits>

#include <QApplication>
#include <QAction>
#include <QDoubleSpinBox>
#include <QKeyEvent>
#include <QString>
#include <QTextEdit>

#include "app/MainWindow.h"
#include "data/DataSourceDescriptor.h"
#include "ui/PropertyDock.h"

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
    if (!undoRequested) {
        return EXIT_FAILURE;
    }

    bool editRequested = false;
    QObject::connect(&window, &MainWindow::editSelectedMeasurementRequested, [&editRequested]() {
        editRequested = true;
    });
    auto *editAction = window.findChild<QAction *>("editMeasureAction");
    if (!editAction) {
        return EXIT_FAILURE;
    }
    editAction->trigger();
    if (!editRequested) {
        return EXIT_FAILURE;
    }

    PropertyDock dock;
    auto *textEdit = dock.findChild<QTextEdit *>();
    if (!textEdit || textEdit->maximumHeight() == 64) {
        return EXIT_FAILURE;
    }

    if (textEdit->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff ||
        textEdit->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff) {
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
                             "三维模型",
                             "D:/demo/building.glb",
                             true,
                             1.0,
                             std::nullopt,
                             std::nullopt,
                             placement);

    auto *longitudeSpin = dock.findChild<QDoubleSpinBox *>("modelLongitudeSpin");
    auto *scaleSpin = dock.findChild<QDoubleSpinBox *>("modelScaleSpin");
    if (!longitudeSpin || !scaleSpin) {
        return EXIT_FAILURE;
    }

    if (std::abs(longitudeSpin->value() - placement.longitude) > 1.0e-6 ||
        std::abs(scaleSpin->value() - placement.scale) > 1.0e-6) {
        return EXIT_FAILURE;
    }

    scaleSpin->setValue(3.5);
    if (!signalReceived || emittedLayerId != "model-1") {
        return EXIT_FAILURE;
    }

    if (std::abs(emittedPlacement.scale - 3.5) > 1.0e-6 ||
        std::abs(emittedPlacement.longitude - placement.longitude) > 1.0e-6) {
        return EXIT_FAILURE;
    }

    dock.clearLayerProperties();
    if (dock.findChild<QDoubleSpinBox *>("modelScaleSpin")->isVisible()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
