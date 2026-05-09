#include "ui/PropertyDock.h"

#include <algorithm>
#include <cmath>
#include <QAbstractItemView>
#include <QAbstractTextDocumentLayout>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>
#include <QSlider>
#include <QStringLiteral>
#include <QTableWidget>
#include <QTextEdit>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QWidget>

#include "data/DataSourceDescriptor.h"

using namespace Qt::Literals::StringLiterals;

namespace {

QString utf8(const std::string &s) {
    return QString::fromUtf8(s.c_str(), static_cast<int>(s.size()));
}

QString compactMiddleText(const QString &text, int leading = 28, int trailing = 28) {
    if (text.size() <= leading + trailing + 5) {
        return text;
    }
    return text.left(leading) + " ... " + text.right(trailing);
}

int attributePriority(const QString &name) {
    const QString key = name.trimmed().toLower();
    if (key == "name" || key == "名称") return 0;
    if (key == "id" || key == "fid" || key == "objectid") return 1;
    if (key == "code" || key == "编码") return 2;
    if (key == "type" || key == "类型" || key == "class") return 3;
    return 10;
}

QString formatDistance(double meters) {
    if (meters >= 1000.0) {
        return QString(u"%1 km"_s).arg(meters / 1000.0, 0, 'f', 3);
    }
    return QString(u"%1 m"_s).arg(meters, 0, 'f', 1);
}

QString formatArea(double squareMeters) {
    if (squareMeters >= 1000000.0) {
        return QString(u"%1 km²"_s).arg(squareMeters / 1000000.0, 0, 'f', 3);
    }
    return QString(u"%1 m²"_s).arg(squareMeters, 0, 'f', 1);
}

QLabel *makeValueLabel(const QString &text, QWidget *parent, bool wordWrap = false) {
    auto *label = new QLabel(text, parent);
    label->setWordWrap(wordWrap);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setCursor(Qt::IBeamCursor);
    return label;
}

QLabel *makeCompactValueLabel(const QString &text, QWidget *parent, bool wordWrap = false) {
    auto *label = makeValueLabel(compactMiddleText(text), parent, wordWrap);
    if (label->text() != text) {
        label->setToolTip(text);
    }
    return label;
}

QTableWidgetItem *makeTableItem(const QString &text) {
    auto *item = new QTableWidgetItem(text);
    item->setToolTip(text);
    return item;
}

QDoubleSpinBox *makeSpinBox(QWidget *parent,
                            const char *objectName,
                            double minimum,
                            double maximum,
                            int decimals,
                            double singleStep,
                            const QString &suffix = QString()) {
    auto *spin = new QDoubleSpinBox(parent);
    spin->setObjectName(QString::fromLatin1(objectName));
    spin->setRange(minimum, maximum);
    spin->setDecimals(decimals);
    spin->setSingleStep(singleStep);
    spin->setKeyboardTracking(false);
    if (!suffix.isEmpty()) {
        spin->setSuffix(suffix);
    }
    return spin;
}

} // namespace

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
    text_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    text_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    text_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(text_);
    connect(text_->document()->documentLayout(),
            &QAbstractTextDocumentLayout::documentSizeChanged,
            this,
            [this]() { updateTextHeight(); });

    pickGroup_ = new QGroupBox(u"拾取结果"_s, propertiesWidget_);
    pickForm_ = new QFormLayout(pickGroup_);
    pickForm_->setLabelAlignment(Qt::AlignRight);
    pickForm_->setSpacing(6);
    pickGroup_->setVisible(false);
    layout->addWidget(pickGroup_);

    pickTable_ = new QTableWidget(propertiesWidget_);
    pickTable_->setColumnCount(2);
    pickTable_->setHorizontalHeaderLabels({u"属性"_s, u"值"_s});
    pickTable_->horizontalHeader()->setStretchLastSection(true);
    pickTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    pickTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    pickTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pickTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    pickTable_->setAlternatingRowColors(true);
    pickTable_->verticalHeader()->setVisible(false);
    pickTable_->setVisible(false);
    layout->addWidget(pickTable_);

    basicGroup_ = new QGroupBox(u"基本信息"_s, propertiesWidget_);
    basicForm_ = new QFormLayout(basicGroup_);
    basicForm_->setLabelAlignment(Qt::AlignRight);
    basicForm_->setSpacing(6);
    basicGroup_->setVisible(false);
    layout->addWidget(basicGroup_);

    modelGroup_ = new QGroupBox(u"模型定位"_s, propertiesWidget_);
    modelForm_ = new QFormLayout(modelGroup_);
    modelForm_->setLabelAlignment(Qt::AlignRight);
    modelForm_->setSpacing(6);
    modelLongitudeSpin_ = makeSpinBox(modelGroup_, "modelLongitudeSpin", -180.0, 180.0, 6, 0.0001);
    modelLatitudeSpin_ = makeSpinBox(modelGroup_, "modelLatitudeSpin", -90.0, 90.0, 6, 0.0001);
    modelHeightSpin_ = makeSpinBox(modelGroup_, "modelHeightSpin", -100000.0, 100000.0, 2, 1.0, u" m"_s);
    modelScaleSpin_ = makeSpinBox(modelGroup_, "modelScaleSpin", 0.01, 100000.0, 3, 0.1);
    modelHeadingSpin_ = makeSpinBox(modelGroup_, "modelHeadingSpin", -360.0, 360.0, 2, 1.0, u"°"_s);
    modelForm_->addRow(u"经度:"_s, modelLongitudeSpin_);
    modelForm_->addRow(u"纬度:"_s, modelLatitudeSpin_);
    modelForm_->addRow(u"高度:"_s, modelHeightSpin_);
    modelForm_->addRow(u"缩放:"_s, modelScaleSpin_);
    modelForm_->addRow(u"航向:"_s, modelHeadingSpin_);
    modelGroup_->setVisible(false);
    layout->addWidget(modelGroup_);

    measurementGroup_ = new QGroupBox(u"量测结果"_s, propertiesWidget_);
    measurementForm_ = new QFormLayout(measurementGroup_);
    measurementForm_->setLabelAlignment(Qt::AlignRight);
    measurementForm_->setSpacing(6);
    measurementGroup_->setVisible(false);
    layout->addWidget(measurementGroup_);

    opacityWidget_ = new QWidget(propertiesWidget_);
    auto *opacityLayout = new QHBoxLayout(opacityWidget_);
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
    opacityWidget_->setVisible(false);
    layout->addWidget(opacityWidget_);

    bandLabel_ = new QLabel(u"波段组合"_s, propertiesWidget_);
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
    bandTable_->setHorizontalHeaderLabels({u"波段"_s, u"类型"_s, u"范围"_s, u"NoData"_s});
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
    connect(modelLongitudeSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PropertyDock::onModelPlacementChanged);
    connect(modelLatitudeSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PropertyDock::onModelPlacementChanged);
    connect(modelHeightSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PropertyDock::onModelPlacementChanged);
    connect(modelScaleSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PropertyDock::onModelPlacementChanged);
    connect(modelHeadingSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PropertyDock::onModelPlacementChanged);
    updateTextHeight();
}

void PropertyDock::showText(const QString &text) {
    text_->setPlainText(text);
    updateTextHeight();
    clearInspection();
}

void PropertyDock::showPickDetails(const QStringList &summaryLines,
                                   const QList<std::pair<QString, QString>> &attributes) {
    text_->setPlainText(u"当前显示拾取结果。"_s);
    updateTextHeight();
    clearInspection();

    for (const QString &line : summaryLines) {
        int separator = line.indexOf(':');
        if (separator <= 0) {
            separator = line.indexOf(u'：');
        }
        if (separator <= 0) {
            continue;
        }

        const QString label = line.left(separator + 1);
        const QString value = line.mid(separator + 1).trimmed();
        pickForm_->addRow(label, makeValueLabel(value, pickGroup_, true));
    }

    pickGroup_->setVisible(pickForm_->rowCount() > 0);

    pickTable_->clearContents();
    auto sortedAttributes = attributes;
    std::stable_sort(sortedAttributes.begin(), sortedAttributes.end(), [](const auto &lhs, const auto &rhs) {
        const int lp = attributePriority(lhs.first);
        const int rp = attributePriority(rhs.first);
        if (lp != rp) {
            return lp < rp;
        }
        return lhs.first.localeAwareCompare(rhs.first) < 0;
    });

    pickTable_->setRowCount(sortedAttributes.size());
    for (int i = 0; i < sortedAttributes.size(); ++i) {
        const auto &[name, value] = sortedAttributes[i];
        pickTable_->setItem(i, 0, makeTableItem(name));
        pickTable_->setItem(i, 1, makeTableItem(value));
    }
    pickTable_->setVisible(!attributes.isEmpty());
}

void PropertyDock::showLayerProperties(const QString &layerId, const QString &name, const QString &typeText,
                                       const QString &source, bool visible, double opacity,
                                       const std::optional<RasterMetadata> &rasterMeta,
                                       const std::optional<VectorLayerInfo> &vectorMeta,
                                       const std::optional<ModelPlacement> &modelPlacement,
                                       const std::optional<MeasurementLayerData> &measurementData) {
    currentLayerId_ = layerId;
    text_->setPlainText(u"当前显示选中图层属性。"_s);
    updateTextHeight();

    clearForm(basicForm_);
    basicForm_->addRow(u"名称:"_s, makeValueLabel(name, basicGroup_, true));
    basicForm_->addRow(u"类型:"_s, makeValueLabel(typeText, basicGroup_));
    basicForm_->addRow(u"来源:"_s, makeCompactValueLabel(source, basicGroup_, true));
    basicForm_->addRow(u"可见:"_s, makeValueLabel(visible ? u"是"_s : u"否"_s, basicGroup_));
    basicGroup_->setVisible(true);
    opacityWidget_->setVisible(true);

    opacitySlider_->blockSignals(true);
    opacitySlider_->setValue(static_cast<int>(opacity * 100.0 + 0.5));
    opacitySlider_->blockSignals(false);
    opacityLabel_->setText(QString::number(opacity, 'f', 2));

    if (modelPlacement.has_value()) {
        modelLongitudeSpin_->blockSignals(true);
        modelLatitudeSpin_->blockSignals(true);
        modelHeightSpin_->blockSignals(true);
        modelScaleSpin_->blockSignals(true);
        modelHeadingSpin_->blockSignals(true);
        modelLongitudeSpin_->setValue(modelPlacement->longitude);
        modelLatitudeSpin_->setValue(modelPlacement->latitude);
        modelHeightSpin_->setValue(modelPlacement->height);
        modelScaleSpin_->setValue(modelPlacement->scale > 0.0 ? modelPlacement->scale : 1.0);
        modelHeadingSpin_->setValue(modelPlacement->heading);
        modelLongitudeSpin_->blockSignals(false);
        modelLatitudeSpin_->blockSignals(false);
        modelHeightSpin_->blockSignals(false);
        modelScaleSpin_->blockSignals(false);
        modelHeadingSpin_->blockSignals(false);
        modelGroup_->setVisible(true);
    } else {
        modelGroup_->setVisible(false);
    }

    if (measurementData.has_value()) {
        clearForm(measurementForm_);
        measurementForm_->addRow(u"类型:"_s,
                                 makeValueLabel(measurementData->kind == MeasurementKind::Area ? u"测面"_s : u"测距"_s,
                                                measurementGroup_));
        measurementForm_->addRow(u"点数:"_s,
                                 makeValueLabel(QString::number(measurementData->points.size()), measurementGroup_));
        measurementForm_->addRow(
            measurementData->kind == MeasurementKind::Area ? u"周长:"_s : u"总长:"_s,
            makeValueLabel(formatDistance(measurementData->lengthMeters), measurementGroup_));
        if (measurementData->kind == MeasurementKind::Area) {
            measurementForm_->addRow(u"面积:"_s,
                                     makeValueLabel(formatArea(measurementData->areaSquareMeters), measurementGroup_));
        }
        measurementGroup_->setVisible(true);
    } else {
        measurementGroup_->setVisible(false);
    }

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
        clearForm(rasterForm_);
        rasterForm_->addRow(u"驱动:"_s, makeValueLabel(utf8(rasterMeta->driverName), rasterGroup_));
        rasterForm_->addRow(
            u"尺寸:"_s,
            makeValueLabel(QString("%1 x %2").arg(rasterMeta->rasterXSize).arg(rasterMeta->rasterYSize), rasterGroup_));
        rasterForm_->addRow(u"波段数:"_s, makeValueLabel(QString::number(rasterMeta->bandCount), rasterGroup_));
        if (!rasterMeta->epsgCode.empty()) {
            rasterForm_->addRow(u"坐标系:"_s, makeValueLabel(utf8(rasterMeta->epsgCode), rasterGroup_, true));
        }
        if (rasterMeta->pixelSizeX > 0 || rasterMeta->pixelSizeY > 0) {
            rasterForm_->addRow(
                u"像素大小:"_s,
                makeValueLabel(
                    QString("%1 x %2").arg(rasterMeta->pixelSizeX, 0, 'f', 6).arg(rasterMeta->pixelSizeY, 0, 'f', 6),
                    rasterGroup_));
        }
        rasterGroup_->setVisible(true);

        bandTable_->clearContents();
        bandTable_->setRowCount(static_cast<int>(rasterMeta->bands.size()));
        for (int i = 0; i < static_cast<int>(rasterMeta->bands.size()); ++i) {
            const auto &band = rasterMeta->bands[i];
            bandTable_->setItem(i, 0, makeTableItem(QString::number(band.index)));
            bandTable_->setItem(i, 1, makeTableItem(utf8(band.dataType)));
            if (band.hasMinMax) {
                bandTable_->setItem(i, 2, makeTableItem(QString("[%1, %2]").arg(band.min, 0, 'f', 2).arg(band.max, 0, 'f', 2)));
            } else {
                bandTable_->setItem(i, 2, makeTableItem("-"));
            }
            if (band.hasNoDataValue) {
                bandTable_->setItem(i, 3, makeTableItem(QString::number(band.noDataValue, 'f', 2)));
            } else {
                bandTable_->setItem(i, 3, makeTableItem("-"));
            }
        }
        bandTable_->setVisible(!rasterMeta->bands.empty());
    } else {
        rasterGroup_->setVisible(false);
        bandTable_->setVisible(false);
    }

    if (vectorMeta.has_value()) {
        clearForm(vectorForm_);
        vectorForm_->addRow(u"图层:"_s, makeValueLabel(utf8(vectorMeta->name), vectorGroup_, true));
        vectorForm_->addRow(u"几何类型:"_s, makeValueLabel(utf8(vectorMeta->geometryType), vectorGroup_));
        vectorForm_->addRow(u"要素数:"_s, makeValueLabel(QString::number(vectorMeta->featureCount), vectorGroup_));
        if (!vectorMeta->epsgCode.empty()) {
            vectorForm_->addRow(u"坐标系:"_s, makeValueLabel(utf8(vectorMeta->epsgCode), vectorGroup_, true));
        }
        vectorGroup_->setVisible(true);

        fieldTable_->clearContents();
        fieldTable_->setRowCount(static_cast<int>(vectorMeta->fields.size()));
        for (int i = 0; i < static_cast<int>(vectorMeta->fields.size()); ++i) {
            const auto &field = vectorMeta->fields[i];
            fieldTable_->setItem(i, 0, makeTableItem(utf8(field.name)));
            fieldTable_->setItem(i, 1, makeTableItem(utf8(field.typeName)));
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
    text_->setPlainText(u"未选择图层。"_s);
    updateTextHeight();
    opacitySlider_->blockSignals(true);
    opacitySlider_->setValue(100);
    opacitySlider_->blockSignals(false);
    opacityLabel_->setText("1.00");
    opacityWidget_->setVisible(false);
    bandCombo_->setVisible(false);
    bandLabel_->setVisible(false);
    basicGroup_->setVisible(false);
    modelLongitudeSpin_->blockSignals(true);
    modelLatitudeSpin_->blockSignals(true);
    modelHeightSpin_->blockSignals(true);
    modelScaleSpin_->blockSignals(true);
    modelHeadingSpin_->blockSignals(true);
    modelLongitudeSpin_->setValue(0.0);
    modelLatitudeSpin_->setValue(0.0);
    modelHeightSpin_->setValue(0.0);
    modelScaleSpin_->setValue(1.0);
    modelHeadingSpin_->setValue(0.0);
    modelLongitudeSpin_->blockSignals(false);
    modelLatitudeSpin_->blockSignals(false);
    modelHeightSpin_->blockSignals(false);
    modelScaleSpin_->blockSignals(false);
    modelHeadingSpin_->blockSignals(false);
    modelGroup_->setVisible(false);
    measurementGroup_->setVisible(false);
    rasterGroup_->setVisible(false);
    bandTable_->setVisible(false);
    vectorGroup_->setVisible(false);
    fieldTable_->setVisible(false);
}

void PropertyDock::resizeEvent(QResizeEvent *event) {
    QDockWidget::resizeEvent(event);
    updateTextHeight();
}

void PropertyDock::updateTextHeight() {
    if (text_ == nullptr || updatingTextHeight_) {
        return;
    }

    updatingTextHeight_ = true;
    const int viewportWidth = (std::max)(text_->viewport()->width(), 1);
    text_->document()->setTextWidth(viewportWidth);
    text_->document()->adjustSize();

    const int frame = text_->frameWidth() * 2;
    const int margin = static_cast<int>(std::ceil(text_->document()->documentMargin() * 2.0));
    const int documentHeight = static_cast<int>(std::ceil(text_->document()->size().height()));
    const int targetHeight = (std::max)(40, documentHeight + frame + margin);
    text_->setFixedHeight(targetHeight);
    updatingTextHeight_ = false;
}

void PropertyDock::clearForm(QFormLayout *form) {
    while (form->rowCount() > 0) {
        form->removeRow(0);
    }
}

void PropertyDock::clearInspection() {
    clearForm(pickForm_);
    pickGroup_->setVisible(false);
    pickTable_->clearContents();
    pickTable_->setRowCount(0);
    pickTable_->setVisible(false);
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

void PropertyDock::onModelPlacementChanged() {
    if (currentLayerId_.isEmpty()) {
        return;
    }

    ModelPlacement placement;
    placement.longitude = modelLongitudeSpin_->value();
    placement.latitude = modelLatitudeSpin_->value();
    placement.height = modelHeightSpin_->value();
    placement.scale = modelScaleSpin_->value();
    placement.heading = modelHeadingSpin_->value();
    emit modelPlacementChanged(currentLayerId_, placement);
}
