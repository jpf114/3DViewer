#pragma once

#include <vector>

#include "globe/MeasurementUtils.h"
#include "layers/MeasurementLayerData.h"
#include "tools/MapTool.h"

class MeasureAreaTool : public MapTool {
public:
    void mouseReleaseEvent(GlobeWidget &widget, QMouseEvent *event) override;
    void clear(GlobeWidget &widget) override;
    void undo(GlobeWidget &widget) override;
    void beginEditing(GlobeWidget &widget, const MeasurementLayerData &data);

private:
    void commit(GlobeWidget &widget);
    void publishMeasurement(GlobeWidget &widget) const;

    std::string editingLayerId_;
    std::vector<globe::MeasurementPoint> points_;
};
