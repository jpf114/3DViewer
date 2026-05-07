#include "ui/PropertyDock.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QScrollArea>
#include <QSlider>
#include <QStringLiteral>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include "data/DataSourceDescriptor.h"

using namespace Qt::Literals::StringLiterals;

namespace {
QString utf8(const std::string &s) {
    return QString::fromUtf8(s.c_str(), static_cast<int>(s.size()));
}
}

PropertyDock::PropertyDock(QWidget *parent)
    : QDockWidget(u"属性"_s, parent) {
    setupUi();
}

void PropertyDock::setupUi() {
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    propertiesWidget_ = new QWidget(scrollArea);
    auto *layout = new QVBoxLayout(propertiesWidget_);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    text_ = new QTextEdit(propertiesWidget_);
    text_->setReadOnly(true);
    text_->setMaximumHeight(80);
    layout->addWidget(text_);

    basicGroup_ = new QGroupBox(u"基本信息"_s, propertiesWidget_);
    basicForm_ = new QFormLayout(basicGroup_);
    basicForm_->setLabelAlignment(Qt::AlignRight);
    basicForm_->setSpacing(6);
    basicGroup_->setVisible(false);
    layout->addWidget(basicGroup_);

    auto *opacityWidget = new QWidget(propertiesWidget_);
    auto *opacityLayout = new QHBoxLayout(opacityWidget);
    opacityLayout->setContentsMargins(0, 0, 0, 0);
    opacitySlider_ = new QSlider(Qt::Horizontal, propertiesWidget_);
    opacitySlider_->setRange(0, 100);
    opacitySlider_->setValue(100);
    opacitySlider_->setTickPosition(QSlider::TicksBelow);
    opacitySlider_->setTickInterval(25);
    opacityLabel_ = new QLabel("1.00", propertiesWidget_);
    opacityLabel_->setMinimumWidth(36);
    opacityLayout->addWidget(opacitySlider_);
    opacityLayout->addWidget(opacityLabel_);
    layout->addWidget(opacityWidget);

    bandLabel_ = new QLabel(u"波段组合："_s, propertiesWidget_);
    layout->addWidget(bandLabel_);

    bandCombo_ = new QComboBox(propertiesWidget_);
    bandCombo_->addItem(u"真彩色 (1,2,3)"_s);
    bandCombo_->setVisible(false);
    bandLabel_->setVisible(false);
    layout->addWidget(bandCombo_);

    rasterGroup_ = new QGroupBox(u"影像信息"_s, propertiesWidget_);
    rasterForm_ = new QFormLayout(rasterGroup_);
    rasterForm_->setLabelAlignment(Qt::AlignRight);
    rasterForm_->setSpacing(6);
    rasterGroup_->setVisible(false);
    layout->addWidget(rasterGroup_);

    bandTable_ = new QTableWidget(propertiesWidget_);
    bandTable_->setColumnCount(4);
    bandTable_->setHorizontalHeaderLabels({u"波段"_s, u"类型"_s, u"范围"_s, u"无数据值"_s});
    bandTable_->horizontalHeader()->setStretchLastSection(true);
    bandTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    bandTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    bandTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    bandTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    bandTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    bandTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    bandTable_->setAlternatingRowColors(true);
    bandTable_->verticalHeader()->setVisible(false);
    bandTable_->setVisible(false);
    layout->addWidget(bandTable_);

    vectorGroup_ = new QGroupBox(u"矢量信息"_s, propertiesWidget_);
    vectorForm_ = new QFormLayout(vectorGroup_);
    vectorForm_->setLabelAlignment(Qt::AlignRight);
    vectorForm_->setSpacing(6);
    vectorGroup_->setVisible(false);
    layout->addWidget(vectorGroup_);

    fieldTable_ = new QTableWidget(propertiesWidget_);
    fieldTable_->setColumnCount(2);
    fieldTable_->setHorizontalHeaderLabels({u"字段名"_s, u"类型"_s});
    fieldTable_->horizontalHeader()->setStretchLastSection(true);
    fieldTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    fieldTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    fieldTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fieldTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    fieldTable_->setAlternatingRowColors(true);
    fieldTable_->verticalHeader()->setVisible(false);
    fieldTable_->setVisible(false);
    layout->addWidget(fieldTable_);

    layout->addStretch();
    scrollArea->setWidget(propertiesWidget_);
    setWidget(scrollArea);

    connect(opacitySlider_, &QSlider::valueChanged, this, &PropertyDock::onOpacitySliderChanged);
    connect(bandCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PropertyDock::onBandComboChanged);
}

void PropertyDock::showText(const QString &text) {
    text_->setPlainText(text);
}

void PropertyDock::showLayerProperties(const QString &layerId, const QString &name, const QString &typeText,
                                       const QString &source, bool visible, double opacity,
                                       const std::optional<RasterMetadata> &rasterMeta,
                                       const std::optional<VectorLayerInfo> &vectorMeta) {
    currentLayerId_ = layerId;

    while (basicForm_->rowCount() > 0) {
        basicForm_->removeRow(0);
    }
    basicForm_->addRow(u"名称："_s, new QLabel(name));
    basicForm_->addRow(u"类型："_s, new QLabel(typeText));
    basicForm_->addRow(u"来源："_s, new QLabel(source));
    basicForm_->addRow(u"可见："_s, new QLabel(visible ? u"是"_s : u"否"_s));
    basicGroup_->setVisible(true);

    opacitySlider_->blockSignals(true);
    opacitySlider_->setValue(static_cast<int>(opacity * 100.0 + 0.5));
    opacitySlider_->blockSignals(false);
    opacityLabel_->setText(QString::number(opacity, 'f', 2));

    const int bandCount = rasterMeta.has_value() ? rasterMeta->bandCount : 0;
    currentBandCount_ = bandCount;

    if (bandCount >= 3) {
        bandCombo_->blockSignals(true);
        bandCombo_->clear();
        bandCombo_->addItem(u"真彩色 (1,2,3)"_s, "1,2,3");
        if (bandCount >= 4) {
            bandCombo_->addItem(u"假彩色 (4,3,2)"_s, "4,3,2");
            bandCombo_->addItem(u"红外彩色 (4,2,1)"_s, "4,2,1");
        }
        if (bandCount >= 5) {
            bandCombo_->addItem(u"短波红外 (5,4,3)"_s, "5,4,3");
        }
        if (bandCount >= 7) {
            bandCombo_->addItem(u"城市 (7,5,3)"_s, "7,5,3");
        }
        bandCombo_->blockSignals(false);
        bandCombo_->setVisible(true);
        bandLabel_->setVisible(true);
    } else {
        bandCombo_->setVisible(false);
        bandLabel_->setVisible(false);
    }

    if (rasterMeta.has_value()) {
        while (rasterForm_->rowCount() > 0) {
            rasterForm_->removeRow(0);
        }
        rasterForm_->addRow(u"驱动："_s, new QLabel(utf8(rasterMeta->driverName)));
        rasterForm_->addRow(u"尺寸："_s, new QLabel(QString("%1 x %2").arg(rasterMeta->rasterXSize).arg(rasterMeta->rasterYSize)));
        rasterForm_->addRow(u"波段数："_s, new QLabel(QString::number(rasterMeta->bandCount)));
        if (!rasterMeta->epsgCode.empty()) {
            rasterForm_->addRow(u"坐标系："_s, new QLabel(utf8(rasterMeta->epsgCode)));
        }
        if (rasterMeta->pixelSizeX > 0 || rasterMeta->pixelSizeY > 0) {
            rasterForm_->addRow(u"像素大小："_s, new QLabel(QString("%1 x %2")
                .arg(rasterMeta->pixelSizeX, 0, 'f', 6).arg(rasterMeta->pixelSizeY, 0, 'f', 6)));
        }
        rasterGroup_->setVisible(true);

        bandTable_->setRowCount(static_cast<int>(rasterMeta->bands.size()));
        for (int i = 0; i < static_cast<int>(rasterMeta->bands.size()); ++i) {
            const auto &band = rasterMeta->bands[i];
            bandTable_->setItem(i, 0, new QTableWidgetItem(QString::number(band.index)));
            bandTable_->setItem(i, 1, new QTableWidgetItem(utf8(band.dataType)));
            if (band.hasMinMax) {
                bandTable_->setItem(i, 2, new QTableWidgetItem(QString("[%1, %2]").arg(band.min, 0, 'f', 2).arg(band.max, 0, 'f', 2)));
            } else {
                bandTable_->setItem(i, 2, new QTableWidgetItem("-"));
            }
            if (band.hasNoDataValue) {
                bandTable_->setItem(i, 3, new QTableWidgetItem(QString::number(band.noDataValue, 'f', 2)));
            } else {
                bandTable_->setItem(i, 3, new QTableWidgetItem("-"));
            }
        }
        bandTable_->setVisible(!rasterMeta->bands.empty());
    } else {
        rasterGroup_->setVisible(false);
        bandTable_->setVisible(false);
    }

    if (vectorMeta.has_value()) {
        while (vectorForm_->rowCount() > 0) {
            vectorForm_->removeRow(0);
        }
        vectorForm_->addRow(u"图层："_s, new QLabel(utf8(vectorMeta->name)));
        vectorForm_->addRow(u"几何类型："_s, new QLabel(utf8(vectorMeta->geometryType)));
        vectorForm_->addRow(u"要素数："_s, new QLabel(QString::number(vectorMeta->featureCount)));
        if (!vectorMeta->epsgCode.empty()) {
            vectorForm_->addRow(u"坐标系："_s, new QLabel(utf8(vectorMeta->epsgCode)));
        }
        vectorGroup_->setVisible(true);

        fieldTable_->setRowCount(static_cast<int>(vectorMeta->fields.size()));
        for (int i = 0; i < static_cast<int>(vectorMeta->fields.size()); ++i) {
            const auto &field = vectorMeta->fields[i];
            fieldTable_->setItem(i, 0, new QTableWidgetItem(utf8(field.name)));
            fieldTable_->setItem(i, 1, new QTableWidgetItem(utf8(field.typeName)));
        }
        fieldTable_->setVisible(!vectorMeta->fields.empty());
    } else {
        vectorGroup_->setVisible(false);
        fieldTable_->setVisible(false);
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
    basicGroup_->setVisible(false);
    rasterGroup_->setVisible(false);
    bandTable_->setVisible(false);
    vectorGroup_->setVisible(false);
    fieldTable_->setVisible(false);
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

    const QString bandData = bandCombo_->itemData(index).toString();
    const QStringList parts = bandData.split(',');
    if (parts.size() != 3) {
        return;
    }

    const int red = parts[0].toInt();
    const int green = parts[1].toInt();
    const int blue = parts[2].toInt();
    emit bandMappingChanged(currentLayerId_, red, green, blue);
}
