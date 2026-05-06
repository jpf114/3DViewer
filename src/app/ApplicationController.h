#pragma once

#include <string>

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

private:
    MainWindow &window_;
    SceneController &sceneController_;
    LayerManager &layerManager_;
    DataImporter &importer_;
};
