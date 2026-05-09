#pragma once

#include <QDockWidget>
#include <QList>
#include <optional>
#include <utility>

#include "data/DataSourceDescriptor.h"
#include "layers/MeasurementLayerData.h"

struct RasterMetadata;
struct VectorLayerInfo;

class QComboBox;
class QDoubleSpinBox;
class QSlider;
class QLabel;
class QGroupBox;
class QFormLayout;
class QResizeEvent;
class QTableWidget;
class QTextEdit;
class QWidget;

class PropertyDock : public QDockWidget {
    Q_OBJECT

public:
    explicit PropertyDock(QWidget *parent = nullptr);
    void showText(const QString &text);
    void showPickDetails(const QStringList &summaryLines,
                         const QList<std::pair<QString, QString>> &attributes);
    void showLayerProperties(const QString &layerId, const QString &name, const QString &typeText,
                             const QString &source, bool visible, double opacity,
                             const std::optional<RasterMetadata> &rasterMeta,
                             const std::optional<VectorLayerInfo> &vectorMeta,
                             const std::optional<ModelPlacement> &modelPlacement = std::nullopt,
                             const std::optional<MeasurementLayerData> &measurementData = std::nullopt);
    void clearLayerProperties();

signals:
    void opacityChanged(const QString &layerId, double opacity);
    void bandMappingChanged(const QString &layerId, int red, int green, int blue);
    void modelPlacementChanged(const QString &layerId, const ModelPlacement &placement);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupUi();
    void updateTextHeight();
    void clearForm(QFormLayout *form);
    void clearInspection();
    void onOpacitySliderChanged(int value);
    void onBandComboChanged(int index);
    void onModelPlacementChanged();

    QTextEdit *text_;
    QWidget *opacityWidget_;
    QSlider *opacitySlider_;
    QLabel *opacityLabel_;
    QComboBox *bandCombo_;
    QLabel *bandLabel_;
    QWidget *propertiesWidget_;
    QString currentLayerId_;
    int currentBandCount_ = 0;
    bool updatingTextHeight_ = false;

    QGroupBox *basicGroup_;
    QFormLayout *basicForm_;
    QGroupBox *modelGroup_;
    QFormLayout *modelForm_;
    QDoubleSpinBox *modelLongitudeSpin_;
    QDoubleSpinBox *modelLatitudeSpin_;
    QDoubleSpinBox *modelHeightSpin_;
    QDoubleSpinBox *modelScaleSpin_;
    QDoubleSpinBox *modelHeadingSpin_;
    QGroupBox *measurementGroup_;
    QFormLayout *measurementForm_;
    QGroupBox *rasterGroup_;
    QFormLayout *rasterForm_;
    QTableWidget *bandTable_;
    QGroupBox *vectorGroup_;
    QFormLayout *vectorForm_;
    QTableWidget *fieldTable_;
    QGroupBox *pickGroup_;
    QFormLayout *pickForm_;
    QTableWidget *pickTable_;
};
