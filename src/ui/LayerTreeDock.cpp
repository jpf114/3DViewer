#include "ui/LayerTreeDock.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QMenu>
#include <QSignalBlocker>
#include <QStringLiteral>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "layers/LayerTypes.h"
#include "ui/IconManager.h"

using namespace Qt::Literals::StringLiterals;

LayerTreeDock::LayerTreeDock(QWidget *parent)
    : QDockWidget(u"图层"_s, parent), tree_(new QTreeWidget(this)) {
    tree_->setHeaderLabel(u"场景"_s);
    tree_->setDragDropMode(QAbstractItemView::InternalMove);
    tree_->setDefaultDropAction(Qt::MoveAction);
    tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_->setAnimated(true);
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_->setIndentation(12);
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
    connect(tree_, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
        if (item == nullptr) {
            return;
        }
        emit zoomToLayerRequested(item->data(0, Qt::UserRole).toString());
    });
    connect(tree_->model(), &QAbstractItemModel::rowsMoved, this, [this](const QModelIndex &, int, int, const QModelIndex &, int) {
        emitLayerOrderFromTree();
    });
    connect(tree_, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QTreeWidgetItem *item = tree_->itemAt(pos);
        if (item == nullptr) {
            return;
        }

        auto &icons = IconManager::instance();
        const QColor menuColor("#4F637A");
        QMenu menu;
        QAction *zoomAction = menu.addAction(icons.icon("magnifying-glass-plus-regular.svg", 16, menuColor), u"缩放到图层"_s);
        menu.addSeparator();
        QAction *removeAction = menu.addAction(icons.icon("minus-circle-regular.svg", 16, menuColor), u"移除图层"_s);
        QAction *chosen = menu.exec(tree_->mapToGlobal(pos));
        if (chosen == zoomAction) {
            emit zoomToLayerRequested(item->data(0, Qt::UserRole).toString());
        } else if (chosen == removeAction) {
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

void LayerTreeDock::addLayer(const std::string &id, const std::string &name, bool visible, LayerKind kind) {
    const QSignalBlocker blocker(tree_);
    auto &icons = IconManager::instance();
    const QColor layerColor("#2E7EC9");

    auto *item = new QTreeWidgetItem();
    item->setText(0, QString::fromUtf8(name.c_str(), static_cast<int>(name.size())));
    item->setData(0, Qt::UserRole, QString::fromUtf8(id.c_str(), static_cast<int>(id.size())));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(0, visible ? Qt::Checked : Qt::Unchecked);

    if (kind == LayerKind::Imagery) {
        item->setIcon(0, icons.icon("mountains-regular.svg", 16, layerColor));
    } else if (kind == LayerKind::Vector) {
        item->setIcon(0, icons.icon("polygon-regular.svg", 16, layerColor));
    }

    tree_->addTopLevelItem(item);
}

QString LayerTreeDock::currentLayerId() const {
    QTreeWidgetItem *current = tree_->currentItem();
    if (current == nullptr) {
        return QString();
    }
    return current->data(0, Qt::UserRole).toString();
}

QTreeWidget *LayerTreeDock::tree() const {
    return tree_;
}

void LayerTreeDock::removeLayer(const std::string &id) {
    const QString qId = QString::fromUtf8(id.c_str(), static_cast<int>(id.size()));
    for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = tree_->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == qId) {
            delete tree_->takeTopLevelItem(i);
            return;
        }
    }
}
