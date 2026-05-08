#include <QApplication>
#include <QFile>
#include <QString>
#include <stdexcept>
#include <string>

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <eh.h>
#include <filesystem>
#endif

#include "app/ApplicationController.h"
#include "app/MainWindow.h"
#include "common/Logging.h"
#include "data/DataImporter.h"
#include "globe/GlobeWidget.h"
#include "globe/PickResult.h"
#include "layers/LayerManager.h"
#include "ui/IconManager.h"

#if defined(Q_OS_WIN)
static void seTranslator(unsigned int code, EXCEPTION_POINTERS * /*ep*/) {
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
        throw std::runtime_error("Access Violation (SEH)");
    case EXCEPTION_STACK_OVERFLOW:
        throw std::runtime_error("Stack Overflow (SEH)");
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        throw std::runtime_error("Divide by Zero (SEH)");
    default:
        throw std::runtime_error("Structured Exception: " + std::to_string(code));
    }
}

static void setupWindowsRuntimeEnvironment() {
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("windows"));
    }
    if (qEnvironmentVariableIsEmpty("FONTCONFIG_FILE")) {
        wchar_t module[MAX_PATH];
        const DWORD n = GetModuleFileNameW(nullptr, module, MAX_PATH);
        if (n > 0 && n < MAX_PATH) {
            std::filesystem::path exeDir = std::filesystem::path(module).parent_path();
            std::filesystem::path conf = exeDir / "fonts.conf";
            if (std::filesystem::exists(conf)) {
                const std::wstring native = conf.native();
                qputenv("FONTCONFIG_FILE", QString::fromStdWString(native).toUtf8());
            }
            std::filesystem::path dataDir = exeDir / "data";
            if (std::filesystem::exists(dataDir)) {
                const std::wstring native = dataDir.native();
                qputenv("THREEDVIEWER_RESOURCE_DIR", QString::fromStdWString(native).toUtf8());
            }
            std::filesystem::path iconsDir = exeDir / "icons";
            if (std::filesystem::exists(iconsDir)) {
                IconManager::instance().setIconsBasePath(
                    QString::fromStdWString(iconsDir.native()));
            }
        }
    }
}
#endif

static void loadStyleSheet(QApplication &app) {
    QString qssPath;
#if defined(Q_OS_WIN)
    wchar_t module[MAX_PATH];
    const DWORD n = GetModuleFileNameW(nullptr, module, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        std::filesystem::path p = std::filesystem::path(module).parent_path() / "style.qss";
        qssPath = QString::fromStdWString(p.native());
    }
#endif
    if (qssPath.isEmpty()) {
        qssPath = QStringLiteral("style.qss");
    }
    QFile qss(qssPath);
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));
    }
}

int main(int argc, char *argv[]) {
#if defined(Q_OS_WIN)
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    _set_se_translator(seTranslator);
    setupWindowsRuntimeEnvironment();
#endif
    QApplication app(argc, argv);
    qRegisterMetaType<PickResult>("PickResult");
    logging::initialize();
    loadStyleSheet(app);

    MainWindow window;
    LayerManager layerManager;
    DataImporter importer;
    ApplicationController controller(window, window.globeWidget()->sceneController(), layerManager, importer);
    Q_UNUSED(controller);
    window.show();

    return app.exec();
}
