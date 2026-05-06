#pragma once

#include <memory>

class GlobeWidget;
class MapTool;
class QMouseEvent;

class ToolManager {
public:
    ToolManager();

    void mouseMoveEvent(GlobeWidget &widget, QMouseEvent *event);
    void mousePressEvent(GlobeWidget &widget, QMouseEvent *event);
    void mouseReleaseEvent(GlobeWidget &widget, QMouseEvent *event);

private:
    std::unique_ptr<MapTool> currentTool_;
};
