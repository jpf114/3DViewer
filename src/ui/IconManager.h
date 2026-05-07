#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>
#include <map>
#include <string>

class IconManager {
public:
    static IconManager &instance();

    void setIconsBasePath(const QString &path);
    QIcon icon(const QString &name, int size = 20, const QColor &color = QColor("#4F637A"));

private:
    IconManager() = default;

    QString basePath_;
    std::map<std::string, QPixmap> cache_;

    QPixmap renderSvg(const QString &filePath, int size, const QColor &color);
};
