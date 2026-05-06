#include "ui/PropertyDock.h"

#include <QTextEdit>

PropertyDock::PropertyDock(QWidget *parent)
    : QDockWidget("Properties", parent), text_(new QTextEdit(this)) {
    text_->setReadOnly(true);
    setWidget(text_);
}

void PropertyDock::showText(const QString &text) {
    text_->setPlainText(text);
}
