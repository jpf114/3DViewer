#include "tools/PickTool.h"

#include <QMouseEvent>

#include "globe/GlobeWidget.h"
#include "globe/PickResult.h"
#include "globe/SceneController.h"

void PickTool::mouseReleaseEvent(GlobeWidget &widget, QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        return;
    }

    const float fx = static_cast<float>(event->position().x());
    const float fy = static_cast<float>(event->position().y());
    const PickResult pick = widget.sceneController().pickAt(static_cast<int>(fx), static_cast<int>(fy));
    emit widget.terrainPickCompleted(pick);
}
