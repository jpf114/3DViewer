#include <QApplication>
#include <QString>

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <filesystem>
#endif

#include "app/ApplicationController.h"
#include "app/MainWindow.h"
#include "common/Logging.h"
#include "data/DataImporter.h"
#include "globe/GlobeWidget.h"
#include "globe/PickResult.h"
#include "layers/LayerManager.h"

#if defined(Q_OS_WIN)
static void setupWindowsRuntimeEnvironment() {
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("windows:dpiawareness=1"));
    }
    if (qEnvironmentVariableIsEmpty("FONTCONFIG_FILE")) {
        wchar_t module[MAX_PATH];
        const DWORD n = GetModuleFileNameW(nullptr, module, MAX_PATH);
        if (n > 0 && n < MAX_PATH) {
            std::filesystem::path conf = std::filesystem::path(module).parent_path() / "fonts.conf";
            if (std::filesystem::exists(conf)) {
                const std::wstring native = conf.native();
                qputenv("FONTCONFIG_FILE", QString::fromStdWString(native).toUtf8());
            }
        }
    }
}
#endif

int main(int argc, char *argv[]) {
#if defined(Q_OS_WIN)
    setupWindowsRuntimeEnvironment();
#endif
    QApplication app(argc, argv);
    qRegisterMetaType<PickResult>("PickResult");
    logging::initialize();

    MainWindow window;
    LayerManager layerManager;
    DataImporter importer;
    ApplicationController controller(window, window.globeWidget()->sceneController(), layerManager, importer);
    Q_UNUSED(controller);
    window.show();

    return app.exec();
}
