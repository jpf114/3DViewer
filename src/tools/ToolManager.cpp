#include "tools/MeasureAreaTool.h"
#include "tools/MeasureTool.h"
#include "tools/ToolManager.h"

#include "tools/PanTool.h"
#include "tools/PickTool.h"

ToolManager::ToolManager(QObject *parent)
    : parent_(parent), currentTool_(std::make_unique<PanTool>()) {}

void ToolManager::setActiveTool(ToolId toolId) {
    if (toolId == currentToolId_) {
        return;
    }

    currentToolId_ = toolId;
    switch (toolId) {
    case ToolId::MeasureArea:
        currentTool_ = std::make_unique<MeasureAreaTool>();
        break;
    case ToolId::Measure:
        currentTool_ = std::make_unique<MeasureTool>();
        break;
    case ToolId::Pick:
        currentTool_ = std::make_unique<PickTool>();
        break;
    case ToolId::Pan:
    default:
        currentTool_ = std::make_unique<PanTool>();
        break;
    }
}

ToolId ToolManager::activeToolId() const {
    return currentToolId_;
}

void ToolManager::mouseMoveEvent(GlobeWidget &widget, QMouseEvent *event) {
    currentTool_->mouseMoveEvent(widget, event);
}

void ToolManager::mousePressEvent(GlobeWidget &widget, QMouseEvent *event) {
    currentTool_->mousePressEvent(widget, event);
}

void ToolManager::mouseReleaseEvent(GlobeWidget &widget, QMouseEvent *event) {
    currentTool_->mouseReleaseEvent(widget, event);
}

void ToolManager::startMeasurementEditing(GlobeWidget &widget, const MeasurementLayerData &data) {
    const ToolId targetTool = data.kind == MeasurementKind::Area ? ToolId::MeasureArea : ToolId::Measure;
    setActiveTool(targetTool);

    if (targetTool == ToolId::Measure) {
        if (auto *tool = dynamic_cast<MeasureTool *>(currentTool_.get())) {
            tool->beginEditing(widget, data);
        }
        return;
    }

    if (auto *tool = dynamic_cast<MeasureAreaTool *>(currentTool_.get())) {
        tool->beginEditing(widget, data);
    }
}

void ToolManager::clearActiveToolState(GlobeWidget &widget) {
    currentTool_->clear(widget);
}

void ToolManager::undoActiveToolState(GlobeWidget &widget) {
    currentTool_->undo(widget);
}
