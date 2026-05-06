#pragma once

#include <QOpenGLWidget>

#include "globe/SceneController.h"

class ToolManager;
class QTimer;

class GlobeWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit GlobeWidget(QWidget *parent = nullptr);

    SceneController &sceneController();
    ToolManager &toolManager();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    float flipY(float y) const;

    SceneController sceneController_;
    ToolManager *toolManager_;
    QTimer *frameTimer_;
};
