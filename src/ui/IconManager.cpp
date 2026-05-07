#include "ui/IconManager.h"

#include <QFile>
#include <QPainter>
#include <QSvgRenderer>
#include <QStringLiteral>

using namespace Qt::Literals::StringLiterals;

IconManager &IconManager::instance() {
    static IconManager mgr;
    return mgr;
}

void IconManager::setIconsBasePath(const QString &path) {
    basePath_ = path;
    cache_.clear();
}

QIcon IconManager::icon(const QString &name, int size, const QColor &color) {
    const std::string key = QString("%1_%2_%3").arg(name).arg(size).arg(color.name()).toStdString();
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return QIcon(it->second);
    }

    const QString filePath = basePath_.isEmpty()
        ? QString(u"icons/regular/%1"_s).arg(name)
        : QString(u"%1/regular/%2"_s).arg(basePath_, name);

    QPixmap pm = renderSvg(filePath, size, color);
    if (pm.isNull()) {
        return QIcon();
    }

    cache_[key] = pm;
    return QIcon(pm);
}

QPixmap IconManager::renderSvg(const QString &filePath, int size, const QColor &color) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QPixmap();
    }

    QString svgContent = QString::fromUtf8(file.readAll());
    file.close();

    svgContent.replace(u"currentColor"_s, color.name());

    QSvgRenderer renderer(svgContent.toUtf8());
    if (!renderer.isValid()) {
        return QPixmap();
    }

    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();

    return pixmap;
}
