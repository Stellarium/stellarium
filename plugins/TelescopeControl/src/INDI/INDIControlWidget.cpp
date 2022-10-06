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

	QObject::connect(ui->northButton, &QPushButton::pressed,  this, [=](){mTelescope->move(180, speed());});
	QObject::connect(ui->northButton, &QPushButton::released, this, [=](){mTelescope->move(180, 0);});
	QObject::connect(ui->eastButton,  &QPushButton::pressed,  this, [=](){mTelescope->move(270, speed());});
	QObject::connect(ui->eastButton,  &QPushButton::released, this, [=](){mTelescope->move(270, 0);});
	QObject::connect(ui->southButton, &QPushButton::pressed,  this, [=](){mTelescope->move(0, speed());});
	QObject::connect(ui->southButton, &QPushButton::released, this, [=](){mTelescope->move(0, 0);});
	QObject::connect(ui->westButton,  &QPushButton::pressed,  this, [=](){mTelescope->move(90, speed());});
	QObject::connect(ui->westButton,  &QPushButton::released, this, [=](){mTelescope->move(90, 0);});
}

INDIControlWidget::~INDIControlWidget()
{
	delete ui;
}

double INDIControlWidget::speed() const
{
	double speed = ui->speedSlider->value();
	speed /= ui->speedSlider->maximum();
	return speed;
}

