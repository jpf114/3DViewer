#pragma once

#include <memory>

class GlobeWidget;
class MapTool;
class QMouseEvent;
class QObject;

class ToolManager {
public:
    explicit ToolManager(QObject *parent = nullptr);

    void mouseMoveEvent(GlobeWidget &widget, QMouseEvent *event);
    void mousePressEvent(GlobeWidget &widget, QMouseEvent *event);
    void mouseReleaseEvent(GlobeWidget &widget, QMouseEvent *event);

private:
    QObject *parent_;
    std::unique_ptr<MapTool> currentTool_;
};
