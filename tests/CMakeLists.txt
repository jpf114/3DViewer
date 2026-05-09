add_executable(ApplicationSmokeTests
    app/ApplicationSmokeTests.cpp
    ../src/app/MainWindow.cpp
    ../src/globe/GlobeWidget.cpp
    ../src/globe/SceneController.cpp
    ../src/data/VectorFeatureQuery.cpp
    ../src/layers/Layer.cpp
    ../src/tools/MeasureAreaTool.cpp
    ../src/tools/MeasureTool.cpp
    ../src/tools/PickTool.cpp
    ../src/tools/ToolManager.cpp
    ../src/ui/IconManager.cpp
    ../src/ui/LayerTreeDock.cpp
    ../src/ui/PropertyDock.cpp
    ../src/ui/StatusBarController.cpp
)

target_include_directories(ApplicationSmokeTests PRIVATE ../src)
target_link_libraries(ApplicationSmokeTests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Svg osgEarth::osgEarth GDAL::GDAL spdlog::spdlog)
threeviewer_runtime_post_build(ApplicationSmokeTests)

add_test(NAME ApplicationSmokeTests COMMAND ApplicationSmokeTests)

add_executable(SceneControllerTests
    globe/SceneControllerTests.cpp
    ../src/globe/SceneController.cpp
    ../src/data/VectorFeatureQuery.cpp
    ../src/layers/Layer.cpp
)

target_include_directories(SceneControllerTests PRIVATE ../src)
target_link_libraries(SceneControllerTests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets osgEarth::osgEarth GDAL::GDAL spdlog::spdlog)
threeviewer_runtime_post_build(SceneControllerTests)

add_test(NAME SceneControllerTests COMMAND SceneControllerTests)

add_executable(InputMappingTests
    globe/InputMappingTests.cpp
)

target_include_directories(InputMappingTests PRIVATE ../src)
target_link_libraries(InputMappingTests PRIVATE Qt6::Core)

add_test(NAME InputMappingTests COMMAND InputMappingTests)

add_executable(MeasurementUtilsTests
    globe/MeasurementUtilsTests.cpp
)

target_include_directories(MeasurementUtilsTests PRIVATE ../src)
target_link_libraries(MeasurementUtilsTests PRIVATE Qt6::Core)

add_test(NAME MeasurementUtilsTests COMMAND MeasurementUtilsTests)

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
threeviewer_runtime_post_build(GdalDatasetInspectorTests)

add_test(NAME GdalDatasetInspectorTests COMMAND GdalDatasetInspectorTests)

add_executable(MapConfigTests
    data/MapConfigTests.cpp
    ../src/data/MapConfig.cpp
)

target_compile_options(MapConfigTests PRIVATE /utf-8)
target_include_directories(MapConfigTests PRIVATE ../src)
target_link_libraries(MapConfigTests PRIVATE Qt6::Core spdlog::spdlog)

add_test(NAME MapConfigTests COMMAND MapConfigTests)

add_executable(LayerConfigTests
    data/LayerConfigTests.cpp
    ../src/data/LayerConfig.cpp
)

target_compile_options(LayerConfigTests PRIVATE /utf-8)
target_include_directories(LayerConfigTests PRIVATE ../src)
target_link_libraries(LayerConfigTests PRIVATE Qt6::Core spdlog::spdlog)

add_test(NAME LayerConfigTests COMMAND LayerConfigTests)

add_executable(DataImporterTests
    data/DataImporterTests.cpp
    ../src/data/GdalDatasetInspector.cpp
    ../src/data/DataImporter.cpp
    ../src/layers/Layer.cpp
)

target_compile_options(DataImporterTests PRIVATE /utf-8)
target_include_directories(DataImporterTests PRIVATE ../src)
target_link_libraries(DataImporterTests PRIVATE GDAL::GDAL osgEarth::osgEarth spdlog::spdlog)
threeviewer_runtime_post_build(DataImporterTests)

add_test(NAME DataImporterTests COMMAND DataImporterTests)

add_executable(VectorFeatureQueryTests
    data/VectorFeatureQueryTests.cpp
    ../src/data/VectorFeatureQuery.cpp
)

target_compile_options(VectorFeatureQueryTests PRIVATE /utf-8)
target_include_directories(VectorFeatureQueryTests PRIVATE ../src)
target_link_libraries(VectorFeatureQueryTests PRIVATE Qt6::Core GDAL::GDAL spdlog::spdlog)
threeviewer_runtime_post_build(VectorFeatureQueryTests)

add_test(NAME VectorFeatureQueryTests COMMAND VectorFeatureQueryTests)
