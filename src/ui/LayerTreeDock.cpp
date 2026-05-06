#include "ui/LayerTreeDock.h"

#include <QSignalBlocker>
#include <QTreeWidget>
#include <QTreeWidgetItem>

LayerTreeDock::LayerTreeDock(QWidget *parent)
    : QDockWidget("Layers", parent), tree_(new QTreeWidget(this)) {
    tree_->setHeaderLabel("Scene");
    setWidget(tree_);

    connect(tree_, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item, int) {
        emit layerVisibilityChanged(item->data(0, Qt::UserRole).toString(), item->checkState(0) == Qt::Checked);
    });
}

void LayerTreeDock::addLayer(const std::string &id, const std::string &name, bool visible) {
    auto *item = new QTreeWidgetItem(tree_);
    item->setText(0, QString::fromStdString(name));
    item->setData(0, Qt::UserRole, QString::fromStdString(id));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

    const QSignalBlocker blocker(tree_);
    item->setCheckState(0, visible ? Qt::Checked : Qt::Unchecked);
    tree_->addTopLevelItem(item);
}

QTreeWidget *LayerTreeDock::tree() const {
    return tree_;
}
