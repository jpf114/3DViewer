#pragma once

#include <QMetaType>
#include <QString>
#include <QWidget>

#include "layers/MeasurementLayerData.h"
#include "globe/PickResult.h"
#include "globe/SceneController.h"

class ToolManager;
class QTimer;

Q_DECLARE_METATYPE(MeasurementLayerData)

class GlobeWidget : public QWidget {
    Q_OBJECT

public:
    explicit GlobeWidget(QWidget *parent = nullptr);

    SceneController &sceneController();
    ToolManager &toolManager();
    PickResult pickAt(const QPointF &logicalPos) const;

signals:
    void cursorTextChanged(const QString &text);
    void terrainPickCompleted(const PickResult &result);
    void measurementTextChanged(const QString &text);
    void measurementStatusChanged(const QString &text);
    void measurementCommitted(const MeasurementLayerData &data);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void performPick(int x, int y);

    SceneController sceneController_;
    ToolManager *toolManager_;
    QTimer *frameTimer_;
    QTimer *pickTimer_;
    int lastPickX_ = 0;
    int lastPickY_ = 0;
};
