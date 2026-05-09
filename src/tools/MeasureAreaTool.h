#pragma once

#include <vector>

#include "globe/MeasurementUtils.h"
#include "tools/MapTool.h"

class MeasureAreaTool : public MapTool {
public:
    void mouseReleaseEvent(GlobeWidget &widget, QMouseEvent *event) override;
    void clear(GlobeWidget &widget) override;

private:
    void commit(GlobeWidget &widget);
    void publishMeasurement(GlobeWidget &widget) const;

    std::vector<globe::MeasurementPoint> points_;
};
