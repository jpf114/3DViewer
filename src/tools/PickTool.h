#pragma once

#include "tools/MapTool.h"

class PickTool : public MapTool {
public:
    void mouseReleaseEvent(GlobeWidget &widget, QMouseEvent *event) override;
};
