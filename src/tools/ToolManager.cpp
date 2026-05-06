#include "tools/ToolManager.h"

#include "tools/PanTool.h"

ToolManager::ToolManager(QObject *parent)
    : parent_(parent), currentTool_(std::make_unique<PanTool>()) {}

void ToolManager::mouseMoveEvent(GlobeWidget &widget, QMouseEvent *event) {
    currentTool_->mouseMoveEvent(widget, event);
}

void ToolManager::mousePressEvent(GlobeWidget &widget, QMouseEvent *event) {
    currentTool_->mousePressEvent(widget, event);
}

void ToolManager::mouseReleaseEvent(GlobeWidget &widget, QMouseEvent *event) {
    currentTool_->mouseReleaseEvent(widget, event);
}
