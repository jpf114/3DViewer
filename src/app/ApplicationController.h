#pragma once

#include <QStringList>
#include <string>

#include "globe/PickResult.h"

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
    void setBandMapping(const std::string &layerId, int red, int green, int blue);
    void removeLayer(const std::string &layerId);

private:
    void applyLayerOrderFromUi(const QStringList &orderedIds);
    void handleTerrainPick(const PickResult &pick);

    MainWindow &window_;
    SceneController &sceneController_;
    LayerManager &layerManager_;
    DataImporter &importer_;
};
