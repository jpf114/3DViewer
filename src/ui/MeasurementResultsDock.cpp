#include "ui/MeasurementResultsDock.h"

#include <QAction>
#include <QHeaderView>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include "layers/MeasurementLayerData.h"
#include "ui/IconManager.h"

namespace {

constexpr int kLayerIdRole = Qt::UserRole;

QString measurementKindText(MeasurementKind kind) {
    return kind == MeasurementKind::Area
        ? QString::fromUtf8(u8"测面")
        : QString::fromUtf8(u8"测距");
}

} // namespace

MeasurementResultsDock::MeasurementResultsDock(QWidget *parent)
    : QDockWidget(QString::fromUtf8(u8"量测成果"), parent),
      table_(new QTableWidget(this)),
      zoomAction_(nullptr),
      editAction_(nullptr),
      deleteAction_(nullptr),
      bulkDeleteAction_(nullptr),
      bulkExportAction_(nullptr) {
    setObjectName("measurementResultsDock");

    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto &icons = IconManager::instance();
    const QColor toolColor("#4F637A");
    auto *toolbar = new QToolBar(container);
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(16, 16));

    zoomAction_ = toolbar->addAction(
        icons.icon("magnifying-glass-plus-regular.svg", 16, toolColor),
        QString::fromUtf8(u8"定位"));
    zoomAction_->setObjectName("measurementResultsZoomAction");

    editAction_ = toolbar->addAction(
        icons.icon("ruler-regular.svg", 16, toolColor),
        QString::fromUtf8(u8"编辑"));
    editAction_->setObjectName("measurementResultsEditAction");

    deleteAction_ = toolbar->addAction(
        icons.icon("minus-circle-regular.svg", 16, toolColor),
        QString::fromUtf8(u8"删除"));
    deleteAction_->setObjectName("measurementResultsDeleteAction");

    toolbar->addSeparator();

    bulkDeleteAction_ = toolbar->addAction(
        icons.icon("trash-regular.svg", 16, toolColor),
        QString::fromUtf8(u8"批量删除"));
    bulkDeleteAction_->setObjectName("measurementResultsBulkDeleteAction");

    bulkExportAction_ = toolbar->addAction(
        icons.icon("export-regular.svg", 16, toolColor),
        QString::fromUtf8(u8"批量导出"));
    bulkExportAction_->setObjectName("measurementResultsBulkExportAction");

    for (QAction *action : {zoomAction_, editAction_, deleteAction_, bulkDeleteAction_, bulkExportAction_}) {
        action->setEnabled(false);
    }

    table_->setObjectName("measurementResultsTable");
    table_->setColumnCount(3);
    table_->setHorizontalHeaderLabels({
        QString::fromUtf8(u8"名称"),
        QString::fromUtf8(u8"类型"),
        QString::fromUtf8(u8"摘要")
    });
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->setSortingEnabled(false);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    connect(table_, &QTableWidget::itemSelectionChanged, this, [this]() {
        updateActionStates();
    });
    connect(deleteAction_, &QAction::triggered, this, [this]() {
        const QString layerId = currentResultId();
        if (!layerId.isEmpty()) {
            emit removeRequested(layerId);
        }
    });
    connect(bulkDeleteAction_, &QAction::triggered, this, [this]() {
        const QStringList layerIds = selectedResultIds();
        if (!layerIds.isEmpty()) {
            emit bulkRemoveRequested(layerIds);
        }
    });

    layout->addWidget(toolbar);
    layout->addWidget(table_);
    setWidget(container);
}

QTableWidget *MeasurementResultsDock::table() const {
    return table_;
}

void MeasurementResultsDock::addOrUpdateResult(const MeasurementResultItemData &item) {
    int row = rowForLayerId(item.layerId);
    if (row < 0) {
        row = table_->rowCount();
        table_->insertRow(row);
        for (int column = 0; column < 3; ++column) {
            auto *cell = new QTableWidgetItem();
            if (column == 0) {
                cell->setData(kLayerIdRole, item.layerId);
            }
            table_->setItem(row, column, cell);
        }
    }

    table_->item(row, 0)->setText(item.name);
    table_->item(row, 0)->setData(kLayerIdRole, item.layerId);
    table_->item(row, 1)->setText(measurementKindText(item.kind));
    table_->item(row, 2)->setText(item.summary);
    updateActionStates();
}

void MeasurementResultsDock::removeResult(const QString &layerId) {
    const int row = rowForLayerId(layerId);
    if (row >= 0) {
        table_->removeRow(row);
    }
    updateActionStates();
}

void MeasurementResultsDock::clearResults() {
    table_->setRowCount(0);
    updateActionStates();
}

void MeasurementResultsDock::selectResult(const QString &layerId) {
    const int row = rowForLayerId(layerId);
    if (row < 0) {
        clearResultSelection();
        return;
    }

    const QSignalBlocker blocker(table_);
    table_->clearSelection();
    table_->setCurrentCell(row, 0);
    table_->selectRow(row);
    updateActionStates();
}

void MeasurementResultsDock::clearResultSelection() {
    const QSignalBlocker blocker(table_);
    table_->clearSelection();
    table_->setCurrentCell(-1, -1);
    updateActionStates();
}

QString MeasurementResultsDock::currentResultId() const {
    const QStringList ids = selectedResultIds();
    return ids.isEmpty() ? QString() : ids.front();
}

QStringList MeasurementResultsDock::selectedResultIds() const {
    QStringList ids;
    if (table_->selectionModel() == nullptr) {
        return ids;
    }
    for (const auto &index : table_->selectionModel()->selectedRows()) {
        if (auto *item = table_->item(index.row(), 0)) {
            ids.append(item->data(kLayerIdRole).toString());
        }
    }
    ids.removeDuplicates();
    return ids;
}

int MeasurementResultsDock::rowForLayerId(const QString &layerId) const {
    for (int row = 0; row < table_->rowCount(); ++row) {
        if (auto *item = table_->item(row, 0); item != nullptr &&
            item->data(kLayerIdRole).toString() == layerId) {
            return row;
        }
    }
    return -1;
}

void MeasurementResultsDock::updateActionStates() {
    const int count = selectedResultIds().size();
    if (zoomAction_ != nullptr) {
        zoomAction_->setEnabled(count >= 1);
    }
    if (editAction_ != nullptr) {
        editAction_->setEnabled(count == 1);
    }
    if (deleteAction_ != nullptr) {
        deleteAction_->setEnabled(count == 1);
    }
    if (bulkDeleteAction_ != nullptr) {
        bulkDeleteAction_->setEnabled(count >= 1);
    }
    if (bulkExportAction_ != nullptr) {
        bulkExportAction_->setEnabled(count >= 1);
    }
}
