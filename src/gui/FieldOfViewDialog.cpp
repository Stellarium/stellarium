/*
 * Stellarium
 * Copyright (C) 2023 Alexander Wolf
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

#include "Dialog.hpp"
#include "FieldOfViewDialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelMovementMgr.hpp"
#include "StelUtils.hpp"

#include "ui_fieldOfViewDialogGui.h"

FieldOfViewDialog::FieldOfViewDialog(QObject* parent)
	: StelDialog("FieldOfView", parent)
	, ui(new Ui_fieldOfViewDialogForm)
	, core(StelApp::getInstance().getCore())
	, degree(0)
	, minute(0)
	, second(0)
	, fov(0.)
{
//
}

FieldOfViewDialog::~FieldOfViewDialog()
{
	delete ui;
	ui=Q_NULLPTR;
}

void FieldOfViewDialog::createDialogContent()
{
	ui->setupUi(dialog);
	setFieldOfView(getFieldOfView());

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connectSpinnerEvents();

	connect(ui->spinner_second, &ExternalStepSpinBox::stepsRequested, this, [this](int steps){secondChanged(second+steps);});
	connect(ui->spinner_minute, &ExternalStepSpinBox::stepsRequested, this, [this](int steps){minuteChanged(minute+steps);});
	connect(ui->spinner_degree, &ExternalStepSpinBox::stepsRequested, this, [this](int steps){degreeChanged(degree+steps);});

	//ui->spinner_degree->setFocus();
	ui->spinner_degree->setSuffix("°");
	ui->spinner_decimal->setSuffix("°");
	ui->spinner_minute->setSuffix("′");
	ui->spinner_second->setSuffix("″");

	connect(core, SIGNAL(currentProjectionTypeKeyChanged(QString)), this, SLOT(setFieldOfViewLimit(QString)));
	connect(core->getMovementMgr(), SIGNAL(fovChagned(double)), this, SLOT(setFieldOfView(double)));
	setFieldOfViewLimit(QString());
}

void FieldOfViewDialog::connectSpinnerEvents() const
{
	connect(ui->spinner_degree, SIGNAL(valueChanged(int)), this, SLOT(degreeChanged(int)));
	connect(ui->spinner_minute, SIGNAL(valueChanged(int)), this, SLOT(minuteChanged(int)));
	connect(ui->spinner_second, SIGNAL(valueChanged(int)), this, SLOT(secondChanged(int)));	
	connect(ui->spinner_decimal, SIGNAL(valueChanged(double)), this, SLOT(fovChanged(double)));
}

void FieldOfViewDialog::disconnectSpinnerEvents()const
{
	disconnect(ui->spinner_degree, SIGNAL(valueChanged(int)), this, SLOT(degreeChanged(int)));
	disconnect(ui->spinner_minute, SIGNAL(valueChanged(int)), this, SLOT(minuteChanged(int)));
	disconnect(ui->spinner_second, SIGNAL(valueChanged(int)), this, SLOT(secondChanged(int)));
	disconnect(ui->spinner_decimal, SIGNAL(valueChanged(double)), this, SLOT(fovChanged(double)));
}

void FieldOfViewDialog::setFieldOfViewLimit(QString)
{
	double maxFov = core->getProjection(StelCore::FrameJ2000)->getMaxFov();
	ui->spinner_degree->setMaximum(qRound(maxFov));
	ui->spinner_decimal->setMaximum(maxFov);
}

bool FieldOfViewDialog::makeValidAndApply(int d, int m, int s)
{
	if (!StelUtils::changeFieldOfViewForRollover(d, m, s, &degree, &minute, &second)) {
		degree = d;
		minute = m;
		second = s;
		fov = degree + minute/60. + second/3600.;
	}

	return applyFieldOfView(fov);
}

bool FieldOfViewDialog::applyFieldOfView(double fov)
{
	pushToWidgets();
	core->getMovementMgr()->zoomTo(fov, 0.f);
	return true;
}

void FieldOfViewDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);	
}

void FieldOfViewDialog::close()
{
	StelDialog::close();
}

void FieldOfViewDialog::fovChanged(double nd)
{
	setFieldOfView(nd);
	core->getMovementMgr()->zoomTo(nd, 0.f);
}

void FieldOfViewDialog::degreeChanged(int newdegree)
{
	if ( degree != newdegree )
		makeValidAndApply( newdegree, minute, second );
}

void FieldOfViewDialog::minuteChanged(int newminute)
{
	if ( minute != newminute )
		makeValidAndApply( degree, newminute, second );
}

void FieldOfViewDialog::secondChanged(int newsecond)
{
	if ( second != newsecond )
		makeValidAndApply( degree, minute, newsecond );
}

double FieldOfViewDialog::getFieldOfView()
{
	return core->getMovementMgr()->getCurrentFov();
}

void FieldOfViewDialog::pushToWidgets()
{
	disconnectSpinnerEvents();

	// Don't touch spinboxes that don't change. Otherwise it interferes with
	// typing (e.g. when starting a number with leading zero), and sometimes
	// even with stepping (e.g. the user clicks an arrow, but the number
	// remains the same, although the time did change).
	if(ui->spinner_degree->value() != degree)
		ui->spinner_degree->setValue(degree);
	if(ui->spinner_minute->value() != minute)
		ui->spinner_minute->setValue(minute);
	if(ui->spinner_second->value() != second)
		ui->spinner_second->setValue(second);
	if(ui->spinner_decimal->value() != fov)
		ui->spinner_decimal->setValue(fov);

	connectSpinnerEvents();
}

void FieldOfViewDialog::setFieldOfView(double newFOV)
{
	bool sign;
	double s;
	unsigned int d, m;
	StelUtils::decDegToDms(newFOV, sign, d, m, s);

	degree = d;
	minute = m;
	second = qRound(s);
	fov = newFOV;

	pushToWidgets();
}
