#pragma once

#include <QMainWindow>

class GlobeWidget;
class LayerTreeDock;
class Layer;
class PropertyDock;
class StatusBarController;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    GlobeWidget *globeWidget() const;
    void addLayerRow(const Layer &layer);

signals:
    void importDataRequested(const QString &path);
    void layerVisibilityChanged(const QString &layerId, bool visible);

private:
    GlobeWidget *globeWidget_;
    LayerTreeDock *layerDock_;
    PropertyDock *propertyDock_;
    StatusBarController *statusController_;
};
