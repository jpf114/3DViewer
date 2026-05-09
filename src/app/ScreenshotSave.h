#pragma once

#include <QString>

namespace screenshot {

QString defaultSavePath();
QString normalizeSavePath(const QString &path, const QString &selectedFilter);

} // namespace screenshot
