#include "ui/LayerTreeDock.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QMenu>
#include <QSignalBlocker>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "layers/LayerTypes.h"
#include "ui/IconManager.h"

namespace {

constexpr int kLayerIdRole = Qt::UserRole;
constexpr int kLayerKindRole = Qt::UserRole + 1;
constexpr int kPlaceholderRole = Qt::UserRole + 2;

QString placeholderText() {
    return QString::fromUtf8(u8"暂无图层，可拖拽文件到窗口或使用“导入数据”。");
}

} // namespace

LayerTreeDock::LayerTreeDock(QWidget *parent)
    : QDockWidget(QString::fromUtf8(u8"图层"), parent), tree_(new QTreeWidget(this)) {
    tree_->setHeaderLabel(QString::fromUtf8(u8"场景"));
    tree_->setDragDropMode(QAbstractItemView::InternalMove);
    tree_->setDefaultDropAction(Qt::MoveAction);
    tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_->setAnimated(true);
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_->setIndentation(12);
    setWidget(tree_);

    connect(tree_, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item, int) {
        if (item == nullptr || isPlaceholderItem(item)) {
            return;
        }
        emit layerVisibilityChanged(item->data(0, kLayerIdRole).toString(), item->checkState(0) == Qt::Checked);
    });
    connect(tree_, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *current, QTreeWidgetItem *) {
        if (current == nullptr || isPlaceholderItem(current)) {
            return;
        }
        emit layerSelected(current->data(0, kLayerIdRole).toString());
    });
    connect(tree_, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
        if (item == nullptr || isPlaceholderItem(item)) {
            return;
        }
        emit zoomToLayerRequested(item->data(0, kLayerIdRole).toString());
    });
    connect(tree_->model(), &QAbstractItemModel::rowsMoved, this, [this](const QModelIndex &, int, int, const QModelIndex &, int) {
        emitLayerOrderFromTree();
    });
    connect(tree_, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QTreeWidgetItem *item = tree_->itemAt(pos);
        if (item == nullptr || isPlaceholderItem(item)) {
            return;
        }

        tree_->setCurrentItem(item);
        const QString layerId = item->data(0, kLayerIdRole).toString();
        const auto kind = static_cast<LayerKind>(item->data(0, kLayerKindRole).toInt());

        auto &icons = IconManager::instance();
        const QColor menuColor("#4F637A");
        QMenu menu;
        QAction *zoomAction = menu.addAction(
            icons.icon("magnifying-glass-plus-regular.svg", 16, menuColor),
            QString::fromUtf8(u8"缩放到图层"));
        QAction *editMeasurementAction = nullptr;
        if (kind == LayerKind::Measurement) {
            editMeasurementAction = menu.addAction(
                icons.icon("ruler-regular.svg", 16, menuColor),
                QString::fromUtf8(u8"编辑量测"));
            menu.addSeparator();
        } else {
            menu.addSeparator();
        }
        QAction *removeAction = menu.addAction(
            icons.icon("minus-circle-regular.svg", 16, menuColor),
            QString::fromUtf8(u8"移除图层"));

        QAction *chosen = menu.exec(tree_->mapToGlobal(pos));
        if (chosen == zoomAction) {
            emit zoomToLayerRequested(layerId);
        } else if (chosen == editMeasurementAction) {
            emit editMeasurementRequested(layerId);
        } else if (chosen == removeAction) {
            emit removeLayerRequested(layerId);
        }
    });

    ensurePlaceholderItem();
}

void LayerTreeDock::emitLayerOrderFromTree() {
    if (tree_->topLevelItemCount() == 0) {
        return;
    }

    QStringList ids;
    ids.reserve(tree_->topLevelItemCount());
    for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = tree_->topLevelItem(i);
        if (isPlaceholderItem(item)) {
            continue;
        }
        ids.append(item->data(0, kLayerIdRole).toString());
    }
    if (!ids.isEmpty()) {
        emit layerOrderChanged(ids);
    }
}

void LayerTreeDock::addLayer(const std::string &id, const std::string &name, bool visible, LayerKind kind) {
    const QSignalBlocker blocker(tree_);
    clearPlaceholderItem();

    auto &icons = IconManager::instance();
    const QColor layerColor("#2E7EC9");

    auto *item = new QTreeWidgetItem();
    item->setText(0, QString::fromUtf8(name.c_str(), static_cast<int>(name.size())));
    item->setData(0, kLayerIdRole, QString::fromUtf8(id.c_str(), static_cast<int>(id.size())));
    item->setData(0, kLayerKindRole, static_cast<int>(kind));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(0, visible ? Qt::Checked : Qt::Unchecked);

    if (kind == LayerKind::Imagery) {
        item->setIcon(0, icons.icon("mountains-regular.svg", 16, layerColor));
    } else if (kind == LayerKind::Vector) {
        item->setIcon(0, icons.icon("polygon-regular.svg", 16, layerColor));
    } else if (kind == LayerKind::Measurement) {
        item->setIcon(0, icons.icon("ruler-regular.svg", 16, layerColor));
    } else if (kind == LayerKind::Model) {
        item->setIcon(0, icons.icon("stack-regular.svg", 16, layerColor));
    }

    tree_->addTopLevelItem(item);
}

QString LayerTreeDock::currentLayerId() const {
    QTreeWidgetItem *current = tree_->currentItem();
    if (current == nullptr || isPlaceholderItem(current)) {
        return QString();
    }
    return current->data(0, kLayerIdRole).toString();
}

QTreeWidget *LayerTreeDock::tree() const {
    return tree_;
}

void LayerTreeDock::removeLayer(const std::string &id) {
    const QString qId = QString::fromUtf8(id.c_str(), static_cast<int>(id.size()));
    for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = tree_->topLevelItem(i);
        if (item->data(0, kLayerIdRole).toString() == qId) {
            delete tree_->takeTopLevelItem(i);
            break;
        }
    }
    ensurePlaceholderItem();
}

void LayerTreeDock::selectLayer(const std::string &id) {
    const QString qId = QString::fromUtf8(id.c_str(), static_cast<int>(id.size()));
    for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = tree_->topLevelItem(i);
        if (item->data(0, kLayerIdRole).toString() == qId) {
            tree_->setCurrentItem(item);
            tree_->scrollToItem(item);
            return;
        }
    }
}

void LayerTreeDock::clearSelection() {
    const QSignalBlocker blocker(tree_);
    tree_->setCurrentItem(nullptr);
    tree_->clearSelection();
}

void LayerTreeDock::ensurePlaceholderItem() {
    for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
        if (!isPlaceholderItem(tree_->topLevelItem(i))) {
            return;
        }
    }

    const QSignalBlocker blocker(tree_);
    auto *placeholder = new QTreeWidgetItem();
    placeholder->setText(0, placeholderText());
    placeholder->setData(0, kPlaceholderRole, true);
    placeholder->setFlags(Qt::ItemIsEnabled);
    tree_->addTopLevelItem(placeholder);
}

void LayerTreeDock::clearPlaceholderItem() {
    for (int i = tree_->topLevelItemCount() - 1; i >= 0; --i) {
        QTreeWidgetItem *item = tree_->topLevelItem(i);
        if (isPlaceholderItem(item)) {
            delete tree_->takeTopLevelItem(i);
        }
    }
}

bool LayerTreeDock::isPlaceholderItem(QTreeWidgetItem *item) const {
    return item != nullptr && item->data(0, kPlaceholderRole).toBool();
}
