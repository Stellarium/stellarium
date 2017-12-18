#include "TelescopeMoveWidget.hpp"
#include "ui_TelescopeMoveWidget.h"

TelescopeMoveWidget::TelescopeMoveWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TelescopeMoveWidget)
{
    ui->setupUi(this);
}

TelescopeMoveWidget::~TelescopeMoveWidget()
{
    delete ui;
}
