#include "tools/MeasureAreaTool.h"

#include <QMouseEvent>
#include <QString>
#include <QStringList>

using namespace Qt::Literals::StringLiterals;

#include "globe/GlobeWidget.h"
#include "globe/PickResult.h"
#include "layers/MeasurementLayerData.h"

namespace {

QString formatDistance(double meters) {
    if (meters >= 1000.0) {
        return QString(u"%1 km"_s).arg(meters / 1000.0, 0, 'f', 3);
    }
    return QString(u"%1 m"_s).arg(meters, 0, 'f', 1);
}

QString formatArea(double squareMeters) {
    if (squareMeters >= 1000000.0) {
        return QString(u"%1 km²"_s).arg(squareMeters / 1000000.0, 0, 'f', 3);
    }
    return QString(u"%1 m²"_s).arg(squareMeters, 0, 'f', 1);
}

QString buildMeasurementText(const std::vector<globe::MeasurementPoint> &points) {
    if (points.empty()) {
        return u"测面：未开始。\n操作：左键添加点，右键结束并保留，Backspace 撤销最后一点，Esc 或工具栏清空草稿。"_s;
    }

    QStringList lines;
    lines.append(QString(u"测面点数：%1"_s).arg(points.size()));
    lines.append(QString(u"周长：%1"_s).arg(formatDistance(globe::polylineLengthMeters(points))));

    if (points.size() >= 3) {
        lines.append(QString(u"近似面积：%1"_s).arg(formatArea(globe::polygonAreaSquareMeters(points))));
    } else {
        lines.append(u"近似面积：至少需要 3 个点。"_s);
    }

    const auto &lastPoint = points.back();
    lines.append(QString(u"当前点经度：%1"_s).arg(lastPoint.longitude, 0, 'f', 6));
    lines.append(QString(u"当前点纬度：%1"_s).arg(lastPoint.latitude, 0, 'f', 6));
    lines.append(u"操作：左键继续，右键结束并保留，Backspace 撤销最后一点，Esc 或工具栏清空草稿。"_s);
    return lines.join('\n');
}

} // namespace

void MeasureAreaTool::mouseReleaseEvent(GlobeWidget &widget, QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        commit(widget);
        return;
    }

    if (event->button() != Qt::LeftButton) {
        return;
    }

    const PickResult pick = widget.pickAt(event->position());
    if (!pick.hit) {
        emit widget.measurementTextChanged(u"测面：当前点击位置无可用地形点。"_s);
        emit widget.measurementStatusChanged(u"测面：取点失败"_s);
        return;
    }

    points_.push_back({pick.longitude, pick.latitude});
    publishMeasurement(widget);
}

void MeasureAreaTool::clear(GlobeWidget &widget) {
    editingLayerId_.clear();
    points_.clear();
    widget.sceneController().clearMeasurementDraft();
    emit widget.measurementTextChanged(buildMeasurementText(points_));
    emit widget.measurementStatusChanged(u"测面：未开始"_s);
}

void MeasureAreaTool::undo(GlobeWidget &widget) {
    if (points_.empty()) {
        clear(widget);
        return;
    }

    points_.pop_back();
    if (points_.empty()) {
        clear(widget);
        return;
    }

    publishMeasurement(widget);
}

void MeasureAreaTool::beginEditing(GlobeWidget &widget, const MeasurementLayerData &data) {
    editingLayerId_ = data.targetLayerId;
    points_ = data.points;
    if (points_.empty()) {
        clear(widget);
        return;
    }

    publishMeasurement(widget);
    emit widget.measurementStatusChanged(u"测面：正在编辑已有结果"_s);
}

void MeasureAreaTool::commit(GlobeWidget &widget) {
    if (points_.size() < 3) {
        clear(widget);
        return;
    }

    MeasurementLayerData data;
    data.kind = MeasurementKind::Area;
    data.targetLayerId = editingLayerId_;
    data.points = points_;
    data.lengthMeters = globe::polylineLengthMeters(points_);
    data.areaSquareMeters = globe::polygonAreaSquareMeters(points_);
    emit widget.measurementCommitted(data);

    editingLayerId_.clear();
    points_.clear();
    widget.sceneController().clearMeasurementDraft();
    emit widget.measurementTextChanged(u"测面：结果已保留，可继续开始下一条。"_s);
    emit widget.measurementStatusChanged(u"测面：已保存结果"_s);
}

void MeasureAreaTool::publishMeasurement(GlobeWidget &widget) const {
    MeasurementLayerData draft;
    draft.kind = MeasurementKind::Area;
    draft.targetLayerId = editingLayerId_;
    draft.points = points_;
    draft.lengthMeters = globe::polylineLengthMeters(points_);
    draft.areaSquareMeters = globe::polygonAreaSquareMeters(points_);
    widget.sceneController().setMeasurementDraft(draft);
    emit widget.measurementTextChanged(buildMeasurementText(points_));
    if (points_.size() >= 3) {
        emit widget.measurementStatusChanged(
            QString(u"测面面积：%1"_s).arg(formatArea(globe::polygonAreaSquareMeters(points_))));
        return;
    }

    emit widget.measurementStatusChanged(
        QString(u"测面周长：%1"_s).arg(formatDistance(globe::polylineLengthMeters(points_))));
}
