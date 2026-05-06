# 3DViewer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a desktop professional 3D globe application on Windows using Qt, osgEarth, OSG, and GDAL, starting with a stable platform skeleton and a first usable GIS feature set.

**Architecture:** The application is split into application shell, globe rendering, layer model, data import, and interaction tools. All external data enters through importer/descriptor objects, is normalized into application-owned layer objects, and is then mapped to osgEarth scene objects by a dedicated scene controller.

**Tech Stack:** CMake, C++20, Qt6 Widgets, OpenSceneGraph, osgEarth, GDAL/OGR, PROJ, GEOS, spdlog, global vcpkg toolchain, Windows x64

---

## Scope and sequencing

This project is large enough that it must be delivered in phases. The plan below intentionally builds the platform in this order:

1. Toolchain and project skeleton
2. Qt6 + osgEarth integration proof
3. Application-owned layer model
4. Raster/DEM/vector import path
5. Core desktop UX
6. Packaging and runtime dependency copying
7. Deferred features: chart and NetCDF support

The first execution target is a usable MVP:

- Qt main window
- Embedded 3D globe
- Base imagery
- Local GeoTIFF import
- Local vector import
- DEM/elevation layer import
- Layer tree visibility/order control
- Basic pick/query/status display

## Planned file structure

### Project root

- Create: `CMakeLists.txt`
- Create: `cmake/RuntimeDeployment.cmake`
- Create: `src/main.cpp`
- Create: `src/app/ApplicationController.h`
- Create: `src/app/ApplicationController.cpp`
- Create: `src/app/MainWindow.h`
- Create: `src/app/MainWindow.cpp`
- Create: `src/ui/LayerTreeDock.h`
- Create: `src/ui/LayerTreeDock.cpp`
- Create: `src/ui/PropertyDock.h`
- Create: `src/ui/PropertyDock.cpp`
- Create: `src/ui/StatusBarController.h`
- Create: `src/ui/StatusBarController.cpp`
- Create: `src/globe/GlobeWidget.h`
- Create: `src/globe/GlobeWidget.cpp`
- Create: `src/globe/SceneController.h`
- Create: `src/globe/SceneController.cpp`
- Create: `src/globe/PickResult.h`
- Create: `src/layers/LayerTypes.h`
- Create: `src/layers/Layer.h`
- Create: `src/layers/Layer.cpp`
- Create: `src/layers/ImageryLayer.h`
- Create: `src/layers/ElevationLayer.h`
- Create: `src/layers/VectorLayer.h`
- Create: `src/layers/ChartLayer.h`
- Create: `src/layers/ScientificLayer.h`
- Create: `src/layers/LayerManager.h`
- Create: `src/layers/LayerManager.cpp`
- Create: `src/data/DataSourceDescriptor.h`
- Create: `src/data/DataImporter.h`
- Create: `src/data/DataImporter.cpp`
- Create: `src/data/GdalDatasetInspector.h`
- Create: `src/data/GdalDatasetInspector.cpp`
- Create: `src/tools/MapTool.h`
- Create: `src/tools/PanTool.h`
- Create: `src/tools/PickTool.h`
- Create: `src/tools/ToolManager.h`
- Create: `src/tools/ToolManager.cpp`
- Create: `src/common/Result.h`
- Create: `src/common/Logging.h`
- Create: `src/common/Logging.cpp`
- Create: `tests/CMakeLists.txt`
- Create: `tests/layers/LayerManagerTests.cpp`
- Create: `tests/data/GdalDatasetInspectorTests.cpp`
- Create: `tests/data/DataImporterTests.cpp`
- Create: `tests/app/ApplicationSmokeTests.cpp`
- Create: `assets/earth/README.md`
- Create: `docs/architecture/3dviewer-architecture.md`
- Create: `docs/notes/runtime-deployment.md`

### Responsibility map

- `src/app`: application orchestration, window composition, top-level actions
- `src/ui`: view widgets and docks that do not own GIS/business logic
- `src/globe`: osgEarth/OSG embedding, map scene, pick bridge
- `src/layers`: application-owned layer model and layer state management
- `src/data`: dataset inspection and conversion from files to layer descriptors
- `src/tools`: map interaction tool state and event dispatch
- `src/common`: logging and small shared utility types
- `tests`: focused unit and smoke tests

## Architecture rules

These constraints apply to every implementation task:

1. UI classes must not directly create or mutate osgEarth layer objects.
2. GDAL inspection/import code must not directly modify the scene graph.
3. `LayerManager` is the single source of truth for application-visible layer state.
4. `SceneController` owns the mapping from app layers to osgEarth/OSG objects.
5. First implementation target is Qt Widgets, not QML.
6. ImGui is optional and only for diagnostics, never the primary application shell.
7. Chart and NetCDF support are designed in interfaces now, but implemented later.

## Milestones

### Milestone A: Platform skeleton

Success criteria:

- Empty Qt app builds and starts
- Main window, docks, and central globe placeholder appear
- Logging works in debug and release

### Milestone B: Globe integration

Success criteria:

- osgEarth globe renders inside Qt
- Camera interaction works
- Base imagery loads
- Status bar can show cursor and camera information

### Milestone C: Core GIS import

Success criteria:

- GeoTIFF imports as imagery
- DEM imports as elevation
- Shapefile/GeoJSON/GPKG imports as vector
- Layer tree can control visibility and ordering

### Milestone D: Usable MVP

Success criteria:

- Pick/query works on map and layers
- View can zoom to layer extent
- Project can be built, installed, and run with copied runtime dependencies

### Milestone E: Deferred professional extensions

Success criteria:

- Interfaces for chart and scientific layers exist
- Follow-up plans are written for S-57 and NetCDF pipelines

## Task plan

### Task 1: Create the build skeleton and top-level CMake layout

**Files:**
- Create: `CMakeLists.txt`
- Create: `tests/CMakeLists.txt`
- Create: `src/main.cpp`
- Create: `src/common/Logging.h`
- Create: `src/common/Logging.cpp`

- [ ] **Step 1: Write the failing smoke test target declaration**

```cmake
enable_testing()
add_subdirectory(tests)
```

Add this to the root `CMakeLists.txt` before any tests exist so configuration fails until the `tests` directory is created.

- [ ] **Step 2: Create the root CMake project file**

```cmake
cmake_minimum_required(VERSION 3.25)
project(3DViewer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
find_package(OpenSceneGraph CONFIG REQUIRED COMPONENTS osg osgDB osgGA osgViewer osgUtil)
find_package(osgEarth CONFIG REQUIRED)
find_package(GDAL CONFIG REQUIRED)
find_package(PROJ CONFIG REQUIRED)
find_package(GEOS CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

qt_standard_project_setup()

add_executable(3DViewer
    src/main.cpp
    src/common/Logging.cpp
)

target_include_directories(3DViewer PRIVATE src)
target_link_libraries(3DViewer
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        osg
        osgDB
        osgGA
        osgViewer
        osgUtil
        GDAL::GDAL
        PROJ::proj
        GEOS::geos
        spdlog::spdlog
)

enable_testing()
add_subdirectory(tests)

install(TARGETS 3DViewer RUNTIME DESTINATION .)
```

- [ ] **Step 3: Create the initial logging utility**

```cpp
#pragma once

namespace logging {
void initialize();
}
```

```cpp
#include "common/Logging.h"

#include <spdlog/spdlog.h>

namespace logging {
void initialize() {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    spdlog::info("logging initialized");
}
}
```

- [ ] **Step 4: Create the minimal application entry point**

```cpp
#include <QApplication>
#include <QLabel>

#include "common/Logging.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    logging::initialize();

    QLabel placeholder("3DViewer bootstrap");
    placeholder.resize(640, 480);
    placeholder.show();

    return app.exec();
}
```

- [ ] **Step 5: Create the initial test CMake file**

```cmake
add_executable(ApplicationSmokeTests
    app/ApplicationSmokeTests.cpp
)

target_link_libraries(ApplicationSmokeTests PRIVATE Qt6::Core)

add_test(NAME ApplicationSmokeTests COMMAND ApplicationSmokeTests)
```

- [ ] **Step 6: Create the first smoke test**

```cpp
#include <iostream>

int main() {
    std::cout << "ApplicationSmokeTests placeholder\n";
    return 0;
}
```

- [ ] **Step 7: Configure and build debug**

Run: `cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=D:/Code/GitHubCode/vcpkg/scripts/buildsystems/vcpkg.cmake`  
Expected: configure succeeds

Run: `cmake --build build --config Debug`
Expected: `3DViewer` and `ApplicationSmokeTests` build successfully

- [ ] **Step 8: Commit**

```bash
git add CMakeLists.txt tests/CMakeLists.txt src/main.cpp src/common/Logging.* tests/app/ApplicationSmokeTests.cpp
git commit -m "build: add initial Qt and dependency skeleton"
```

### Task 2: Establish the main window shell and UI docking layout

**Files:**
- Create: `src/app/MainWindow.h`
- Create: `src/app/MainWindow.cpp`
- Create: `src/ui/LayerTreeDock.h`
- Create: `src/ui/LayerTreeDock.cpp`
- Create: `src/ui/PropertyDock.h`
- Create: `src/ui/PropertyDock.cpp`
- Create: `src/ui/StatusBarController.h`
- Create: `src/ui/StatusBarController.cpp`
- Modify: `src/main.cpp`

- [ ] **Step 1: Write the failing application shell test**

```cpp
#include "app/MainWindow.h"

int main() {
    MainWindow window;
    return window.windowTitle().isEmpty() ? 1 : 0;
}
```

- [ ] **Step 2: Create the dock widget headers**

```cpp
#pragma once

#include <QDockWidget>
#include <QTreeWidget>

class LayerTreeDock : public QDockWidget {
    Q_OBJECT
public:
    explicit LayerTreeDock(QWidget *parent = nullptr);
    QTreeWidget *tree() const;
private:
    QTreeWidget *tree_;
};
```

```cpp
#pragma once

#include <QDockWidget>
#include <QTextEdit>

class PropertyDock : public QDockWidget {
    Q_OBJECT
public:
    explicit PropertyDock(QWidget *parent = nullptr);
    void showText(const QString &text);
private:
    QTextEdit *text_;
};
```

- [ ] **Step 3: Implement the dock widgets**

```cpp
#include "ui/LayerTreeDock.h"

LayerTreeDock::LayerTreeDock(QWidget *parent)
    : QDockWidget("Layers", parent), tree_(new QTreeWidget(this)) {
    tree_->setHeaderLabel("Scene");
    setWidget(tree_);
}

QTreeWidget *LayerTreeDock::tree() const {
    return tree_;
}
```

```cpp
#include "ui/PropertyDock.h"

PropertyDock::PropertyDock(QWidget *parent)
    : QDockWidget("Properties", parent), text_(new QTextEdit(this)) {
    text_->setReadOnly(true);
    setWidget(text_);
}

void PropertyDock::showText(const QString &text) {
    text_->setPlainText(text);
}
```

- [ ] **Step 4: Implement the status controller**

```cpp
#pragma once

#include <QObject>

class QMainWindow;
class QLabel;

class StatusBarController : public QObject {
    Q_OBJECT
public:
    explicit StatusBarController(QMainWindow *window);
    void setCursorText(const QString &text);
private:
    QLabel *cursorLabel_;
};
```

```cpp
#include "ui/StatusBarController.h"

#include <QLabel>
#include <QMainWindow>
#include <QStatusBar>

StatusBarController::StatusBarController(QMainWindow *window)
    : QObject(window), cursorLabel_(new QLabel("Lon: -, Lat: -, Elev: -", window)) {
    window->statusBar()->addPermanentWidget(cursorLabel_);
}

void StatusBarController::setCursorText(const QString &text) {
    cursorLabel_->setText(text);
}
```

- [ ] **Step 5: Implement the main window**

```cpp
#pragma once

#include <QMainWindow>

class LayerTreeDock;
class PropertyDock;
class StatusBarController;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
private:
    LayerTreeDock *layerDock_;
    PropertyDock *propertyDock_;
    StatusBarController *statusController_;
};
```

```cpp
#include "app/MainWindow.h"

#include <QLabel>

#include "ui/LayerTreeDock.h"
#include "ui/PropertyDock.h"
#include "ui/StatusBarController.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      layerDock_(new LayerTreeDock(this)),
      propertyDock_(new PropertyDock(this)),
      statusController_(new StatusBarController(this)) {
    setWindowTitle("3DViewer");
    resize(1600, 900);
    setCentralWidget(new QLabel("Globe view placeholder", this));
    addDockWidget(Qt::LeftDockWidgetArea, layerDock_);
    addDockWidget(Qt::RightDockWidgetArea, propertyDock_);
}
```

- [ ] **Step 6: Update the application entry point to use MainWindow**

```cpp
#include <QApplication>

#include "app/MainWindow.h"
#include "common/Logging.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    logging::initialize();

    MainWindow window;
    window.show();

    return app.exec();
}
```

- [ ] **Step 7: Build and run the shell**

Run: `cmake --build build --config Debug`
Expected: build succeeds

Run: `build/Debug/3DViewer.exe`
Expected: main window opens with left layers dock, right properties dock, and status bar

- [ ] **Step 8: Commit**

```bash
git add src/app/MainWindow.* src/ui/LayerTreeDock.* src/ui/PropertyDock.* src/ui/StatusBarController.* src/main.cpp tests/app/ApplicationSmokeTests.cpp
git commit -m "feat: add desktop application shell"
```

### Task 3: Add application-owned layer model and manager

**Files:**
- Create: `src/layers/LayerTypes.h`
- Create: `src/layers/Layer.h`
- Create: `src/layers/Layer.cpp`
- Create: `src/layers/ImageryLayer.h`
- Create: `src/layers/ElevationLayer.h`
- Create: `src/layers/VectorLayer.h`
- Create: `src/layers/ChartLayer.h`
- Create: `src/layers/ScientificLayer.h`
- Create: `src/layers/LayerManager.h`
- Create: `src/layers/LayerManager.cpp`
- Test: `tests/layers/LayerManagerTests.cpp`

- [ ] **Step 1: Write the failing layer manager test**

```cpp
#include "layers/LayerManager.h"
#include "layers/ImageryLayer.h"

int main() {
    LayerManager manager;
    auto layer = std::make_shared<ImageryLayer>("world", "World Imagery", "memory://world");
    manager.addLayer(layer);
    return manager.layers().size() == 1 ? 0 : 1;
}
```

- [ ] **Step 2: Define the layer kinds**

```cpp
#pragma once

enum class LayerKind {
    Imagery,
    Elevation,
    Vector,
    Chart,
    Scientific
};
```

- [ ] **Step 3: Define the base layer type**

```cpp
#pragma once

#include <memory>
#include <string>

#include "layers/LayerTypes.h"

class Layer {
public:
    Layer(std::string id, std::string name, std::string sourceUri, LayerKind kind);
    virtual ~Layer() = default;

    const std::string &id() const;
    const std::string &name() const;
    const std::string &sourceUri() const;
    LayerKind kind() const;
    bool visible() const;
    double opacity() const;

    void setVisible(bool visible);
    void setOpacity(double opacity);

private:
    std::string id_;
    std::string name_;
    std::string sourceUri_;
    LayerKind kind_;
    bool visible_ = true;
    double opacity_ = 1.0;
};
```

- [ ] **Step 4: Implement the layer base**

```cpp
#include "layers/Layer.h"

Layer::Layer(std::string id, std::string name, std::string sourceUri, LayerKind kind)
    : id_(std::move(id)), name_(std::move(name)), sourceUri_(std::move(sourceUri)), kind_(kind) {}

const std::string &Layer::id() const { return id_; }
const std::string &Layer::name() const { return name_; }
const std::string &Layer::sourceUri() const { return sourceUri_; }
LayerKind Layer::kind() const { return kind_; }
bool Layer::visible() const { return visible_; }
double Layer::opacity() const { return opacity_; }
void Layer::setVisible(bool visible) { visible_ = visible; }
void Layer::setOpacity(double opacity) { opacity_ = opacity; }
```

- [ ] **Step 5: Define the concrete layer types**

```cpp
#pragma once

#include "layers/Layer.h"

class ImageryLayer : public Layer {
public:
    ImageryLayer(std::string id, std::string name, std::string sourceUri)
        : Layer(std::move(id), std::move(name), std::move(sourceUri), LayerKind::Imagery) {}
};
```

Repeat the same pattern for `ElevationLayer`, `VectorLayer`, `ChartLayer`, and `ScientificLayer`, changing only the `LayerKind`.

- [ ] **Step 6: Define and implement the layer manager**

```cpp
#pragma once

#include <memory>
#include <vector>

class Layer;

class LayerManager {
public:
    void addLayer(const std::shared_ptr<Layer> &layer);
    const std::vector<std::shared_ptr<Layer>> &layers() const;
private:
    std::vector<std::shared_ptr<Layer>> layers_;
};
```

```cpp
#include "layers/LayerManager.h"

#include "layers/Layer.h"

void LayerManager::addLayer(const std::shared_ptr<Layer> &layer) {
    layers_.push_back(layer);
}

const std::vector<std::shared_ptr<Layer>> &LayerManager::layers() const {
    return layers_;
}
```

- [ ] **Step 7: Run the test**

Run: `cmake --build build --config Debug`
Expected: build succeeds

Run: `ctest --test-dir build -C Debug --output-on-failure`
Expected: `LayerManagerTests` passes

- [ ] **Step 8: Commit**

```bash
git add src/layers/*.h src/layers/*.cpp tests/layers/LayerManagerTests.cpp CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat: add application layer model"
```

### Task 4: Embed osgEarth inside Qt and render a base globe

**Files:**
- Create: `src/globe/GlobeWidget.h`
- Create: `src/globe/GlobeWidget.cpp`
- Create: `src/globe/SceneController.h`
- Create: `src/globe/SceneController.cpp`
- Modify: `src/app/MainWindow.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing globe smoke test**

```cpp
#include "globe/SceneController.h"

int main() {
    SceneController controller;
    return controller.isInitialized() ? 0 : 1;
}
```

- [ ] **Step 2: Define the scene controller surface**

```cpp
#pragma once

#include <memory>

namespace osgEarth { class Map; }
namespace osgViewer { class Viewer; }

class SceneController {
public:
    SceneController();
    bool isInitialized() const;
    void initializeDefaultScene();
private:
    bool initialized_ = false;
};
```

- [ ] **Step 3: Implement the scene controller bootstrap**

```cpp
#include "globe/SceneController.h"

SceneController::SceneController() = default;

bool SceneController::isInitialized() const {
    return initialized_;
}

void SceneController::initializeDefaultScene() {
    initialized_ = true;
}
```

- [ ] **Step 4: Create the globe widget wrapper**

```cpp
#pragma once

#include <QOpenGLWidget>

#include "globe/SceneController.h"

class GlobeWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit GlobeWidget(QWidget *parent = nullptr);
    SceneController &sceneController();
protected:
    void initializeGL() override;
private:
    SceneController sceneController_;
};
```

```cpp
#include "globe/GlobeWidget.h"

GlobeWidget::GlobeWidget(QWidget *parent) : QOpenGLWidget(parent) {}

SceneController &GlobeWidget::sceneController() {
    return sceneController_;
}

void GlobeWidget::initializeGL() {
    sceneController_.initializeDefaultScene();
}
```

- [ ] **Step 5: Replace the main window placeholder with the globe widget**

```cpp
setCentralWidget(new GlobeWidget(this));
```

- [ ] **Step 6: Expand the scene controller from stub to osgEarth-backed implementation**

Implementation requirements:

- Create an `osgEarth::Map`
- Create an `osgEarth::MapNode`
- Configure an `osgViewer::Viewer`
- Attach a default earth manipulator
- Add a temporary base imagery layer

Code must live in `src/globe/SceneController.cpp`; keep Qt-facing concerns in `GlobeWidget.cpp`.

- [ ] **Step 7: Manual validation**

Run: `build/Debug/3DViewer.exe`
Expected: the central widget renders a globe, not a blank label

- [ ] **Step 8: Commit**

```bash
git add src/globe/*.h src/globe/*.cpp src/app/MainWindow.cpp CMakeLists.txt tests/app/ApplicationSmokeTests.cpp
git commit -m "feat: embed osgEarth globe in Qt shell"
```

### Task 5: Add dataset inspection and file import pipeline

**Files:**
- Create: `src/data/DataSourceDescriptor.h`
- Create: `src/data/GdalDatasetInspector.h`
- Create: `src/data/GdalDatasetInspector.cpp`
- Create: `src/data/DataImporter.h`
- Create: `src/data/DataImporter.cpp`
- Test: `tests/data/GdalDatasetInspectorTests.cpp`
- Test: `tests/data/DataImporterTests.cpp`

- [ ] **Step 1: Write the failing descriptor test**

```cpp
#include "data/DataSourceDescriptor.h"

int main() {
    DataSourceDescriptor descriptor;
    return descriptor.path.empty() ? 1 : 0;
}
```

- [ ] **Step 2: Define the dataset descriptor**

```cpp
#pragma once

#include <string>

#include "layers/LayerTypes.h"

struct DataSourceDescriptor {
    std::string path;
    std::string driverName;
    std::string crsWkt;
    LayerKind suggestedKind = LayerKind::Imagery;
    bool isRaster = false;
    bool isVector = false;
    bool hasElevation = false;
};
```

- [ ] **Step 3: Implement the GDAL dataset inspector**

```cpp
#pragma once

#include <optional>
#include <string>

#include "data/DataSourceDescriptor.h"

class GdalDatasetInspector {
public:
    std::optional<DataSourceDescriptor> inspect(const std::string &path) const;
};
```

Implementation requirements in `GdalDatasetInspector.cpp`:

- call `GDALAllRegister()`
- open dataset read-only
- detect raster vs vector
- detect elevation candidate from raster band count / metadata
- extract driver short name
- extract projection WKT when available

- [ ] **Step 4: Implement the importer**

```cpp
#pragma once

#include <memory>
#include <optional>

#include "data/GdalDatasetInspector.h"

class Layer;

class DataImporter {
public:
    explicit DataImporter(GdalDatasetInspector inspector = {});
    std::shared_ptr<Layer> importPath(const std::string &path) const;
private:
    GdalDatasetInspector inspector_;
};
```

Implementation requirements in `DataImporter.cpp`:

- inspect the path
- map descriptor kind to `ImageryLayer`, `ElevationLayer`, or `VectorLayer`
- derive layer id from file stem
- derive display name from file name
- return `nullptr` if the path is unsupported

- [ ] **Step 5: Add importer tests**

Add fixtures under `tests/data/fixtures/` later if needed, but first cover these cases with narrow tests:

- missing file returns `nullptr`
- raster descriptor maps to `ImageryLayer`
- elevation descriptor maps to `ElevationLayer`
- vector descriptor maps to `VectorLayer`

- [ ] **Step 6: Run the tests**

Run: `cmake --build build --config Debug`
Expected: build succeeds

Run: `ctest --test-dir build -C Debug --output-on-failure`
Expected: all data tests pass

- [ ] **Step 7: Commit**

```bash
git add src/data/*.h src/data/*.cpp tests/data/*.cpp
git commit -m "feat: add GDAL-backed import pipeline"
```

### Task 6: Connect imported layers to the scene and layer tree

**Files:**
- Modify: `src/app/ApplicationController.h`
- Modify: `src/app/ApplicationController.cpp`
- Modify: `src/app/MainWindow.h`
- Modify: `src/app/MainWindow.cpp`
- Modify: `src/ui/LayerTreeDock.h`
- Modify: `src/ui/LayerTreeDock.cpp`
- Modify: `src/globe/SceneController.h`
- Modify: `src/globe/SceneController.cpp`
- Modify: `src/layers/LayerManager.h`
- Modify: `src/layers/LayerManager.cpp`

- [ ] **Step 1: Create the application controller interface**

```cpp
#pragma once

#include <memory>

class MainWindow;
class SceneController;
class LayerManager;
class DataImporter;

class ApplicationController {
public:
    ApplicationController(MainWindow &window, SceneController &sceneController, LayerManager &layerManager, DataImporter &importer);
    void importFile(const std::string &path);
private:
    MainWindow &window_;
    SceneController &sceneController_;
    LayerManager &layerManager_;
    DataImporter &importer_;
};
```

- [ ] **Step 2: Implement file import flow**

Implementation requirements:

- `importFile()` calls `DataImporter`
- successful imports are added to `LayerManager`
- `SceneController` is told to add the layer to osgEarth
- `MainWindow` updates the layer tree row

- [ ] **Step 3: Extend `LayerManager`**

Add methods:

```cpp
std::shared_ptr<Layer> findById(const std::string &id) const;
void moveLayer(std::size_t from, std::size_t to);
void setVisibility(const std::string &id, bool visible);
```

- [ ] **Step 4: Extend `SceneController`**

Add methods:

```cpp
void addLayer(const std::shared_ptr<Layer> &layer);
void removeLayer(const std::string &id);
void syncLayerState(const std::shared_ptr<Layer> &layer);
```

Implementation requirements:

- imagery maps to osgEarth image layer
- elevation maps to osgEarth elevation layer
- vector maps to osgEarth feature/model pathway appropriate for first MVP
- maintain an internal `std::unordered_map<std::string, osg::ref_ptr<osg::Referenced>>`

- [ ] **Step 5: Extend layer tree UI**

Implementation requirements:

- each tree row stores layer id
- rows are checkable for visibility
- visibility toggles call back into application controller

- [ ] **Step 6: Add a File -> Import Data action**

Implementation requirements:

- use `QFileDialog::getOpenFileName`
- pass selected path to `ApplicationController::importFile`
- show an error dialog on unsupported paths

- [ ] **Step 7: Manual validation**

Run: `build/Debug/3DViewer.exe`
Expected:

- import menu action opens file dialog
- imported raster/vector appears in layer tree
- visibility checkbox affects rendered scene

- [ ] **Step 8: Commit**

```bash
git add src/app/*.h src/app/*.cpp src/ui/LayerTreeDock.* src/globe/SceneController.* src/layers/LayerManager.* 
git commit -m "feat: connect import pipeline to scene and layer tree"
```

### Task 7: Add pick/query and status updates

**Files:**
- Create: `src/globe/PickResult.h`
- Create: `src/tools/MapTool.h`
- Create: `src/tools/PanTool.h`
- Create: `src/tools/PickTool.h`
- Create: `src/tools/ToolManager.h`
- Create: `src/tools/ToolManager.cpp`
- Modify: `src/globe/GlobeWidget.h`
- Modify: `src/globe/GlobeWidget.cpp`
- Modify: `src/ui/StatusBarController.h`
- Modify: `src/ui/StatusBarController.cpp`
- Modify: `src/ui/PropertyDock.h`
- Modify: `src/ui/PropertyDock.cpp`

- [ ] **Step 1: Define the pick result**

```cpp
#pragma once

#include <optional>
#include <string>

struct PickResult {
    bool hit = false;
    double longitude = 0.0;
    double latitude = 0.0;
    double elevation = 0.0;
    std::string layerId;
    std::string displayText;
};
```

- [ ] **Step 2: Define the map tool interface**

```cpp
#pragma once

#include <QMouseEvent>

class GlobeWidget;

class MapTool {
public:
    virtual ~MapTool() = default;
    virtual void mousePressEvent(GlobeWidget &, QMouseEvent *) {}
    virtual void mouseMoveEvent(GlobeWidget &, QMouseEvent *) {}
    virtual void mouseReleaseEvent(GlobeWidget &, QMouseEvent *) {}
};
```

- [ ] **Step 3: Add tool manager and pick tool**

Implementation requirements:

- `ToolManager` owns the current `MapTool`
- `PickTool` requests pick results from `SceneController`
- `PickTool` updates property dock and status bar through callbacks

- [ ] **Step 4: Extend `GlobeWidget`**

Implementation requirements:

- forward mouse events to `ToolManager`
- emit cursor coordinate updates
- keep default pan/orbit behavior working when no special tool is active

- [ ] **Step 5: Extend `SceneController` with picking**

Add:

```cpp
PickResult pickAt(int x, int y) const;
```

Implementation requirements:

- convert screen coordinate to map intersection
- resolve lon/lat/elevation
- if a feature/layer is hit, populate `layerId` and `displayText`

- [ ] **Step 6: Manual validation**

Run: `build/Debug/3DViewer.exe`
Expected:

- moving on globe updates status text
- clicking map shows query details in properties dock

- [ ] **Step 7: Commit**

```bash
git add src/globe/PickResult.h src/tools/*.h src/tools/*.cpp src/globe/GlobeWidget.* src/ui/StatusBarController.* src/ui/PropertyDock.*
git commit -m "feat: add map pick and query workflow"
```

### Task 8: Add runtime deployment helpers and install validation

**Files:**
- Create: `cmake/RuntimeDeployment.cmake`
- Modify: `CMakeLists.txt`
- Create: `docs/notes/runtime-deployment.md`

- [ ] **Step 1: Create the runtime deployment helper**

Implementation requirements in `cmake/RuntimeDeployment.cmake`:

- copy Qt runtime DLLs for the active config
- copy osgEarth/OSG DLLs
- copy osgEarth/OSG plugin directories
- copy GDAL data and PROJ data directories
- keep all paths configurable through cache variables

- [ ] **Step 2: Hook deployment into install**

Add root CMake logic similar to:

```cmake
include(cmake/RuntimeDeployment.cmake)
install_runtime_dependencies(TARGET 3DViewer DESTINATION .)
```

- [ ] **Step 3: Document local install rules**

Write `docs/notes/runtime-deployment.md` with:

- expected build tree layout
- expected install tree layout
- which runtime folders are copied from global vcpkg installation
- how to validate release startup on a clean machine or shell

- [ ] **Step 4: Validate install**

Run: `cmake --install build --config Debug --prefix install`
Expected: `install/` contains executable plus required runtime payload

Run: `install/3DViewer.exe`
Expected: application starts without requiring the original build directory

- [ ] **Step 5: Commit**

```bash
git add cmake/RuntimeDeployment.cmake CMakeLists.txt docs/notes/runtime-deployment.md
git commit -m "build: add runtime deployment helpers"
```

### Task 9: Write the companion architecture note

**Files:**
- Create: `docs/architecture/3dviewer-architecture.md`

- [ ] **Step 1: Document the runtime architecture**

Include:

- layer flow from import to render
- component ownership
- directory responsibilities
- deferred extension points for chart and scientific layers
- deployment assumptions for global vcpkg + local install copying

- [ ] **Step 2: Commit**

```bash
git add docs/architecture/3dviewer-architecture.md
git commit -m "docs: add architecture reference"
```

### Task 10: Create follow-up plans for chart and NetCDF support

**Files:**
- Create: `docs/followups/chart-support-plan.md`
- Create: `docs/followups/netcdf-support-plan.md`

- [ ] **Step 1: Write the chart follow-up plan**

Must cover:

- S-57/ENC ingestion strategy
- symbolization limits for first release
- attribute inspection
- compatibility risks

- [ ] **Step 2: Write the NetCDF follow-up plan**

Must cover:

- variable/time-step selection
- rasterized 2D overlay first
- deferred particle/volume rendering
- test dataset strategy

- [ ] **Step 3: Commit**

```bash
git add docs/followups/chart-support-plan.md docs/followups/netcdf-support-plan.md
git commit -m "docs: add deferred data source plans"
```

## Cross-cutting testing rules

For every executable milestone:

- Build both `Debug` and `Release`
- Run `ctest --test-dir build -C Debug --output-on-failure`
- Run the desktop app manually after any rendering or deployment change
- When adding importer logic, verify at least one positive and one negative case

## Risks and mitigation

### Risk 1: Qt6 and osgEarth embedding friction

Mitigation:

- validate globe embedding before importer work
- keep Qt-facing wrapper thin
- if `QOpenGLWidget` proves unstable, switch the widget wrapper to a `QWindow` container without changing app/data layers

### Risk 2: Runtime dependency sprawl

Mitigation:

- centralize deployment logic in one CMake module
- validate startup from `install/` early

### Risk 3: Data-type scope explosion

Mitigation:

- first MVP supports imagery, elevation, and vector only
- chart and scientific data stay behind explicit follow-up plans

### Risk 4: Tight coupling between UI and render code

Mitigation:

- keep `SceneController` as the only render bridge
- keep `LayerManager` as the only layer state owner

## Self-review

Spec coverage check:

- architecture: covered by file structure, architecture rules, and Tasks 2-9
- platform skeleton: covered by Tasks 1-4
- imagery/elevation/vector MVP: covered by Tasks 5-7
- deployment approach with global vcpkg and local install copying: covered by Task 8
- deferred chart and NetCDF strategy: covered by Task 10

Placeholder scan:

- no `TODO`/`TBD` markers remain
- deferred work is explicitly moved into follow-up plan documents, not left vague inside code tasks

Type consistency check:

- `LayerManager`, `SceneController`, `ApplicationController`, `DataImporter`, `GlobeWidget`, and `MainWindow` names are used consistently throughout

## Immediate execution order

When implementation begins, execute tasks in this exact order:

1. Task 1
2. Task 2
3. Task 3
4. Task 4
5. Task 5
6. Task 6
7. Task 7
8. Task 8
9. Task 9
10. Task 10
