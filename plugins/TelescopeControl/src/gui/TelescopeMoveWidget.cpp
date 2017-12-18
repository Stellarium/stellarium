#include "TelescopeMoveWidget.hpp"
#include "ui_TelescopeMoveWidget.h"

TelescopeMoveWidget::TelescopeMoveWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::TelescopeMoveWidget)
{
	ui->setupUi(this);

	QObject::connect(ui->northButton, &QPushButton::pressed, this, &TelescopeMoveWidget::moveNorthStart);
	QObject::connect(ui->eastButton, &QPushButton::pressed, this, &TelescopeMoveWidget::moveEastStart);
	QObject::connect(ui->southButton, &QPushButton::pressed, this, &TelescopeMoveWidget::moveSouthStart);
	QObject::connect(ui->westButton, &QPushButton::pressed, this, &TelescopeMoveWidget::moveWestStart);
	QObject::connect(ui->northButton, &QPushButton::released, this, &TelescopeMoveWidget::moveNorthStop);
	QObject::connect(ui->eastButton, &QPushButton::released, this, &TelescopeMoveWidget::moveEastStop);
	QObject::connect(ui->southButton, &QPushButton::released, this, &TelescopeMoveWidget::moveSouthStop);
	QObject::connect(ui->westButton, &QPushButton::released, this, &TelescopeMoveWidget::moveWestStop);
}

TelescopeMoveWidget::~TelescopeMoveWidget()
{
	delete ui;
}
