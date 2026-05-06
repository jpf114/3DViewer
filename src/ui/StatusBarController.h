#pragma once

#include <QObject>

class QLabel;
class QMainWindow;

class StatusBarController : public QObject {
    Q_OBJECT

public:
    explicit StatusBarController(QMainWindow *window);
    void setCursorText(const QString &text);

private:
    QLabel *cursorLabel_;
};
