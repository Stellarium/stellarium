#include "INDIControlWidget.hpp"
#include "ui_INDIControlWidget.h"

INDIControlWidget::INDIControlWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::INDIControlWidget)
{
    ui->setupUi(this);

	QObject::connect(ui->northButton, &QPushButton::pressed, this, &INDIControlWidget::onNorthButtonPressed);
	QObject::connect(ui->northButton, &QPushButton::released, this, &INDIControlWidget::onNorthButtonReleased);
	QObject::connect(ui->eastButton, &QPushButton::pressed, this, &INDIControlWidget::onEastButtonPressed);
	QObject::connect(ui->eastButton, &QPushButton::released, this, &INDIControlWidget::onEastButtonReleased);
	QObject::connect(ui->southButton, &QPushButton::pressed, this, &INDIControlWidget::onSouthButtonPressed);
	QObject::connect(ui->southButton, &QPushButton::released, this, &INDIControlWidget::onSouthButtonReleased);
	QObject::connect(ui->westButton, &QPushButton::pressed, this, &INDIControlWidget::onWestButtonPressed);
	QObject::connect(ui->westButton, &QPushButton::released, this, &INDIControlWidget::onWestButtonReleased);
}

INDIControlWidget::~INDIControlWidget()
{
    delete ui;
}

void INDIControlWidget::onNorthButtonPressed()
{
	emit move(0, speed());
}

void INDIControlWidget::onNorthButtonReleased()
{
	emit move(0, 0);
}

void INDIControlWidget::onEastButtonPressed()
{
	emit move(90, speed());
}

void INDIControlWidget::onEastButtonReleased()
{
	emit move(90, 0);
}

void INDIControlWidget::onSouthButtonPressed()
{
	emit move(180, speed());
}

void INDIControlWidget::onSouthButtonReleased()
{
	emit move(180, 0);
}

void INDIControlWidget::onWestButtonPressed()
{
	emit move(270, speed());
}

void INDIControlWidget::onWestButtonReleased()
{
	emit move(270, 0);
}

double INDIControlWidget::speed() const
{
	double speed = ui->speedSlider->value();
	speed /= ui->speedSlider->maximum();
	return speed;
}

