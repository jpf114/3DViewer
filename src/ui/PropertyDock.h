#pragma once

#include <QDockWidget>

class QTextEdit;

class PropertyDock : public QDockWidget {
    Q_OBJECT

public:
    explicit PropertyDock(QWidget *parent = nullptr);
    void showText(const QString &text);

private:
    QTextEdit *text_;
};
