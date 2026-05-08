#pragma once

#include <vector>

#include "globe/MeasurementUtils.h"
#include "tools/MapTool.h"

class MeasureTool : public MapTool {
public:
    void mouseReleaseEvent(GlobeWidget &widget, QMouseEvent *event) override;
    void clear(GlobeWidget &widget) override;

private:
    void publishMeasurement(GlobeWidget &widget) const;

    std::vector<globe::MeasurementPoint> points_;
};
