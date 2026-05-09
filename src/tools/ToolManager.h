#pragma once

#include <memory>
#include <string>

#include "layers/MeasurementLayerData.h"

class GlobeWidget;
class MapTool;
class QMouseEvent;
class QObject;

enum class ToolId {
    Pan,
    Pick,
    Measure,
    MeasureArea
};

class ToolManager {
public:
    explicit ToolManager(QObject *parent = nullptr);

    void setActiveTool(ToolId toolId);
    ToolId activeToolId() const;

    void mouseMoveEvent(GlobeWidget &widget, QMouseEvent *event);
    void mousePressEvent(GlobeWidget &widget, QMouseEvent *event);
    void mouseReleaseEvent(GlobeWidget &widget, QMouseEvent *event);
    void startMeasurementEditing(GlobeWidget &widget, const MeasurementLayerData &data);
    void clearActiveToolState(GlobeWidget &widget);
    void undoActiveToolState(GlobeWidget &widget);

private:
    QObject *parent_;
    std::unique_ptr<MapTool> currentTool_;
    ToolId currentToolId_ = ToolId::Pan;
};
