#include "app/ScreenshotSave.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

namespace {

QString extensionForFilter(const QString &selectedFilter) {
    if (selectedFilter.contains("*.jpg", Qt::CaseInsensitive) ||
        selectedFilter.contains("*.jpeg", Qt::CaseInsensitive)) {
        return ".jpg";
    }
    if (selectedFilter.contains("*.bmp", Qt::CaseInsensitive)) {
        return ".bmp";
    }
    return ".png";
}

} // namespace

namespace screenshot {

QString defaultSavePath() {
    const QString filename = QString("3DViewer_screenshot_%1.png")
                                 .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    return QDir::home().filePath(filename);
}

QString normalizeSavePath(const QString &path, const QString &selectedFilter) {
    if (path.isEmpty()) {
        return path;
    }

    const QFileInfo fileInfo(path);
    if (!fileInfo.suffix().isEmpty()) {
        return path;
    }

    return path + extensionForFilter(selectedFilter);
}

} // namespace screenshot
