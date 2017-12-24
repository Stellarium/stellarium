#include "TelescopeMoveWidget.hpp"
#include "ui_TelescopeMoveWidget.h"

TelescopeMoveWidget::TelescopeMoveWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::TelescopeMoveWidget)
{
	ui->setupUi(this);

	QObject::connect(ui->northButton, &QPushButton::pressed, this, &TelescopeMoveWidget::onNorthButtonPressed);
	QObject::connect(ui->northButton, &QPushButton::released, this, &TelescopeMoveWidget::onNorthButtonReleased);
	QObject::connect(ui->eastButton, &QPushButton::pressed, this, &TelescopeMoveWidget::onEastButtonPressed);
	QObject::connect(ui->eastButton, &QPushButton::released, this, &TelescopeMoveWidget::onEastButtonReleased);
	QObject::connect(ui->southButton, &QPushButton::pressed, this, &TelescopeMoveWidget::onSouthButtonPressed);
	QObject::connect(ui->southButton, &QPushButton::released, this, &TelescopeMoveWidget::onSouthButtonReleased);
	QObject::connect(ui->westButton, &QPushButton::pressed, this, &TelescopeMoveWidget::onWestButtonPressed);
	QObject::connect(ui->westButton, &QPushButton::released, this, &TelescopeMoveWidget::onWestButtonReleased);
}

TelescopeMoveWidget::~TelescopeMoveWidget()
{
	delete ui;
}

void TelescopeMoveWidget::onNorthButtonPressed()
{
	emit move(0, speed());
}

void TelescopeMoveWidget::onNorthButtonReleased()
{
	emit move(0, 0);
}

void TelescopeMoveWidget::onEastButtonPressed()
{
	emit move(90, speed());
}

void TelescopeMoveWidget::onEastButtonReleased()
{
	emit move(90, 0);
}

void TelescopeMoveWidget::onSouthButtonPressed()
{
	emit move(180, speed());
}

void TelescopeMoveWidget::onSouthButtonReleased()
{
	emit move(180, 0);
}

void TelescopeMoveWidget::onWestButtonPressed()
{
	emit move(270, speed());
}

void TelescopeMoveWidget::onWestButtonReleased()
{
	emit move(270, 0);
}

double TelescopeMoveWidget::speed() const
{
	double speed = ui->speedSlider->value();
	speed /= ui->speedSlider->maximum();
	return speed;
}

