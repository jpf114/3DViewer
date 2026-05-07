#pragma once

#include <QStringList>
#include <QMainWindow>

class GlobeWidget;
class LayerTreeDock;
class Layer;
class PropertyDock;
class StatusBarController;
class QAction;
class QActionGroup;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    GlobeWidget *globeWidget() const;
    void addLayerRow(const Layer &layer);
    void removeLayerRow(const std::string &layerId);
    void showLayerDetails(const QString &text);
    void showLayerProperties(const QString &layerId, const QString &infoText, double opacity, int bandCount = 0);
    void clearLayerProperties();

signals:
    void importDataRequested(const QString &path);
    void layerSelected(const QString &layerId);
    void layerVisibilityChanged(const QString &layerId, bool visible);
    void layerOrderChanged(const QStringList &orderedLayerIds);
    void removeLayerRequested(const QString &layerId);
    void layerOpacityChanged(const QString &layerId, double opacity);
    void bandMappingChanged(const QString &layerId, int red, int green, int blue);
    void toolChanged(int toolId);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    GlobeWidget *globeWidget_;
    LayerTreeDock *layerDock_;
    PropertyDock *propertyDock_;
    StatusBarController *statusController_;
    QAction *panAction_;
    QAction *pickAction_;
    QActionGroup *toolGroup_;
};
