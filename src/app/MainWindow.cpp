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
#include <QStringLiteral>
#include <QToolBar>
#include <QUrl>

using namespace Qt::Literals::StringLiterals;

#include "globe/GlobeWidget.h"
#include "layers/Layer.h"
#include "tools/ToolManager.h"
#include "ui/IconManager.h"
#include "ui/LayerTreeDock.h"
#include "ui/PropertyDock.h"
#include "ui/StatusBarController.h"

namespace {
constexpr int kMaxRecentFiles = 5;
constexpr const char *kRecentFilesKey = "recentFiles";
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      globeWidget_(new GlobeWidget(this)),
      layerDock_(new LayerTreeDock(this)),
      propertyDock_(new PropertyDock(this)),
      statusController_(new StatusBarController(this)) {
    setWindowTitle(u"三维地球浏览器"_s);
    resize(1600, 900);
    setAcceptDrops(true);
    setCentralWidget(globeWidget_);
    addDockWidget(Qt::LeftDockWidgetArea, layerDock_);
    addDockWidget(Qt::RightDockWidgetArea, propertyDock_);

    auto &icons = IconManager::instance();
    const QColor toolColor("#4F637A");

    auto *fileMenu = menuBar()->addMenu(u"文件(&F)"_s);
    auto *importAction = fileMenu->addAction(icons.icon("cloud-arrow-up-regular.svg", 16, toolColor), u"导入数据..."_s);
    importAction->setShortcut(QKeySequence::Open);
    connect(importAction, &QAction::triggered, this, [this]() {
        const QString filter =
            u"影像数据 (*.tif *.tiff *.img *.asc *.srtm *.hgt *.dem *.vrt);;矢量数据 (*.shp *.geojson *.gpkg *.kml *.gml *.json);;所有文件 (*)"_s;
        const QString path = QFileDialog::getOpenFileName(this, u"导入数据"_s, QString(), filter);
        if (!path.isEmpty()) {
            emit importDataRequested(path);
        }
    });

    recentMenu_ = fileMenu->addMenu(u"最近打开"_s);
    QSettings settings;
    recentFiles_ = settings.value(kRecentFilesKey).toStringList();
    updateRecentMenu();

    fileMenu->addSeparator();
    auto *screenshotMenuAction = fileMenu->addAction(icons.icon("copy-regular.svg", 16, toolColor), u"截图保存..."_s);
    screenshotMenuAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_S);
    connect(screenshotMenuAction, &QAction::triggered, this, &MainWindow::screenshotRequested);

    auto *toolBar = addToolBar(u"工具"_s);
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(20, 20));

    toolGroup_ = new QActionGroup(this);
    panAction_ = toolBar->addAction(icons.icon("arrows-out-regular.svg", 20, toolColor), u"平移"_s);
    panAction_->setCheckable(true);
    panAction_->setChecked(true);
    panAction_->setActionGroup(toolGroup_);
    panAction_->setToolTip(u"平移工具 (1)"_s);

    pickAction_ = toolBar->addAction(icons.icon("crosshair-regular.svg", 20, toolColor), u"拾取"_s);
    pickAction_->setCheckable(true);
    pickAction_->setActionGroup(toolGroup_);
    pickAction_->setToolTip(u"拾取工具 (2)"_s);

    toolBar->addSeparator();

    homeAction_ = toolBar->addAction(icons.icon("arrows-clockwise-regular.svg", 20, toolColor), u"归位"_s);
    homeAction_->setToolTip(u"视图归位 (Ctrl+Shift+H)"_s);
    connect(homeAction_, &QAction::triggered, this, &MainWindow::resetViewRequested);

    screenshotAction_ = toolBar->addAction(icons.icon("copy-regular.svg", 20, toolColor), u"截图"_s);
    screenshotAction_->setToolTip(u"截图保存 (Ctrl+Shift+S)"_s);
    connect(screenshotAction_, &QAction::triggered, this, &MainWindow::screenshotRequested);

    connect(toolGroup_, &QActionGroup::triggered, this, [this](QAction *action) {
        if (action == panAction_) {
            emit toolChanged(static_cast<int>(ToolId::Pan));
        } else if (action == pickAction_) {
            emit toolChanged(static_cast<int>(ToolId::Pick));
        }
    });

    connect(layerDock_, &LayerTreeDock::layerSelected, this, &MainWindow::layerSelected);
    connect(layerDock_, &LayerTreeDock::layerVisibilityChanged, this, &MainWindow::layerVisibilityChanged);
    connect(layerDock_, &LayerTreeDock::layerOrderChanged, this, &MainWindow::layerOrderChanged);
    connect(layerDock_, &LayerTreeDock::removeLayerRequested, this, &MainWindow::removeLayerRequested);
    connect(layerDock_, &LayerTreeDock::zoomToLayerRequested, this, &MainWindow::zoomToLayerRequested);
    connect(propertyDock_, &PropertyDock::opacityChanged, this, &MainWindow::layerOpacityChanged);
    connect(propertyDock_, &PropertyDock::bandMappingChanged, this, &MainWindow::bandMappingChanged);
    connect(globeWidget_, &GlobeWidget::cursorTextChanged, statusController_, &StatusBarController::setCursorText);
    propertyDock_->showText(u"未选择图层。"_s);
}

GlobeWidget *MainWindow::globeWidget() const {
    return globeWidget_;
}

void MainWindow::addLayerRow(const Layer &layer) {
    layerDock_->addLayer(layer.id(), layer.name(), layer.visible(), layer.kind());
}

void MainWindow::removeLayerRow(const std::string &layerId) {
    layerDock_->removeLayer(layerId);
}

void MainWindow::showLayerDetails(const QString &text) {
    propertyDock_->showText(text);
}

void MainWindow::showLayerProperties(const QString &layerId, const QString &name, const QString &typeText,
                                     const QString &source, bool visible, double opacity,
                                     const std::optional<RasterMetadata> &rasterMeta,
                                     const std::optional<VectorLayerInfo> &vectorMeta) {
    propertyDock_->showLayerProperties(layerId, name, typeText, source, visible, opacity, rasterMeta, vectorMeta);
}

void MainWindow::clearLayerProperties() {
    propertyDock_->clearLayerProperties();
}

void MainWindow::setRecentFiles(const QStringList &files) {
    recentFiles_ = files;
    QSettings settings;
    settings.setValue(kRecentFilesKey, files);
    updateRecentMenu();
}

void MainWindow::updateRecentMenu() {
    recentMenu_->clear();
    if (recentFiles_.isEmpty()) {
        auto *emptyAction = recentMenu_->addAction(u"(无)"_s);
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
        emit toolChanged(static_cast<int>(ToolId::Pan));
        return;
    }
    if (event->key() == Qt::Key_2 && !event->modifiers()) {
        pickAction_->setChecked(true);
        emit toolChanged(static_cast<int>(ToolId::Pick));
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
