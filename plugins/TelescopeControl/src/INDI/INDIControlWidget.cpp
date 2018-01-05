#include "INDIControlWidget.hpp"
#include "ui_INDIControlWidget.h"

INDIControlWidget::INDIControlWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::INDIControlWidget)
{
    ui->setupUi(this);
}

INDIControlWidget::~INDIControlWidget()
{
    delete ui;
}
