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
	emit moveNorth(speed());
}

void TelescopeMoveWidget::onNorthButtonReleased()
{
	emit moveNorth(0);
}

void TelescopeMoveWidget::onEastButtonPressed()
{
	emit moveEast(speed());
}

void TelescopeMoveWidget::onEastButtonReleased()
{
	emit moveEast(0);
}

void TelescopeMoveWidget::onSouthButtonPressed()
{
	emit moveSouth(speed());
}

void TelescopeMoveWidget::onSouthButtonReleased()
{
	emit moveSouth(0);
}

void TelescopeMoveWidget::onWestButtonPressed()
{
	emit moveWest(speed());
}

void TelescopeMoveWidget::onWestButtonReleased()
{
	emit moveWest(0);
}

int TelescopeMoveWidget::speed() const
{
	return ui->speedSlider->value();
}

