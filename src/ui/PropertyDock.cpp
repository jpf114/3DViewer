#include "ui/PropertyDock.h"

#include <QComboBox>
#include <QLabel>
#include <QSlider>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

PropertyDock::PropertyDock(QWidget *parent)
    : QDockWidget("Properties", parent) {
    setupUi();
}

void PropertyDock::setupUi() {
    propertiesWidget_ = new QWidget(this);
    auto *layout = new QVBoxLayout(propertiesWidget_);

    text_ = new QTextEdit(propertiesWidget_);
    text_->setReadOnly(true);
    layout->addWidget(text_);

    auto *opacityHeader = new QLabel("Opacity:", propertiesWidget_);
    layout->addWidget(opacityHeader);

    opacitySlider_ = new QSlider(Qt::Horizontal, propertiesWidget_);
    opacitySlider_->setRange(0, 100);
    opacitySlider_->setValue(100);
    opacitySlider_->setTickPosition(QSlider::TicksBelow);
    opacitySlider_->setTickInterval(25);
    layout->addWidget(opacitySlider_);

    opacityLabel_ = new QLabel("1.00", propertiesWidget_);
    layout->addWidget(opacityLabel_);

    bandLabel_ = new QLabel("Band Combination:", propertiesWidget_);
    layout->addWidget(bandLabel_);

    bandCombo_ = new QComboBox(propertiesWidget_);
    bandCombo_->addItem("RGB (1,2,3)");
    bandCombo_->setVisible(false);
    bandLabel_->setVisible(false);
    layout->addWidget(bandCombo_);

    layout->addStretch();
    setWidget(propertiesWidget_);

    connect(opacitySlider_, &QSlider::valueChanged, this, &PropertyDock::onOpacitySliderChanged);
    connect(bandCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PropertyDock::onBandComboChanged);
}

void PropertyDock::showText(const QString &text) {
    text_->setPlainText(text);
}

void PropertyDock::showLayerProperties(const QString &layerId, const QString &infoText, double opacity, int bandCount) {
    currentLayerId_ = layerId;
    currentBandCount_ = bandCount;
    text_->setPlainText(infoText);

    opacitySlider_->blockSignals(true);
    opacitySlider_->setValue(static_cast<int>(opacity * 100.0 + 0.5));
    opacitySlider_->blockSignals(false);
    opacityLabel_->setText(QString::number(opacity, 'f', 2));

    if (bandCount >= 3) {
        bandCombo_->blockSignals(true);
        bandCombo_->clear();
        bandCombo_->addItem("RGB (1,2,3)", "1,2,3");
        if (bandCount >= 4) {
            bandCombo_->addItem("False Color (4,3,2)", "4,3,2");
            bandCombo_->addItem("Color IR (4,2,1)", "4,2,1");
        }
        if (bandCount >= 5) {
            bandCombo_->addItem("SWIR (5,4,3)", "5,4,3");
        }
        if (bandCount >= 7) {
            bandCombo_->addItem("Urban (7,5,3)", "7,5,3");
        }
        bandCombo_->blockSignals(false);
        bandCombo_->setVisible(true);
        bandLabel_->setVisible(true);
    } else {
        bandCombo_->setVisible(false);
        bandLabel_->setVisible(false);
    }
}

void PropertyDock::clearLayerProperties() {
    currentLayerId_.clear();
    currentBandCount_ = 0;
    opacitySlider_->blockSignals(true);
    opacitySlider_->setValue(100);
    opacitySlider_->blockSignals(false);
    opacityLabel_->setText("1.00");
    bandCombo_->setVisible(false);
    bandLabel_->setVisible(false);
}

void PropertyDock::onOpacitySliderChanged(int value) {
    const double opacity = value / 100.0;
    opacityLabel_->setText(QString::number(opacity, 'f', 2));
    if (!currentLayerId_.isEmpty()) {
        emit opacityChanged(currentLayerId_, opacity);
    }
}

void PropertyDock::onBandComboChanged(int index) {
    if (currentLayerId_.isEmpty() || index < 0) {
        return;
    }

    const QString data = bandCombo_->itemData(index).toString();
    const QStringList parts = data.split(',');
    if (parts.size() != 3) {
        return;
    }

    const int red = parts[0].toInt();
    const int green = parts[1].toInt();
    const int blue = parts[2].toInt();
    emit bandMappingChanged(currentLayerId_, red, green, blue);
}
