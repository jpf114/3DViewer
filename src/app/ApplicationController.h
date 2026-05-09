#pragma once

#include <QStringList>
#include <string>

#include "globe/PickResult.h"
#include "layers/MeasurementLayerData.h"

struct ModelPlacement;
class DataImporter;
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

private:
    void applyLayerOrderFromUi(const QStringList &orderedIds);
    void handleTerrainPick(const PickResult &pick);
    void addToRecentFiles(const QString &path);
    void addMeasurementLayer(const MeasurementLayerData &data);

    MainWindow &window_;
    SceneController &sceneController_;
    LayerManager &layerManager_;
    DataImporter &importer_;
};
