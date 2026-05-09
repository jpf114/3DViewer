#include "tools/MeasureTool.h"

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

QString buildMeasurementText(const std::vector<globe::MeasurementPoint> &points) {
    if (points.empty()) {
        return u"测距：未开始。\n操作：左键添加点，右键结束并保留，Esc 或工具栏清空草稿。"_s;
    }

    QStringList lines;
    lines.append(QString(u"测距点数：%1"_s).arg(points.size()));
    lines.append(QString(u"总长度：%1"_s).arg(formatDistance(globe::polylineLengthMeters(points))));

    const auto &lastPoint = points.back();
    lines.append(QString(u"当前点经度：%1"_s).arg(lastPoint.longitude, 0, 'f', 6));
    lines.append(QString(u"当前点纬度：%1"_s).arg(lastPoint.latitude, 0, 'f', 6));

    if (points.size() >= 2) {
        const double segment = globe::greatCircleDistanceMeters(points[points.size() - 2], points.back());
        lines.append(QString(u"最近一段：%1"_s).arg(formatDistance(segment)));
    }

    lines.append(u"操作：左键继续，右键结束并保留，Esc 或工具栏清空草稿。"_s);
    return lines.join('\n');
}

} // namespace

void MeasureTool::mouseReleaseEvent(GlobeWidget &widget, QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        commit(widget);
        return;
    }

    if (event->button() != Qt::LeftButton) {
        return;
    }

    const PickResult pick = widget.pickAt(event->position());
    if (!pick.hit) {
        emit widget.measurementTextChanged(u"测距：当前点击位置无可用地形点。"_s);
        emit widget.measurementStatusChanged(u"测距：取点失败"_s);
        return;
    }

    points_.push_back({pick.longitude, pick.latitude});
    publishMeasurement(widget);
}

void MeasureTool::clear(GlobeWidget &widget) {
    points_.clear();
    widget.sceneController().clearMeasurementDraft();
    emit widget.measurementTextChanged(buildMeasurementText(points_));
    emit widget.measurementStatusChanged(u"测距：未开始"_s);
}

void MeasureTool::commit(GlobeWidget &widget) {
    if (points_.size() < 2) {
        clear(widget);
        return;
    }

    MeasurementLayerData data;
    data.kind = MeasurementKind::Distance;
    data.points = points_;
    data.lengthMeters = globe::polylineLengthMeters(points_);
    emit widget.measurementCommitted(data);

    points_.clear();
    widget.sceneController().clearMeasurementDraft();
    emit widget.measurementTextChanged(u"测距：结果已保留，可继续开始下一条。"_s);
    emit widget.measurementStatusChanged(u"测距：已保存结果"_s);
}

void MeasureTool::publishMeasurement(GlobeWidget &widget) const {
    MeasurementLayerData draft;
    draft.kind = MeasurementKind::Distance;
    draft.points = points_;
    draft.lengthMeters = globe::polylineLengthMeters(points_);
    widget.sceneController().setMeasurementDraft(draft);
    emit widget.measurementTextChanged(buildMeasurementText(points_));
    emit widget.measurementStatusChanged(
        QString(u"测距总长：%1"_s).arg(formatDistance(globe::polylineLengthMeters(points_))));
}
