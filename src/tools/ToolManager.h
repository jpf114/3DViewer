#pragma once

#include <memory>
#include <string>

class GlobeWidget;
class MapTool;
class QMouseEvent;
class QObject;

enum class ToolId {
    Pan,
    Pick,
    Measure
};

class ToolManager {
public:
    explicit ToolManager(QObject *parent = nullptr);

    void setActiveTool(ToolId toolId);
    ToolId activeToolId() const;

    void mouseMoveEvent(GlobeWidget &widget, QMouseEvent *event);
    void mousePressEvent(GlobeWidget &widget, QMouseEvent *event);
    void mouseReleaseEvent(GlobeWidget &widget, QMouseEvent *event);
    void clearActiveToolState(GlobeWidget &widget);

private:
    QObject *parent_;
    std::unique_ptr<MapTool> currentTool_;
    ToolId currentToolId_ = ToolId::Pan;
};
