#pragma once

#include <QString>
#include <QWidget>

#include "globe/PickResult.h"
#include "globe/SceneController.h"

class ToolManager;
class QTimer;

class GlobeWidget : public QWidget {
    Q_OBJECT

public:
    explicit GlobeWidget(QWidget *parent = nullptr);

    SceneController &sceneController();
    ToolManager &toolManager();

signals:
    void cursorTextChanged(const QString &text);
    void terrainPickCompleted(const PickResult &result);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    float flipY(float y) const;

    SceneController sceneController_;
    ToolManager *toolManager_;
    QTimer *frameTimer_;
};
