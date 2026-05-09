#pragma once

#include <QStringList>
#include <QMainWindow>
#include <QList>
#include <optional>
#include <utility>

#include "data/DataSourceDescriptor.h"
#include "layers/MeasurementLayerData.h"

class QCloseEvent;
struct RasterMetadata;
struct VectorLayerInfo;

class GlobeWidget;
class LayerTreeDock;
class Layer;
class PropertyDock;
class StatusBarController;
class QAction;
class QActionGroup;
class QMenu;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    GlobeWidget *globeWidget() const;
    void addLayerRow(const Layer &layer);
    void removeLayerRow(const std::string &layerId);
    void selectLayerRow(const std::string &layerId);
    void showLayerDetails(const QString &text);
    void showPickDetails(const QStringList &summaryLines,
                         const QList<std::pair<QString, QString>> &attributes);
    void showLayerProperties(const QString &layerId, const QString &name, const QString &typeText,
                             const QString &source, bool visible, double opacity,
                             const std::optional<RasterMetadata> &rasterMeta,
                             const std::optional<VectorLayerInfo> &vectorMeta,
                             const std::optional<ModelPlacement> &modelPlacement = std::nullopt,
                             const std::optional<MeasurementLayerData> &measurementData = std::nullopt);
    void clearLayerProperties();
    void setRecentFiles(const QStringList &files);
    QString currentLayerId() const;
    void setActiveToolAction(int toolId);

signals:
    void importDataRequested(const QString &path);
    void layerSelected(const QString &layerId);
    void layerVisibilityChanged(const QString &layerId, bool visible);
    void layerOrderChanged(const QStringList &orderedLayerIds);
    void removeLayerRequested(const QString &layerId);
    void layerOpacityChanged(const QString &layerId, double opacity);
    void bandMappingChanged(const QString &layerId, int red, int green, int blue);
    void modelPlacementChanged(const QString &layerId, const ModelPlacement &placement);
    void toolChanged(int toolId);
    void undoMeasurementRequested();
    void editSelectedMeasurementRequested();
    void resetViewRequested();
    void zoomToLayerRequested(const QString &layerId);
    void screenshotRequested();
    void saveAndExitRequested();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void refreshToolActionStates();
    void updateRecentMenu();

    GlobeWidget *globeWidget_;
    LayerTreeDock *layerDock_;
    PropertyDock *propertyDock_;
    StatusBarController *statusController_;
    QAction *panAction_;
    QAction *pickAction_;
    QAction *measureAction_;
    QAction *measureAreaAction_;
    QAction *clearMeasureAction_;
    QAction *undoMeasureAction_;
    QAction *editMeasureAction_;
    QAction *homeAction_;
    QAction *screenshotAction_;
    QActionGroup *toolGroup_;
    QMenu *recentMenu_;
    QStringList recentFiles_;
    int currentToolId_ = 0;
    bool selectedMeasurementLayer_ = false;
};
