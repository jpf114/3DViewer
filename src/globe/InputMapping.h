#pragma once

#include <QPointF>
#include <QSize>
#include <Qt>

#include "tools/ToolManager.h"

namespace globe {

inline QPointF mapToScenePoint(const QPointF &logicalPos,
                               const QSize &logicalSize,
                               const QSize &pixelSize) {
    const double logicalWidth = logicalSize.width() > 0 ? static_cast<double>(logicalSize.width()) : 1.0;
    const double logicalHeight = logicalSize.height() > 0 ? static_cast<double>(logicalSize.height()) : 1.0;
    const double scaleX = pixelSize.width() > 0 ? static_cast<double>(pixelSize.width()) / logicalWidth : 1.0;
    const double scaleY = pixelSize.height() > 0 ? static_cast<double>(pixelSize.height()) / logicalHeight : 1.0;

    const double sceneX = logicalPos.x() * scaleX;
    const double sceneY = (logicalHeight - logicalPos.y()) * scaleY;
    return QPointF(sceneX, sceneY);
}

inline bool shouldScheduleHoverPick(Qt::MouseButtons buttons, ToolId activeToolId) {
    if (buttons != Qt::NoButton) {
        return false;
    }

    return activeToolId != ToolId::Pan;
}

} // namespace globe
