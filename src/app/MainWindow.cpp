#include "app/MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMimeData>
#include <QSettings>
#include <QToolBar>
#include <QUrl>

#include "data/DataImporter.h"
#include "globe/GlobeWidget.h"
#include "layers/Layer.h"
#include "tools/ToolManager.h"
#include "ui/IconManager.h"
#include "ui/LayerTreeDock.h"
#include "ui/MeasurementResultsDock.h"
#include "ui/PropertyDock.h"
#include "ui/StatusBarController.h"

namespace {

constexpr int kMaxRecentFiles = 5;
constexpr const char *kRecentFilesKey = "recentFiles";

QStringList sanitizeRecentFiles(const QStringList &files) {
    QStringList sanitized;
    for (const QString &file : files) {
        if (file.isEmpty() || sanitized.contains(file)) {
            continue;
        }
        sanitized.append(file);
        if (sanitized.size() >= kMaxRecentFiles) {
            break;
        }
    }
    return sanitized;
}

QString modelFilterText() {
    QStringList patterns;
    auto extensions = DataImporter::availableRuntimeModelExtensions();
    if (extensions.empty()) {
        extensions = DataImporter::supportedModelExtensions();
    }
    for (const auto &extension : extensions) {
        patterns.append(QString("*%1").arg(QString::fromStdString(extension)));
    }
    return QString::fromUtf8(u8"三维模型 (%1)").arg(patterns.join(' '));
}

bool splitKeyValue(const QString &line, QString *key, QString *value) {
    int separator = line.indexOf(':');
    if (separator <= 0) {
        separator = line.indexOf(QChar(0xFF1A));
    }
    if (separator <= 0) {
        return false;
    }

    *key = line.left(separator).trimmed();
    *value = line.mid(separator + 1).trimmed();
    return true;
}

bool isStructuredPickSummaryKey(const QString &key) {
    return key == QString::fromUtf8(u8"经度") ||
           key == QString::fromUtf8(u8"纬度") ||
           key == QString::fromUtf8(u8"高程(近似)") ||
           key == QString::fromUtf8(u8"图层");
}

void showMeasureHint(PropertyDock *propertyDock, StatusBarController *statusController) {
    propertyDock->showText(QString::fromUtf8(
        u8"测距：左键添加点，右键保留结果，Backspace 撤销最后一点，Esc 或工具栏清空。"));
    statusController->setMeasurementText(QString::fromUtf8(u8"测距：未开始"));
}

void showMeasureAreaHint(PropertyDock *propertyDock, StatusBarController *statusController) {
    propertyDock->showText(QString::fromUtf8(
        u8"测面：左键添加点，右键保留结果，Backspace 撤销最后一点，Esc 或工具栏清空。"));
    statusController->setMeasurementText(QString::fromUtf8(u8"测面：未开始"));
}

void showMeasurementIdle(StatusBarController *statusController) {
    statusController->setMeasurementText(QString::fromUtf8(u8"量测：未激活"));
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      globeWidget_(new GlobeWidget(this)),
      layerDock_(new LayerTreeDock(this)),
      measurementResultsDock_(new MeasurementResultsDock(this)),
      propertyDock_(new PropertyDock(this)),
      statusController_(new StatusBarController(this)) {
    setWindowTitle(QString::fromUtf8(u8"三维地球浏览器"));
    resize(1600, 900);
    setAcceptDrops(true);
    setCentralWidget(globeWidget_);
    addDockWidget(Qt::LeftDockWidgetArea, layerDock_);
    addDockWidget(Qt::RightDockWidgetArea, measurementResultsDock_);
    addDockWidget(Qt::RightDockWidgetArea, propertyDock_);

    auto &icons = IconManager::instance();
    const QColor toolColor("#4F637A");

    auto *fileMenu = menuBar()->addMenu(QString::fromUtf8(u8"文件(&F)"));
    auto *openProjectAction = fileMenu->addAction(
        icons.icon("folder-open-regular.svg", 16, toolColor),
        QString::fromUtf8(u8"打开工程..."));
    openProjectAction->setObjectName("openProjectAction");
    connect(openProjectAction, &QAction::triggered, this, &MainWindow::openProjectRequested);

    auto *saveProjectAction = fileMenu->addAction(
        icons.icon("floppy-disk-regular.svg", 16, toolColor),
        QString::fromUtf8(u8"保存工程"));
    saveProjectAction->setObjectName("saveProjectAction");
    saveProjectAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    connect(saveProjectAction, &QAction::triggered, this, &MainWindow::saveProjectRequested);

    auto *saveProjectAsAction = fileMenu->addAction(
        icons.icon("floppy-disk-back-regular.svg", 16, toolColor),
        QString::fromUtf8(u8"工程另存为..."));
    saveProjectAsAction->setObjectName("saveProjectAsAction");
    connect(saveProjectAsAction, &QAction::triggered, this, &MainWindow::saveProjectAsRequested);

    fileMenu->addSeparator();
    auto *importAction = fileMenu->addAction(
        icons.icon("cloud-arrow-up-regular.svg", 16, toolColor),
        QString::fromUtf8(u8"导入数据..."));
    importAction->setShortcut(QKeySequence::Open);
    connect(importAction, &QAction::triggered, this, [this]() {
        const QString filter =
            QString::fromUtf8(
                u8"影像数据 (*.tif *.tiff *.img *.asc *.srtm *.hgt *.dem *.vrt);;"
                u8"矢量数据 (*.shp *.geojson *.gpkg *.kml *.gml *.json);;%1;;所有文件(*)")
                .arg(modelFilterText());
        const QString path = QFileDialog::getOpenFileName(
            this,
            QString::fromUtf8(u8"导入数据"),
            QString(),
            filter);
        if (!path.isEmpty()) {
            emit importDataRequested(path);
        }
    });

    recentMenu_ = fileMenu->addMenu(QString::fromUtf8(u8"最近打开"));
    QSettings settings;
    recentFiles_ = sanitizeRecentFiles(settings.value(kRecentFilesKey).toStringList());
    settings.setValue(kRecentFilesKey, recentFiles_);
    updateRecentMenu();

    fileMenu->addSeparator();
    auto *screenshotMenuAction = fileMenu->addAction(
        icons.icon("copy-regular.svg", 16, toolColor),
        QString::fromUtf8(u8"截图保存..."));
    screenshotMenuAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_S);
    connect(screenshotMenuAction, &QAction::triggered, this, &MainWindow::screenshotRequested);

    auto *measurementMenu = menuBar()->addMenu(QString::fromUtf8(u8"量测(&M)"));
    auto *measurementEditAction = measurementMenu->addAction(
        icons.icon("ruler-regular.svg", 16, toolColor),
        QString::fromUtf8(u8"编辑当前量测"));
    connect(measurementEditAction, &QAction::triggered, this, &MainWindow::editSelectedMeasurementRequested);

    auto *measurementExportAction = measurementMenu->addAction(
        icons.icon("export-regular.svg", 16, toolColor),
        QString::fromUtf8(u8"导出当前量测"));
    connect(measurementExportAction, &QAction::triggered, this, &MainWindow::exportSelectedMeasurementRequested);

    deleteSelectedMeasurementAction_ = measurementMenu->addAction(
        icons.icon("minus-circle-regular.svg", 16, toolColor),
        QString::fromUtf8(u8"删除当前量测"));
    deleteSelectedMeasurementAction_->setObjectName("deleteSelectedMeasurementAction");
    connect(deleteSelectedMeasurementAction_, &QAction::triggered, this, [this]() {
        const QString layerId = currentLayerId();
        if (!layerId.isEmpty() && selectedMeasurementLayer_) {
            emit removeLayerRequested(layerId);
        }
    });

    measurementMenu->addSeparator();
    clearAllMeasurementsAction_ = measurementMenu->addAction(
        icons.icon("eraser-regular.svg", 16, toolColor),
        QString::fromUtf8(u8"清空全部量测成果"));
    clearAllMeasurementsAction_->setObjectName("clearAllMeasurementsAction");
    connect(clearAllMeasurementsAction_, &QAction::triggered, this, &MainWindow::clearAllMeasurementsRequested);

    auto *toolBar = addToolBar(QString::fromUtf8(u8"工具"));
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(20, 20));

    toolGroup_ = new QActionGroup(this);
    panAction_ = toolBar->addAction(
        icons.icon("arrows-out-regular.svg", 20, toolColor),
        QString::fromUtf8(u8"平移"));
    panAction_->setCheckable(true);
    panAction_->setChecked(true);
    panAction_->setActionGroup(toolGroup_);
    panAction_->setToolTip(QString::fromUtf8(u8"平移工具 (1)"));

    pickAction_ = toolBar->addAction(
        icons.icon("crosshair-regular.svg", 20, toolColor),
        QString::fromUtf8(u8"拾取"));
    pickAction_->setCheckable(true);
    pickAction_->setActionGroup(toolGroup_);
    pickAction_->setToolTip(QString::fromUtf8(u8"拾取工具 (2)"));

    measureAction_ = toolBar->addAction(
        icons.icon("ruler-regular.svg", 20, toolColor),
        QString::fromUtf8(u8"测距"));
    measureAction_->setCheckable(true);
    measureAction_->setActionGroup(toolGroup_);
    measureAction_->setToolTip(QString::fromUtf8(u8"测距工具 (3)"));

    measureAreaAction_ = toolBar->addAction(
        icons.icon("polygon-regular.svg", 20, toolColor),
        QString::fromUtf8(u8"测面"));
    measureAreaAction_->setCheckable(true);
    measureAreaAction_->setActionGroup(toolGroup_);
    measureAreaAction_->setToolTip(QString::fromUtf8(u8"测面工具 (4)"));

    editMeasureAction_ = toolBar->addAction(
        icons.icon("ruler-regular.svg", 20, toolColor),
        QString::fromUtf8(u8"编辑量测"));
    editMeasureAction_->setObjectName("editMeasureAction");
    exportMeasureAction_ = toolBar->addAction(
        icons.icon("export-regular.svg", 20, toolColor),
        QString::fromUtf8(u8"瀵煎嚭缁撴灉"));
    exportMeasureAction_->setObjectName("exportMeasureAction");
    exportMeasureAction_->setToolTip(QString::fromUtf8(u8"瀵煎嚭褰撳墠閫変腑鐨勯噺娴嬬粨鏋滀负 GeoJSON"));
    editMeasureAction_->setToolTip(QString::fromUtf8(u8"继续编辑当前选中的量测结果"));

    undoMeasureAction_ = toolBar->addAction(
        icons.icon("arrow-bend-up-right-regular.svg", 20, toolColor),
        QString::fromUtf8(u8"撤销量测点"));
    undoMeasureAction_->setObjectName("undoMeasureAction");
    undoMeasureAction_->setToolTip(QString::fromUtf8(u8"撤销当前量测的最后一个点 (Backspace)"));

    clearMeasureAction_ = toolBar->addAction(
        icons.icon("eraser-regular.svg", 20, toolColor),
        QString::fromUtf8(u8"清空量测"));
    clearMeasureAction_->setObjectName("clearMeasureAction");
    clearMeasureAction_->setToolTip(QString::fromUtf8(u8"清空当前量测结果 (Esc)"));

    toolBar->addSeparator();

    homeAction_ = toolBar->addAction(
        icons.icon("arrows-clockwise-regular.svg", 20, toolColor),
        QString::fromUtf8(u8"归位"));
    homeAction_->setToolTip(QString::fromUtf8(u8"视图归位 (Ctrl+Shift+H)"));
    connect(homeAction_, &QAction::triggered, this, &MainWindow::resetViewRequested);

    screenshotAction_ = toolBar->addAction(
        icons.icon("copy-regular.svg", 20, toolColor),
        QString::fromUtf8(u8"截图"));
    screenshotAction_->setToolTip(QString::fromUtf8(u8"截图保存 (Ctrl+Shift+S)"));
    connect(screenshotAction_, &QAction::triggered, this, &MainWindow::screenshotRequested);

    connect(toolGroup_, &QActionGroup::triggered, this, [this](QAction *action) {
        if (action == panAction_) {
            currentToolId_ = static_cast<int>(ToolId::Pan);
            emit toolChanged(static_cast<int>(ToolId::Pan));
            showMeasurementIdle(statusController_);
        } else if (action == pickAction_) {
            currentToolId_ = static_cast<int>(ToolId::Pick);
            emit toolChanged(static_cast<int>(ToolId::Pick));
            showMeasurementIdle(statusController_);
        } else if (action == measureAction_) {
            currentToolId_ = static_cast<int>(ToolId::Measure);
            emit toolChanged(static_cast<int>(ToolId::Measure));
            showMeasureHint(propertyDock_, statusController_);
        } else if (action == measureAreaAction_) {
            currentToolId_ = static_cast<int>(ToolId::MeasureArea);
            emit toolChanged(static_cast<int>(ToolId::MeasureArea));
            showMeasureAreaHint(propertyDock_, statusController_);
        }
        refreshToolActionStates();
    });

    connect(layerDock_, &LayerTreeDock::layerSelected, this, &MainWindow::layerSelected);
    connect(layerDock_, &LayerTreeDock::layerRenameRequested, this, &MainWindow::layerRenameRequested);
    connect(layerDock_, &LayerTreeDock::layerVisibilityChanged, this, &MainWindow::layerVisibilityChanged);
    connect(layerDock_, &LayerTreeDock::layerOrderChanged, this, &MainWindow::layerOrderChanged);
    connect(layerDock_, &LayerTreeDock::removeLayerRequested, this, &MainWindow::removeLayerRequested);
    connect(layerDock_, &LayerTreeDock::zoomToLayerRequested, this, &MainWindow::zoomToLayerRequested);
    connect(layerDock_, &LayerTreeDock::editMeasurementRequested, this, [this](const QString &) {
        if (editMeasureAction_ != nullptr && editMeasureAction_->isEnabled()) {
            emit editSelectedMeasurementRequested();
        }
    });
    connect(layerDock_, &LayerTreeDock::exportMeasurementRequested, this, [this](const QString &) {
        if (exportMeasureAction_ != nullptr && exportMeasureAction_->isEnabled()) {
            emit exportSelectedMeasurementRequested();
        }
    });
    connect(measurementResultsDock_, &MeasurementResultsDock::removeRequested, this, [this](const QString &layerId) {
        if (!layerId.isEmpty()) {
            emit removeLayerRequested(layerId);
        }
    });
    connect(measurementResultsDock_, &MeasurementResultsDock::zoomRequested, this, [this](const QString &layerId) {
        if (!layerId.isEmpty()) {
            selectLayerRow(layerId.toStdString());
            emit zoomToLayerRequested(layerId);
        }
    });
    connect(measurementResultsDock_, &MeasurementResultsDock::editRequested, this, [this](const QString &layerId) {
        if (!layerId.isEmpty()) {
            selectLayerRow(layerId.toStdString());
            emit editSelectedMeasurementRequested();
        }
    });
    connect(measurementResultsDock_, &MeasurementResultsDock::bulkRemoveRequested, this, [this](const QStringList &layerIds) {
        if (!layerIds.isEmpty()) {
            emit removeMeasurementResultsRequested(layerIds);
        }
    });
    connect(measurementResultsDock_, &MeasurementResultsDock::bulkExportRequested, this, [this](const QStringList &layerIds) {
        if (!layerIds.isEmpty()) {
            emit exportMeasurementResultsRequested(layerIds);
        }
    });
    connect(measurementResultsDock_, &MeasurementResultsDock::currentResultChanged, this, [this](const QString &layerId) {
        if (!layerId.isEmpty()) {
            selectLayerRow(layerId.toStdString());
        }
    });
    connect(propertyDock_, &PropertyDock::opacityChanged, this, &MainWindow::layerOpacityChanged);
    connect(propertyDock_, &PropertyDock::bandMappingChanged, this, &MainWindow::bandMappingChanged);
    connect(propertyDock_, &PropertyDock::modelPlacementChanged, this, &MainWindow::modelPlacementChanged);
    connect(globeWidget_, &GlobeWidget::cursorTextChanged, statusController_, &StatusBarController::setCursorText);
    connect(globeWidget_, &GlobeWidget::measurementTextChanged, this, [this](const QString &text) {
        propertyDock_->showText(text);
    });
    connect(globeWidget_, &GlobeWidget::measurementStatusChanged, statusController_, &StatusBarController::setMeasurementText);
    connect(editMeasureAction_, &QAction::triggered, this, &MainWindow::editSelectedMeasurementRequested);
    connect(exportMeasureAction_, &QAction::triggered, this, &MainWindow::exportSelectedMeasurementRequested);
    connect(undoMeasureAction_, &QAction::triggered, this, &MainWindow::undoMeasurementRequested);
    connect(clearMeasureAction_, &QAction::triggered, this, [this]() {
        globeWidget_->toolManager().clearActiveToolState(*globeWidget_);
    });
    propertyDock_->showText(QString::fromUtf8(u8"未选择图层。"));
    refreshToolActionStates();
}

GlobeWidget *MainWindow::globeWidget() const {
    return globeWidget_;
}

void MainWindow::addLayerRow(const Layer &layer) {
    layerDock_->addLayer(layer.id(), layer.name(), layer.visible(), layer.kind());
}

void MainWindow::renameLayerRow(const std::string &layerId, const std::string &name) {
    layerDock_->renameLayer(layerId, name);
}

void MainWindow::removeLayerRow(const std::string &layerId) {
    layerDock_->removeLayer(layerId);
}

void MainWindow::selectLayerRow(const std::string &layerId) {
    layerDock_->selectLayer(layerId);
}

void MainWindow::clearLayerSelection() {
    layerDock_->clearSelection();
}

void MainWindow::addOrUpdateMeasurementResultRow(const QString &layerId, const QString &name,
                                                 MeasurementKind kind, const QString &summary) {
    measurementResultsDock_->addOrUpdateResult(MeasurementResultItemData{layerId, name, kind, summary});
}

void MainWindow::removeMeasurementResultRow(const QString &layerId) {
    measurementResultsDock_->removeResult(layerId);
}

void MainWindow::selectMeasurementResultRow(const QString &layerId) {
    measurementResultsDock_->selectResult(layerId);
}

void MainWindow::clearMeasurementResultSelection() {
    measurementResultsDock_->clearResultSelection();
}

QString MainWindow::currentLayerId() const {
    return layerDock_->currentLayerId();
}

QString MainWindow::currentMeasurementResultId() const {
    return measurementResultsDock_->currentResultId();
}

void MainWindow::setActiveToolAction(int toolId) {
    currentToolId_ = toolId;
    const ToolId resolved = static_cast<ToolId>(toolId);
    switch (resolved) {
    case ToolId::Pick:
        pickAction_->setChecked(true);
        break;
    case ToolId::Measure:
        measureAction_->setChecked(true);
        break;
    case ToolId::MeasureArea:
        measureAreaAction_->setChecked(true);
        break;
    case ToolId::Pan:
    default:
        panAction_->setChecked(true);
        break;
    }
    refreshToolActionStates();
}

void MainWindow::showLayerDetails(const QString &text) {
    selectedMeasurementLayer_ = false;
    clearMeasurementResultSelection();
    refreshToolActionStates();

    const QStringList lines = text.split('\n');
    QStringList summaryLines;
    QList<std::pair<QString, QString>> attributes;
    bool hasStructuredPick = false;
    bool hasPickSummaryFields = false;

    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith("---")) {
            continue;
        }

        QString key;
        QString value;
        if (!splitKeyValue(line, &key, &value)) {
            continue;
        }

        if (rawLine.startsWith("  ")) {
            attributes.append({key, value});
            hasStructuredPick = true;
            continue;
        }

        summaryLines.append(key + ": " + value);
        hasPickSummaryFields = hasPickSummaryFields || isStructuredPickSummaryKey(key);
        hasStructuredPick = true;
    }

    if (hasStructuredPick && !summaryLines.isEmpty() && (hasPickSummaryFields || !attributes.isEmpty())) {
        propertyDock_->showPickDetails(summaryLines, attributes);
        return;
    }

    propertyDock_->showText(text);
}

void MainWindow::showPickDetails(const QStringList &summaryLines,
                                 const QList<std::pair<QString, QString>> &attributes) {
    selectedMeasurementLayer_ = false;
    clearMeasurementResultSelection();
    refreshToolActionStates();
    propertyDock_->showPickDetails(summaryLines, attributes);
}

void MainWindow::showLayerProperties(const QString &layerId, const QString &name, const QString &typeText,
                                     const QString &source, bool visible, double opacity,
                                     const std::optional<RasterMetadata> &rasterMeta,
                                     const std::optional<VectorLayerInfo> &vectorMeta,
                                     const std::optional<ModelPlacement> &modelPlacement,
                                     const std::optional<MeasurementLayerData> &measurementData) {
    selectedMeasurementLayer_ = measurementData.has_value();
    if (measurementData.has_value()) {
        selectMeasurementResultRow(layerId);
    } else {
        clearMeasurementResultSelection();
    }
    refreshToolActionStates();
    propertyDock_->showLayerProperties(
        layerId, name, typeText, source, visible, opacity, rasterMeta, vectorMeta, modelPlacement, measurementData);
}

void MainWindow::clearLayerProperties() {
    selectedMeasurementLayer_ = false;
    refreshToolActionStates();
    propertyDock_->clearLayerProperties();
}

void MainWindow::refreshToolActionStates() {
    const bool measurementToolActive =
        currentToolId_ == static_cast<int>(ToolId::Measure) ||
        currentToolId_ == static_cast<int>(ToolId::MeasureArea);

    if (editMeasureAction_ != nullptr) {
        editMeasureAction_->setEnabled(selectedMeasurementLayer_);
    }
    if (exportMeasureAction_ != nullptr) {
        exportMeasureAction_->setEnabled(selectedMeasurementLayer_);
    }
    if (deleteSelectedMeasurementAction_ != nullptr) {
        deleteSelectedMeasurementAction_->setEnabled(selectedMeasurementLayer_);
    }
    if (undoMeasureAction_ != nullptr) {
        undoMeasureAction_->setEnabled(measurementToolActive);
    }
    if (clearMeasureAction_ != nullptr) {
        clearMeasureAction_->setEnabled(measurementToolActive);
    }
}

void MainWindow::setRecentFiles(const QStringList &files) {
    recentFiles_ = sanitizeRecentFiles(files);
    QSettings settings;
    settings.setValue(kRecentFilesKey, recentFiles_);
    updateRecentMenu();
}

void MainWindow::updateRecentMenu() {
    recentMenu_->clear();
    if (recentFiles_.isEmpty()) {
        auto *emptyAction = recentMenu_->addAction(QString::fromUtf8(u8"(无)"));
        emptyAction->setEnabled(false);
        return;
    }
    for (int i = 0; i < recentFiles_.size(); ++i) {
        const QString &path = recentFiles_[i];
        auto *action = recentMenu_->addAction(QString("&%1 %2").arg(i + 1).arg(path));
        connect(action, &QAction::triggered, this, [this, path]() {
            emit importDataRequested(path);
        });
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            emit importDataRequested(url.toLocalFile());
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_1 && !event->modifiers()) {
        panAction_->setChecked(true);
        currentToolId_ = static_cast<int>(ToolId::Pan);
        emit toolChanged(static_cast<int>(ToolId::Pan));
        showMeasurementIdle(statusController_);
        refreshToolActionStates();
        return;
    }
    if (event->key() == Qt::Key_2 && !event->modifiers()) {
        pickAction_->setChecked(true);
        currentToolId_ = static_cast<int>(ToolId::Pick);
        emit toolChanged(static_cast<int>(ToolId::Pick));
        showMeasurementIdle(statusController_);
        refreshToolActionStates();
        return;
    }
    if (event->key() == Qt::Key_3 && !event->modifiers()) {
        measureAction_->setChecked(true);
        currentToolId_ = static_cast<int>(ToolId::Measure);
        emit toolChanged(static_cast<int>(ToolId::Measure));
        showMeasureHint(propertyDock_, statusController_);
        refreshToolActionStates();
        return;
    }
    if (event->key() == Qt::Key_4 && !event->modifiers()) {
        measureAreaAction_->setChecked(true);
        currentToolId_ = static_cast<int>(ToolId::MeasureArea);
        emit toolChanged(static_cast<int>(ToolId::MeasureArea));
        showMeasureAreaHint(propertyDock_, statusController_);
        refreshToolActionStates();
        return;
    }
    if (event->key() == Qt::Key_Escape && !event->modifiers()) {
        globeWidget_->toolManager().clearActiveToolState(*globeWidget_);
        return;
    }
    if (event->key() == Qt::Key_Backspace && !event->modifiers()) {
        emit undoMeasurementRequested();
        return;
    }
    if (event->key() == Qt::Key_Delete && !event->modifiers()) {
        const QString layerId = layerDock_->currentLayerId();
        if (!layerId.isEmpty()) {
            emit removeLayerRequested(layerId);
        }
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    emit saveAndExitRequested();
    event->accept();
}
