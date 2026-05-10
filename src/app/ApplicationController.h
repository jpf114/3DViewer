#pragma once

#include <memory>
#include <QStringList>
#include <string>

#include "globe/PickResult.h"
#include "layers/MeasurementLayerData.h"

struct ModelPlacement;
class DataImporter;
class Layer;
class LayerManager;
class MainWindow;
class SceneController;

class ApplicationController {
public:
    ApplicationController(MainWindow &window,
                          SceneController &sceneController,
                          LayerManager &layerManager,
                          DataImporter &importer);

    void importFile(const std::string &path);
    void showLayerDetails(const std::string &layerId);
    void renameLayer(const std::string &layerId, const QString &name);
    void setLayerVisibility(const std::string &layerId, bool visible);
    void setLayerOpacity(const std::string &layerId, double opacity);
    void setModelPlacement(const std::string &layerId, const ModelPlacement &placement);
    void setBandMapping(const std::string &layerId, int red, int green, int blue);
    void removeLayer(const std::string &layerId);
    void zoomToLayer(const std::string &layerId);
    void resetView();
    void captureScreenshot();
    void loadBasemapAndLayers(const std::string &resourceDir);
    void saveLayerConfigOnExit(const std::string &resourceDir);
    bool saveProject(const QString &path);
    bool openProject(const QString &path);
    void saveProjectWithDialog();
    void saveProjectAsWithDialog();
    void openProjectWithDialog();
    void clearAllMeasurements();

private:
    void applyLayerOrderFromUi(const QStringList &orderedIds);
    bool updateMeasurementLayer(const std::shared_ptr<Layer> &layer, const MeasurementLayerData &data);
    void handleTerrainPick(const PickResult &pick);
    void addToRecentFiles(const QString &path);
    void addMeasurementLayer(const MeasurementLayerData &data);
    void editSelectedMeasurement();
    void exportSelectedMeasurement();
    void clearCurrentProject();

    MainWindow &window_;
    SceneController &sceneController_;
    LayerManager &layerManager_;
    DataImporter &importer_;
    std::string resourceDir_;
    QString currentProjectPath_;
};
