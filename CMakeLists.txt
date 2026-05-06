cmake_minimum_required(VERSION 3.25)
project(ThreeDViewer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
find_package(OpenSceneGraph REQUIRED COMPONENTS osg osgDB osgGA osgUtil osgViewer)
find_package(osgEarth CONFIG REQUIRED)
find_package(GDAL CONFIG REQUIRED)
find_package(PROJ CONFIG REQUIRED)
find_package(GEOS CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

qt_standard_project_setup()

add_executable(ThreeDViewer
    src/main.cpp
    src/app/ApplicationController.cpp
    src/app/MainWindow.cpp
    src/common/Logging.cpp
    src/data/DataImporter.cpp
    src/data/GdalDatasetInspector.cpp
    src/globe/GlobeWidget.cpp
    src/globe/SceneController.cpp
    src/layers/Layer.cpp
    src/layers/LayerManager.cpp
    src/tools/ToolManager.cpp
    src/ui/LayerTreeDock.cpp
    src/ui/PropertyDock.cpp
    src/ui/StatusBarController.cpp
)

set_target_properties(ThreeDViewer PROPERTIES OUTPUT_NAME 3DViewer)

target_include_directories(ThreeDViewer PRIVATE src)
target_link_libraries(ThreeDViewer
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        osgEarth::osgEarth
        GDAL::GDAL
        PROJ::proj
        GEOS::geos
        spdlog::spdlog
)

get_target_property(QT6_CORE_DLL_RELEASE Qt6::Core IMPORTED_LOCATION_RELEASE)
get_target_property(QT6_CORE_DLL_DEBUG Qt6::Core IMPORTED_LOCATION_DEBUG)

get_filename_component(QT6_BIN_DIR_RELEASE "${QT6_CORE_DLL_RELEASE}" DIRECTORY)
get_filename_component(QT6_BIN_DIR_DEBUG "${QT6_CORE_DLL_DEBUG}" DIRECTORY)

set(QT6_WINDOWS_PLATFORM_PLUGIN_RELEASE "${QT6_BIN_DIR_RELEASE}/../Qt6/plugins/platforms/qwindows.dll")
set(QT6_WINDOWS_PLATFORM_PLUGIN_DEBUG "${QT6_BIN_DIR_DEBUG}/../Qt6/plugins/platforms/qwindowsd.dll")

add_custom_command(TARGET ThreeDViewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:ThreeDViewer>/platforms"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<$<CONFIG:Debug>:${QT6_WINDOWS_PLATFORM_PLUGIN_DEBUG}>$<$<NOT:$<CONFIG:Debug>>:${QT6_WINDOWS_PLATFORM_PLUGIN_RELEASE}>"
        "$<TARGET_FILE_DIR:ThreeDViewer>/platforms/"
    VERBATIM
)

enable_testing()
add_subdirectory(tests)

install(TARGETS ThreeDViewer RUNTIME DESTINATION .)
install(FILES "${QT6_WINDOWS_PLATFORM_PLUGIN_RELEASE}" CONFIGURATIONS Release DESTINATION platforms)
install(FILES "${QT6_WINDOWS_PLATFORM_PLUGIN_DEBUG}" CONFIGURATIONS Debug DESTINATION platforms)
