#include "tools/MeasureTool.h"

#include <QMouseEvent>
#include <QString>
#include <QStringLiteral>
#include <QStringList>

using namespace Qt::Literals::StringLiterals;

#include "globe/GlobeWidget.h"
#include "globe/PickResult.h"

namespace {

QString formatDistance(double meters) {
    if (meters >= 1000.0) {
        return QString(u"%1 km"_s).arg(meters / 1000.0, 0, 'f', 3);
    }
    return QString(u"%1 m"_s).arg(meters, 0, 'f', 1);
}

QString buildMeasurementText(const std::vector<globe::MeasurementPoint> &points) {
    if (points.empty()) {
        return u"测距：未开始。\n操作：左键添加点，右键或工具栏清空。"_s;
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

    lines.append(u"操作：左键继续，右键或工具栏清空。"_s);
    return lines.join('\n');
}

} // namespace

void MeasureTool::mouseReleaseEvent(GlobeWidget &widget, QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        clear(widget);
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
    emit widget.measurementTextChanged(buildMeasurementText(points_));
    emit widget.measurementStatusChanged(u"测距：未开始"_s);
}

void MeasureTool::publishMeasurement(GlobeWidget &widget) const {
    emit widget.measurementTextChanged(buildMeasurementText(points_));
    emit widget.measurementStatusChanged(
        QString(u"测距总长：%1"_s).arg(formatDistance(globe::polylineLengthMeters(points_))));
}
