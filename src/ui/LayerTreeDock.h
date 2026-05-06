#pragma once

#include <QDockWidget>

class QTreeWidget;

class LayerTreeDock : public QDockWidget {
    Q_OBJECT

public:
    explicit LayerTreeDock(QWidget *parent = nullptr);
    void addLayer(const std::string &id, const std::string &name, bool visible);
    QTreeWidget *tree() const;

signals:
    void layerSelected(const QString &layerId);
    void layerVisibilityChanged(const QString &layerId, bool visible);
    void layerOrderChanged(const QStringList &orderedLayerIds);

private:
    void emitLayerOrderFromTree();

    QTreeWidget *tree_;
};
