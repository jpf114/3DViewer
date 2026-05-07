#pragma once

#include <QDockWidget>
#include <optional>

struct RasterMetadata;
struct VectorLayerInfo;

class QComboBox;
class QSlider;
class QLabel;
class QGroupBox;
class QFormLayout;
class QTableWidget;
class QTextEdit;

class PropertyDock : public QDockWidget {
    Q_OBJECT

public:
    explicit PropertyDock(QWidget *parent = nullptr);
    void showText(const QString &text);
    void showLayerProperties(const QString &layerId, const QString &name, const QString &typeText,
                             const QString &source, bool visible, double opacity,
                             const std::optional<RasterMetadata> &rasterMeta,
                             const std::optional<VectorLayerInfo> &vectorMeta);
    void clearLayerProperties();

signals:
    void opacityChanged(const QString &layerId, double opacity);
    void bandMappingChanged(const QString &layerId, int red, int green, int blue);

private:
    void setupUi();
    void onOpacitySliderChanged(int value);
    void onBandComboChanged(int index);

    QTextEdit *text_;
    QSlider *opacitySlider_;
    QLabel *opacityLabel_;
    QComboBox *bandCombo_;
    QLabel *bandLabel_;
    QWidget *propertiesWidget_;
    QString currentLayerId_;
    int currentBandCount_ = 0;

    QGroupBox *basicGroup_;
    QFormLayout *basicForm_;
    QGroupBox *rasterGroup_;
    QFormLayout *rasterForm_;
    QTableWidget *bandTable_;
    QGroupBox *vectorGroup_;
    QFormLayout *vectorForm_;
    QTableWidget *fieldTable_;
};
