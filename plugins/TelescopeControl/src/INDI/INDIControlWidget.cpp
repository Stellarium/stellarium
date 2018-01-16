#include "INDIControlWidget.hpp"
#include "ui_INDIControlWidget.h"

INDIControlWidget::INDIControlWidget(QSharedPointer<TelescopeClient> telescope, QWidget *parent) :
    QWidget(parent),
	ui(new Ui::INDIControlWidget),
	mTelescope(telescope)
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
	mTelescope->move(0, speed());
}

void INDIControlWidget::onNorthButtonReleased()
{
	mTelescope->move(0, 0);
}

void INDIControlWidget::onEastButtonPressed()
{
	mTelescope->move(90, speed());
}

void INDIControlWidget::onEastButtonReleased()
{
	mTelescope->move(90, 0);
}

void INDIControlWidget::onSouthButtonPressed()
{
	mTelescope->move(180, speed());
}

void INDIControlWidget::onSouthButtonReleased()
{
	mTelescope->move(180, 0);
}

void INDIControlWidget::onWestButtonPressed()
{
	mTelescope->move(270, speed());
}

void INDIControlWidget::onWestButtonReleased()
{
	mTelescope->move(270, 0);
}

double INDIControlWidget::speed() const
{
	double speed = ui->speedSlider->value();
	speed /= ui->speedSlider->maximum();
	return speed;
}

