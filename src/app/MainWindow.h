#pragma once

#include <QStringList>
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
    void showLayerDetails(const QString &text);

signals:
    void importDataRequested(const QString &path);
    void layerSelected(const QString &layerId);
    void layerVisibilityChanged(const QString &layerId, bool visible);
    void layerOrderChanged(const QStringList &orderedLayerIds);

private:
    GlobeWidget *globeWidget_;
    LayerTreeDock *layerDock_;
    PropertyDock *propertyDock_;
    StatusBarController *statusController_;
};
