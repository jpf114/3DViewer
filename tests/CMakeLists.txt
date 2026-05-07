add_executable(ApplicationSmokeTests
    app/ApplicationSmokeTests.cpp
    ../src/app/MainWindow.cpp
    ../src/globe/GlobeWidget.cpp
    ../src/globe/SceneController.cpp
    ../src/layers/Layer.cpp
    ../src/tools/PickTool.cpp
    ../src/tools/ToolManager.cpp
    ../src/ui/LayerTreeDock.cpp
    ../src/ui/PropertyDock.cpp
    ../src/ui/StatusBarController.cpp
)

target_include_directories(ApplicationSmokeTests PRIVATE ../src)
target_link_libraries(ApplicationSmokeTests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets osgEarth::osgEarth spdlog::spdlog)

add_test(NAME ApplicationSmokeTests COMMAND ApplicationSmokeTests)

add_executable(SceneControllerTests
    globe/SceneControllerTests.cpp
    ../src/globe/SceneController.cpp
    ../src/layers/Layer.cpp
)

target_include_directories(SceneControllerTests PRIVATE ../src)
target_link_libraries(SceneControllerTests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets osgEarth::osgEarth spdlog::spdlog)

add_test(NAME SceneControllerTests COMMAND SceneControllerTests)

add_executable(LayerManagerTests
    layers/LayerManagerTests.cpp
    ../src/layers/Layer.cpp
    ../src/layers/LayerManager.cpp
)

target_include_directories(LayerManagerTests PRIVATE ../src)

add_test(NAME LayerManagerTests COMMAND LayerManagerTests)

add_executable(GdalDatasetInspectorTests
    data/GdalDatasetInspectorTests.cpp
    ../src/data/GdalDatasetInspector.cpp
)

target_compile_options(GdalDatasetInspectorTests PRIVATE /utf-8)
target_include_directories(GdalDatasetInspectorTests PRIVATE ../src)
target_link_libraries(GdalDatasetInspectorTests PRIVATE GDAL::GDAL spdlog::spdlog)

add_test(NAME GdalDatasetInspectorTests COMMAND GdalDatasetInspectorTests)

add_executable(DataImporterTests
    data/DataImporterTests.cpp
    ../src/data/GdalDatasetInspector.cpp
    ../src/data/DataImporter.cpp
    ../src/layers/Layer.cpp
)

target_compile_options(DataImporterTests PRIVATE /utf-8)
target_include_directories(DataImporterTests PRIVATE ../src)
target_link_libraries(DataImporterTests PRIVATE GDAL::GDAL spdlog::spdlog)

add_test(NAME DataImporterTests COMMAND DataImporterTests)
