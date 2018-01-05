#ifndef INDICONTROLWIDGET_HPP
#define INDICONTROLWIDGET_HPP

#include <QWidget>

namespace Ui {
class INDIControlWidget;
}

class INDIControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit INDIControlWidget(QWidget *parent = 0);
    ~INDIControlWidget();

private:
    Ui::INDIControlWidget *ui;
};

#endif // INDICONTROLWIDGET_HPP
