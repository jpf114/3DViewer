#pragma once

#include <QDockWidget>
#include <QStringList>

class QAction;
class QTableWidget;
enum class MeasurementKind;

struct MeasurementResultItemData {
    QString layerId;
    QString name;
    MeasurementKind kind;
    QString summary;
};

class MeasurementResultsDock : public QDockWidget {
    Q_OBJECT

public:
    explicit MeasurementResultsDock(QWidget *parent = nullptr);
    QTableWidget *table() const;
    void addOrUpdateResult(const MeasurementResultItemData &item);
    void removeResult(const QString &layerId);
    void clearResults();
    void selectResult(const QString &layerId);
    void clearResultSelection();
    QString currentResultId() const;
    QStringList selectedResultIds() const;

signals:
    void currentResultChanged(const QString &layerId);
    void zoomRequested(const QString &layerId);
    void editRequested(const QString &layerId);
    void removeRequested(const QString &layerId);
    void bulkRemoveRequested(const QStringList &layerIds);
    void bulkExportRequested(const QStringList &layerIds);

private:
    int rowForLayerId(const QString &layerId) const;
    void updateActionStates();

    QTableWidget *table_;
    QAction *zoomAction_;
    QAction *editAction_;
    QAction *deleteAction_;
    QAction *bulkDeleteAction_;
    QAction *bulkExportAction_;
};
