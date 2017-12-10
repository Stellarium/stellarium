#include "TelescopeClientINDIUI.hpp"
#include "ui_TelescopeClientINDIUI.h"

TelescopeClientINDIUI::TelescopeClientINDIUI(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TelescopeClientINDIUI)
{
    ui->setupUi(this);
}

TelescopeClientINDIUI::~TelescopeClientINDIUI()
{
    delete ui;
}
