#pragma once

#include <QMouseEvent>

class GlobeWidget;

class MapTool {
public:
    virtual ~MapTool() = default;
    virtual void mouseMoveEvent(GlobeWidget &, QMouseEvent *) {}
    virtual void mousePressEvent(GlobeWidget &, QMouseEvent *) {}
    virtual void mouseReleaseEvent(GlobeWidget &, QMouseEvent *) {}
};
