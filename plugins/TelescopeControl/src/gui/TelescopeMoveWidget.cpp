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
	QObject::connect(ui->speedSlider, &QSlider::valueChanged, this, &TelescopeMoveWidget::setSpeed);
}

TelescopeMoveWidget::~TelescopeMoveWidget()
{
	delete ui;
}

void TelescopeMoveWidget::onNorthButtonPressed()
{
	emit moveNorth(true);
}

void TelescopeMoveWidget::onNorthButtonReleased()
{
	emit moveNorth(false);
}

void TelescopeMoveWidget::onEastButtonPressed()
{
	emit moveEast(true);
}

void TelescopeMoveWidget::onEastButtonReleased()
{
	emit moveEast(false);
}

void TelescopeMoveWidget::onSouthButtonPressed()
{
	emit moveSouth(true);
}

void TelescopeMoveWidget::onSouthButtonReleased()
{
	emit moveSouth(false);
}

void TelescopeMoveWidget::onWestButtonPressed()
{
	emit moveWest(true);
}

void TelescopeMoveWidget::onWestButtonReleased()
{
	emit moveWest(false);
}

