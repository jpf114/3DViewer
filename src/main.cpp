#include <QApplication>

#include "app/ApplicationController.h"
#include "app/MainWindow.h"
#include "common/Logging.h"
#include "data/DataImporter.h"
#include "globe/GlobeWidget.h"
#include "layers/LayerManager.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    logging::initialize();

    MainWindow window;
    LayerManager layerManager;
    DataImporter importer;
    ApplicationController controller(window, window.globeWidget()->sceneController(), layerManager, importer);
    Q_UNUSED(controller);
    window.show();

    return app.exec();
}
