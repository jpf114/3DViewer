#include "tools/PickTool.h"

#include <QMouseEvent>

#include "globe/GlobeWidget.h"
#include "globe/PickResult.h"
#include "globe/SceneController.h"

void PickTool::mouseReleaseEvent(GlobeWidget &widget, QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        return;
    }

    const PickResult pick = widget.pickAt(event->position());
    emit widget.terrainPickCompleted(pick);
}
