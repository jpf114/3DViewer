cmake_minimum_required(VERSION 3.25)
project(ThreeDViewer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_INSTALL_DEFAULT_CONFIG_NAME "Release" CACHE STRING
        "Default configuration for cmake --install when none is given.")
endif()

if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
endif()
if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
endif()
if(NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
endif()

if(NOT CMAKE_INSTALL_PREFIX OR CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Install prefix" FORCE)
endif()

set(THREEDVIEWER_OSGEARTH_DIR "" CACHE PATH "Directory containing osgEarthConfig.cmake")
if(NOT THREEDVIEWER_OSGEARTH_DIR AND DEFINED ENV{OSGEARTH_DIR})
    set(THREEDVIEWER_OSGEARTH_DIR "$ENV{OSGEARTH_DIR}" CACHE PATH
        "Directory containing osgEarthConfig.cmake" FORCE)
endif()
if(THREEDVIEWER_OSGEARTH_DIR AND NOT DEFINED osgEarth_DIR)
    set(osgEarth_DIR "${THREEDVIEWER_OSGEARTH_DIR}" CACHE PATH "Directory containing osgEarthConfig.cmake")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Svg)
find_package(OpenSceneGraph REQUIRED COMPONENTS osg osgDB osgGA osgUtil osgViewer)
find_package(osgEarth CONFIG QUIET)
if(NOT osgEarth_FOUND)
    message(FATAL_ERROR
        "osgEarth package was not found.\n"
        "Expected one of: osgEarthConfig.cmake, osgearth-config.cmake.\n"
        "What to check:\n"
        "  1. Install or build osgEarth for the same MSVC toolchain and architecture as this project.\n"
        "  2. Set THREEDVIEWER_OSGEARTH_DIR or osgEarth_DIR to the directory containing osgEarthConfig.cmake.\n"
        "  3. If you rely on environment variables, set OSGEARTH_DIR before running CMake.\n"
        "  4. Review docs/setup/windows-development-environment.md for the recommended Windows layout.\n"
    )
endif()
find_package(GDAL CONFIG REQUIRED)
find_package(PROJ CONFIG REQUIRED)
find_package(GEOS CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/RuntimeDeployment.cmake")

qt_standard_project_setup()

add_executable(ThreeDViewer
    src/main.cpp
    src/app/ApplicationController.cpp
    src/app/MainWindow.cpp
    src/app/ScreenshotSave.cpp
    src/common/GdalRuntime.cpp
    src/common/Logging.cpp
    src/data/DataImporter.cpp
    src/data/GdalDatasetInspector.cpp
    src/data/LayerConfig.cpp
    src/data/MapConfig.cpp
    src/data/ProjectConfig.cpp
    src/data/VectorFeatureQuery.cpp
    src/globe/GlobeWidget.cpp
    src/globe/SceneController.cpp
    src/layers/Layer.cpp
    src/layers/LayerManager.cpp
    src/tools/MeasureAreaTool.cpp
    src/tools/MeasureTool.cpp
    src/tools/PickTool.cpp
    src/tools/ToolManager.cpp
    src/ui/IconManager.cpp
    src/ui/LayerTreeDock.cpp
    src/ui/MeasurementResultsDock.cpp
    src/ui/PropertyDock.cpp
    src/ui/StatusBarController.cpp
)

set_target_properties(ThreeDViewer PROPERTIES OUTPUT_NAME 3DViewer)

if(MSVC)
    target_compile_options(ThreeDViewer PRIVATE /utf-8 /EHa /W4 /WX- /wd4819)
    target_compile_definitions(ThreeDViewer PRIVATE
        _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
        _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
    )
endif()

target_include_directories(ThreeDViewer PRIVATE src)
target_link_libraries(ThreeDViewer
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        Qt6::Svg
        osgEarth::osgEarth
        GDAL::GDAL
        PROJ::proj
        GEOS::geos
        spdlog::spdlog
)

threeviewer_runtime_post_build(ThreeDViewer)

if(WIN32)
    add_custom_command(TARGET ThreeDViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/fonts.conf"
            "$<TARGET_FILE_DIR:ThreeDViewer>/fonts.conf"
        COMMENT "Minimal fontconfig (FONTCONFIG_FILE) for Windows")

    add_custom_command(TARGET ThreeDViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/data"
            "$<TARGET_FILE_DIR:ThreeDViewer>/data"
        COMMENT "Copying data files")

    add_custom_command(TARGET ThreeDViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons"
            "$<TARGET_FILE_DIR:ThreeDViewer>/icons"
        COMMENT "Copying icon resources")

    add_custom_command(TARGET ThreeDViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/style.qss"
            "$<TARGET_FILE_DIR:ThreeDViewer>/style.qss"
        COMMENT "Copying QSS stylesheet")
endif()

enable_testing()
add_subdirectory(tests)

if(WIN32)
    threeviewer_install_target_release_only(ThreeDViewer)
    threeviewer_install_runtime_data_release()
    threeviewer_install_runtime_dlls(ThreeDViewer)
    threeviewer_install_windows_packaging_extras()

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/"
        DESTINATION data
        CONFIGURATIONS Release
        FILES_MATCHING PATTERN "*.tif" PATTERN "*.prj" PATTERN "*.tfw"
                       PATTERN "*.shp" PATTERN "*.shx" PATTERN "*.dbf"
                       PATTERN "*.prj" PATTERN "*.cpg" PATTERN "*.geojson"
                       PATTERN "*.json"
    )

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/"
        DESTINATION icons
        CONFIGURATIONS Release
    )

    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/style.qss"
        DESTINATION .
        CONFIGURATIONS Release
    )
else()
    install(TARGETS ThreeDViewer RUNTIME DESTINATION bin)
endif()
threeviewer_install_windeployqt_at_install()
