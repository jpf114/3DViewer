#include <cstdlib>
#include <cstdio>

#include <QFileInfo>

#include "app/ScreenshotSave.h"

namespace {

bool require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        return false;
    }
    return true;
}

} // namespace

int main() {
    const QString defaultPath = screenshot::defaultSavePath();
    const QFileInfo defaultInfo(defaultPath);
    if (!require(defaultInfo.fileName().startsWith("3DViewer_screenshot_"),
                 "default screenshot name should contain timestamp prefix")) {
        return EXIT_FAILURE;
    }
    if (!require(defaultInfo.suffix().compare("png", Qt::CaseInsensitive) == 0,
                 "default screenshot suffix should be png")) {
        return EXIT_FAILURE;
    }

    if (!require(screenshot::normalizeSavePath("D:/shots/demo", QString::fromUtf8(u8"PNG 图像 (*.png)")) ==
                     "D:/shots/demo.png",
                 "png filter should append .png when suffix is missing")) {
        return EXIT_FAILURE;
    }
    if (!require(screenshot::normalizeSavePath("D:/shots/demo", QString::fromUtf8(u8"JPEG 图像 (*.jpg)")) ==
                     "D:/shots/demo.jpg",
                 "jpeg filter should append .jpg when suffix is missing")) {
        return EXIT_FAILURE;
    }
    if (!require(screenshot::normalizeSavePath("D:/shots/demo", QString::fromUtf8(u8"BMP 图像 (*.bmp)")) ==
                     "D:/shots/demo.bmp",
                 "bmp filter should append .bmp when suffix is missing")) {
        return EXIT_FAILURE;
    }
    if (!require(screenshot::normalizeSavePath("D:/shots/demo.png", QString::fromUtf8(u8"JPEG 图像 (*.jpg)")) ==
                     "D:/shots/demo.png",
                 "existing suffix should be preserved")) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
