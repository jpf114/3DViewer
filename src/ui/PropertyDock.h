#pragma once

#include <QDockWidget>

class QComboBox;
class QSlider;
class QLabel;
class QTextEdit;

class PropertyDock : public QDockWidget {
    Q_OBJECT

public:
    explicit PropertyDock(QWidget *parent = nullptr);
    void showText(const QString &text);
    void showLayerProperties(const QString &layerId, const QString &infoText, double opacity, int bandCount = 0);
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
};
