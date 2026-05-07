#include "ui/LayerTreeDock.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QMenu>
#include <QSignalBlocker>
#include <QTreeWidget>
#include <QTreeWidgetItem>

LayerTreeDock::LayerTreeDock(QWidget *parent)
    : QDockWidget("Layers", parent), tree_(new QTreeWidget(this)) {
    tree_->setHeaderLabel("Scene");
    tree_->setDragDropMode(QAbstractItemView::InternalMove);
    tree_->setDefaultDropAction(Qt::MoveAction);
    tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_->setAnimated(true);
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    setWidget(tree_);

    connect(tree_, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item, int) {
        emit layerVisibilityChanged(item->data(0, Qt::UserRole).toString(), item->checkState(0) == Qt::Checked);
    });
    connect(tree_, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *current, QTreeWidgetItem *) {
        if (current == nullptr) {
            return;
        }
        emit layerSelected(current->data(0, Qt::UserRole).toString());
    });
    connect(tree_->model(), &QAbstractItemModel::rowsMoved, this, [this](const QModelIndex &, int, int, const QModelIndex &, int) {
        emitLayerOrderFromTree();
    });
    connect(tree_, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QTreeWidgetItem *item = tree_->itemAt(pos);
        if (item == nullptr) {
            return;
        }

        QMenu menu;
        QAction *removeAction = menu.addAction("Remove Layer");
        QAction *chosen = menu.exec(tree_->mapToGlobal(pos));
        if (chosen == removeAction) {
            emit removeLayerRequested(item->data(0, Qt::UserRole).toString());
        }
    });
}

void LayerTreeDock::emitLayerOrderFromTree() {
    if (tree_->topLevelItemCount() == 0) {
        return;
    }

    QStringList ids;
    ids.reserve(tree_->topLevelItemCount());
    for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
        ids.append(tree_->topLevelItem(i)->data(0, Qt::UserRole).toString());
    }
    emit layerOrderChanged(ids);
}

void LayerTreeDock::addLayer(const std::string &id, const std::string &name, bool visible) {
    const QSignalBlocker blocker(tree_);
    auto *item = new QTreeWidgetItem();
    item->setText(0, QString::fromStdString(name));
    item->setData(0, Qt::UserRole, QString::fromStdString(id));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

    item->setCheckState(0, visible ? Qt::Checked : Qt::Unchecked);
    tree_->addTopLevelItem(item);
}

QTreeWidget *LayerTreeDock::tree() const {
    return tree_;
}

void LayerTreeDock::removeLayer(const std::string &id) {
    const QString qId = QString::fromStdString(id);
    for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = tree_->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == qId) {
            delete tree_->takeTopLevelItem(i);
            return;
        }
    }
}
