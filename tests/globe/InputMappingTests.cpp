#include <cstdlib>

#include <QPointF>
#include <QSize>
#include <Qt>

#include "globe/InputMapping.h"

int main() {
    const QPointF mapped = globe::mapToScenePoint(QPointF(25.0, 10.0), QSize(100, 80), QSize(200, 160));
    if (mapped.x() != 50.0 || mapped.y() != 140.0) {
        return EXIT_FAILURE;
    }

    if (globe::shouldScheduleHoverPick(Qt::LeftButton, ToolId::Pick)) {
        return EXIT_FAILURE;
    }

    if (!globe::shouldScheduleHoverPick(Qt::NoButton, ToolId::Pick)) {
        return EXIT_FAILURE;
    }

    if (globe::shouldScheduleHoverPick(Qt::NoButton, ToolId::Pan)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
