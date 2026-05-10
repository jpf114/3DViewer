#include "ui/MeasurementResultsDock.h"

#include <QAction>
#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
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
      filterEdit_(new QLineEdit(this)),
      sortCombo_(new QComboBox(this)),
      emptyStateLabel_(new QLabel(this)),
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
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    filterEdit_->setObjectName("measurementResultsFilterEdit");
    filterEdit_->setPlaceholderText(QString::fromUtf8(u8"筛选名称、类型或摘要"));
    filterEdit_->setClearButtonEnabled(true);
    filterEdit_->setMaximumWidth(220);
    toolbar->addWidget(filterEdit_);

    sortCombo_->setObjectName("measurementResultsSortCombo");
    sortCombo_->addItem(QString::fromUtf8(u8"按名称"));
    sortCombo_->addItem(QString::fromUtf8(u8"按类型"));
    sortCombo_->addItem(QString::fromUtf8(u8"按摘要"));
    toolbar->addWidget(sortCombo_);

    toolbar->addSeparator();

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
    table_->setContextMenuPolicy(Qt::CustomContextMenu);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    connect(table_, &QTableWidget::itemSelectionChanged, this, [this]() {
        updateActionStates();
        emit currentResultChanged(currentResultId());
    });
    connect(filterEdit_, &QLineEdit::textChanged, this, [this]() {
        applyFilterAndSort();
    });
    connect(sortCombo_, &QComboBox::currentIndexChanged, this, [this](int) {
        applyFilterAndSort();
    });
    connect(table_, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem *item) {
        if (item == nullptr) {
            return;
        }
        const QString layerId = item->data(kLayerIdRole).toString();
        if (!layerId.isEmpty()) {
            emit zoomRequested(layerId);
        }
    });
    connect(table_, &QTableWidget::customContextMenuRequested, this, [this, &icons, toolColor](const QPoint &pos) {
        if (selectedResultIds().isEmpty()) {
            return;
        }

        QMenu menu(this);
        menu.addAction(icons.icon("magnifying-glass-plus-regular.svg", 16, toolColor), QString::fromUtf8(u8"定位"), zoomAction_, &QAction::trigger);
        menu.addAction(icons.icon("ruler-regular.svg", 16, toolColor), QString::fromUtf8(u8"编辑"), editAction_, &QAction::trigger);
        menu.addAction(icons.icon("minus-circle-regular.svg", 16, toolColor), QString::fromUtf8(u8"删除"), deleteAction_, &QAction::trigger);
        menu.addSeparator();
        menu.addAction(icons.icon("trash-regular.svg", 16, toolColor), QString::fromUtf8(u8"批量删除"), bulkDeleteAction_, &QAction::trigger);
        menu.addAction(icons.icon("export-regular.svg", 16, toolColor), QString::fromUtf8(u8"批量导出"), bulkExportAction_, &QAction::trigger);
        menu.exec(table_->viewport()->mapToGlobal(pos));
    });
    connect(zoomAction_, &QAction::triggered, this, [this]() {
        const QString layerId = currentResultId();
        if (!layerId.isEmpty()) {
            emit zoomRequested(layerId);
        }
    });
    connect(editAction_, &QAction::triggered, this, [this]() {
        const QString layerId = currentResultId();
        if (!layerId.isEmpty()) {
            emit editRequested(layerId);
        }
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
    connect(bulkExportAction_, &QAction::triggered, this, [this]() {
        const QStringList layerIds = selectedResultIds();
        if (!layerIds.isEmpty()) {
            emit bulkExportRequested(layerIds);
        }
    });

    emptyStateLabel_->setObjectName("measurementResultsEmptyStateLabel");
    emptyStateLabel_->setAlignment(Qt::AlignCenter);
    emptyStateLabel_->setWordWrap(true);
    emptyStateLabel_->setMargin(24);
    emptyStateLabel_->setText(QString::fromUtf8(u8"暂无量测成果。\n开始测距或测面后，结果会显示在这里。"));

    layout->addWidget(toolbar);
    layout->addWidget(emptyStateLabel_);
    layout->addWidget(table_);
    setWidget(container);
    updateEmptyState();
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
    applyFilterAndSort();
    updateActionStates();
    updateEmptyState();
}

void MeasurementResultsDock::removeResult(const QString &layerId) {
    const int row = rowForLayerId(layerId);
    if (row >= 0) {
        table_->removeRow(row);
    }
    applyFilterAndSort();
    updateActionStates();
    updateEmptyState();
}

void MeasurementResultsDock::clearResults() {
    table_->setRowCount(0);
    applyFilterAndSort();
    updateActionStates();
    updateEmptyState();
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

void MeasurementResultsDock::updateEmptyState() {
    bool hasVisibleRows = false;
    for (int row = 0; row < table_->rowCount(); ++row) {
        if (!table_->isRowHidden(row)) {
            hasVisibleRows = true;
            break;
        }
    }

    const bool empty = !hasVisibleRows;
    if (emptyStateLabel_ != nullptr) {
        emptyStateLabel_->setText(table_->rowCount() == 0
            ? QString::fromUtf8(u8"暂无量测成果。\n开始测距或测面后，结果会显示在这里。")
            : QString::fromUtf8(u8"没有符合当前筛选条件的量测成果。"));
        emptyStateLabel_->setVisible(empty);
    }
    if (table_ != nullptr) {
        table_->setVisible(table_->rowCount() > 0);
    }
}

void MeasurementResultsDock::applyFilterAndSort() {
    const QString filterText = filterEdit_ != nullptr ? filterEdit_->text().trimmed() : QString();
    for (int row = 0; row < table_->rowCount(); ++row) {
        bool matches = filterText.isEmpty();
        if (!matches) {
            for (int column = 0; column < table_->columnCount(); ++column) {
                auto *item = table_->item(row, column);
                if (item != nullptr && item->text().contains(filterText, Qt::CaseInsensitive)) {
                    matches = true;
                    break;
                }
            }
        }
        table_->setRowHidden(row, !matches);
    }

    if (sortCombo_ != nullptr) {
        table_->setSortingEnabled(true);
        table_->sortItems(sortCombo_->currentIndex(), Qt::AscendingOrder);
        table_->setSortingEnabled(false);
    }
    updateEmptyState();
}
