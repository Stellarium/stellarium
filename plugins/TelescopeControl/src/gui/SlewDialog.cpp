/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2010 Bogdan Marinov (this file)
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
#include "AngleSpinBox.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelStyle.hpp"
#include "StelUtils.hpp"
#include "VecMath.hpp"
#include "TelescopeControl.hpp"
#include "SlewDialog.hpp"
#include "ui_slewDialog.h"

#include <QDebug>

using namespace TelescopeControlGlobals;


SlewDialog::SlewDialog()
{
	ui = new Ui_slewDialog;
	
	//TODO: Fix this - it's in the same plugin
	telescopeManager = GETSTELMODULE(TelescopeControl);
}

SlewDialog::~SlewDialog()
{	
	delete ui;
}

void SlewDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void SlewDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Inherited connect
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(ui->radioButtonHMS, SIGNAL(toggled(bool)), this, SLOT(setFormatHMS(bool)));
	connect(ui->radioButtonDMS, SIGNAL(toggled(bool)), this, SLOT(setFormatDMS(bool)));
	connect(ui->radioButtonDecimal, SIGNAL(toggled(bool)), this, SLOT(setFormatDecimal(bool)));

	connect(ui->pushButtonSlew, SIGNAL(clicked()), this, SLOT(slew()));
	connect(ui->pushButtonConfigure, SIGNAL(clicked()), this, SLOT(showConfiguration()));

	connect(telescopeManager, SIGNAL(clientConnected(int, QString)), this, SLOT(addTelescope(int, QString)));
	connect(telescopeManager, SIGNAL(clientDisconnected(int)), this, SLOT(removeTelescope(int)));
        //
        connect(ui->pushButtonCurrent,SIGNAL(clicked()),this,SLOT(getCurrentObjectInfo()));
	//Coordinates are in HMS by default:
	ui->radioButtonHMS->setChecked(true);

	updateTelescopeList();
}

void SlewDialog::showConfiguration()
{
	//Hack to work around having no direct way to do display the window
	telescopeManager->configureGui(true);
}

void SlewDialog::setFormatHMS(bool set)
{
	if (!set) return;
	ui->spinBoxRA->setDisplayFormat(AngleSpinBox::HMSLetters);
	ui->spinBoxDec->setDisplayFormat(AngleSpinBox::DMSLetters);
}

void SlewDialog::setFormatDMS(bool set)
{
	if (!set) return;
	ui->spinBoxRA->setDisplayFormat(AngleSpinBox::DMSLetters);
	ui->spinBoxDec->setDisplayFormat(AngleSpinBox::DMSLetters);
}

void SlewDialog::setFormatDecimal(bool set)
{
	if (!set) return;
	ui->spinBoxRA->setDisplayFormat(AngleSpinBox::DecimalDeg);
	ui->spinBoxDec->setDisplayFormat(AngleSpinBox::DecimalDeg);
}

void SlewDialog::updateTelescopeList()
{
	connectedSlotsByName.clear();
	ui->comboBoxTelescope->clear();

	QHash<int, QString> connectedSlotsByNumber = telescopeManager->getConnectedClientsNames();
	foreach(const int slot, connectedSlotsByNumber.keys())
	{
		QString telescopeName = connectedSlotsByNumber.value(slot);
		connectedSlotsByName.insert(telescopeName, slot);
		ui->comboBoxTelescope->addItem(telescopeName);
	}
	
	updateTelescopeControls();
}

void SlewDialog::updateTelescopeControls()
{
	bool connectedTelescopeAvailable = !connectedSlotsByName.isEmpty();
	ui->groupBoxSlew->setVisible(connectedTelescopeAvailable);
	ui->labelNoTelescopes->setVisible(!connectedTelescopeAvailable);
	if (connectedTelescopeAvailable)
		ui->comboBoxTelescope->setCurrentIndex(0);
}

void SlewDialog::addTelescope(int slot, QString name)
{
	if (slot <=0 || name.isEmpty())
		return;

	connectedSlotsByName.insert(name, slot);
	ui->comboBoxTelescope->addItem(name);

	updateTelescopeControls();
}

void SlewDialog::removeTelescope(int slot)
{
	if (slot <= 0)
		return;

	QString name = connectedSlotsByName.key(slot, QString());
	if (name.isEmpty())
		return;
	connectedSlotsByName.remove(name);

	int index = ui->comboBoxTelescope->findText(name);
	if (index != -1)
	{
		ui->comboBoxTelescope->removeItem(index);
	}
	else
	{
		//Something very wrong just happened, so:
		updateTelescopeList();
		return;
	}

	updateTelescopeControls();
}

void SlewDialog::slew()
{
	double radiansRA = ui->spinBoxRA->valueRadians();
	double radiansDec = ui->spinBoxDec->valueRadians();
	int slot = connectedSlotsByName.value(ui->comboBoxTelescope->currentText());

	Vec3d targetPosition;
	StelUtils::spheToRect(radiansRA, radiansDec, targetPosition);

	telescopeManager->telescopeGoto(slot, targetPosition);
}

void SlewDialog::getCurrentObjectInfo(){
    const QList<StelObjectP>& selected = GETSTELMODULE(StelObjectMgr)->getSelectedObject();
    if (!selected.isEmpty()) {
        double dec_j2000 = 0;
        double ra_j2000 = 0;
        StelUtils::rectToSphe(&ra_j2000,&dec_j2000,selected[0]->getJ2000EquatorialPos(StelApp::getInstance().getCore()));
        ui->spinBoxRA->setRadians(ra_j2000);
        ui->spinBoxDec->setRadians(dec_j2000);
    }
}

