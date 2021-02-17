/*
 * Copyright (C) 2017 Alessandro Siniscalchi <asiniscalchi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

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
	mTelescope->move(180, speed());
}

void INDIControlWidget::onNorthButtonReleased()
{
	mTelescope->move(180, 0);
}

void INDIControlWidget::onEastButtonPressed()
{
	mTelescope->move(270, speed());
}

void INDIControlWidget::onEastButtonReleased()
{
	mTelescope->move(270, 0);
}

void INDIControlWidget::onSouthButtonPressed()
{
	mTelescope->move(0, speed());
}

void INDIControlWidget::onSouthButtonReleased()
{
	mTelescope->move(0, 0);
}

void INDIControlWidget::onWestButtonPressed()
{
	mTelescope->move(90, speed());
}

void INDIControlWidget::onWestButtonReleased()
{
	mTelescope->move(90, 0);
}

double INDIControlWidget::speed() const
{
	double speed = ui->speedSlider->value();
	speed /= ui->speedSlider->maximum();
	return speed;
}

