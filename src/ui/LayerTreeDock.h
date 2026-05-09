#pragma once

#include <QDockWidget>

class QTreeWidget;
class QTreeWidgetItem;

enum class LayerKind;

class LayerTreeDock : public QDockWidget {
    Q_OBJECT

public:
    explicit LayerTreeDock(QWidget *parent = nullptr);
    void addLayer(const std::string &id, const std::string &name, bool visible, LayerKind kind);
    void removeLayer(const std::string &id);
    void selectLayer(const std::string &id);
    void clearSelection();
    QString currentLayerId() const;
    QTreeWidget *tree() const;

signals:
    void layerSelected(const QString &layerId);
    void layerVisibilityChanged(const QString &layerId, bool visible);
    void layerOrderChanged(const QStringList &orderedLayerIds);
    void removeLayerRequested(const QString &layerId);
    void zoomToLayerRequested(const QString &layerId);
    void editMeasurementRequested(const QString &layerId);

private:
    void emitLayerOrderFromTree();
    void ensurePlaceholderItem();
    void clearPlaceholderItem();
    bool isPlaceholderItem(QTreeWidgetItem *item) const;

    QTreeWidget *tree_;
};
