#pragma once
#include <QDialog>
#include <QResizeEvent>

class StelDialogWindow : public QDialog
{
    Q_OBJECT
public:
    using QDialog::QDialog; // Inherit constructors

signals:
    void resized(QSizeF size);

protected:
    void resizeEvent(QResizeEvent* event) override {
        QDialog::resizeEvent(event);
        emit resized(event->size());
    }
};