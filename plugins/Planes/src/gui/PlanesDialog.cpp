/*
 * Copyright (C) 2013 Felix Zeltner
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

#include "PlanesDialog.hpp"
#include "ui_PlanesDialog.h"
#include "StelApp.hpp"
#include <QFileDialog>
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelActionMgr.hpp"
#include "StelMainView.hpp"
#include "Planes.hpp"
#include "Flight.hpp"
#include "FlightMgr.hpp"

PlanesDialog::PlanesDialog()
{
	ui = new Ui::PlanesDialog();
	cachedDBStatus = QStringLiteral("Disconnected");
	cachedBSStatus = QStringLiteral("Disconnected");
}

PlanesDialog::~PlanesDialog()
{
	delete ui;
}

void PlanesDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void PlanesDialog::setDBStatus(QString status)
{
	if (ui->dbStatus)
	{
		ui->dbStatus->setText(status);
	}
	else
	{
		cachedDBStatus = status;
	}
}

void PlanesDialog::setBSStatus(QString status)
{
	if (ui->bsStatus)
	{
		ui->bsStatus->setText(status);
	}
	else
	{
		cachedBSStatus = status;
	}
}

void PlanesDialog::createDialogContent()
{
	ui->setupUi(dialog);
	ui->dbDriver->addItem(QStringLiteral("MySQL"), QStringLiteral("QMYSQL"));
	ui->dbDriver->addItem(QStringLiteral("ODBC"), QStringLiteral("QODBC"));
	ui->dbDriver->addItem(QStringLiteral("SQLite"), QStringLiteral("QSQLITE"));
	Planes *planes = GETSTELMODULE(Planes);

	ui->databaseGroup->setChecked(planes->isUsingDB());
	ui->BSGroup->setChecked(planes->isUsingBS());
	ui->useBSDB->setChecked(planes->isDumpingOldFlights());
	ui->useBSDB->setEnabled(planes->isUsingDB() && planes->isUsingBS());
	ui->connectOnStartup->setChecked(planes->isConnectOnStartupEnabled());

	DBCredentials c = planes->getDBCreds();
	for (int i = 0; i < ui->dbDriver->count(); ++i)
	{
		if (ui->dbDriver->itemData(i).toString() == c.type)
		{
			ui->dbDriver->setCurrentIndex(i);
			updateDBFields();
			break;
		}
	}
	ui->dbHost->setText(c.host);
	ui->dbPort->setText(QString::number(c.port));
	ui->dbUser->setText(c.user);
	ui->dbPass->setText(c.pass);
	ui->dbName->setText(c.name);

	ui->useInterp->setChecked(planes->getFlightMgr()->isInterpEnabled());
	ui->showLabels->setChecked(planes->getFlightMgr()->isLabelsVisible());
	Flight::PathDrawMode showTrails = planes->getFlightMgr()->getPathDrawMode();
	ui->trails_nothing->setChecked(showTrails == Flight::NoPaths);
	ui->trails_selected->setChecked(showTrails == Flight::SelectedOnly);
	ui->trails_inView->setChecked(showTrails == Flight::InViewOnly);
	ui->trails_all->setChecked(showTrails == Flight::AllPaths);
	Flight::PathColour trailCol = planes->getFlightMgr()->getPathColourMode();
	ui->col_solid->setChecked(trailCol == Flight::SolidColour);
	ui->col_height->setChecked(trailCol == Flight::EncodeHeight);
	ui->col_velocity->setChecked(trailCol == Flight::EncodeVelocity);

	ui->maxHeight->setValue(Flight::getMaxHeight());
	ui->minHeight->setValue(Flight::getMinHeight());
	ui->maxVel->setValue(Flight::getMaxVelocity());
	ui->minVel->setValue(Flight::getMinVelocity());
	ui->maxVertRate->setValue(Flight::getMaxVertRate());
	ui->minVertRate->setValue(Flight::getMinVertRate());

	ui->bsHost->setText(planes->getBSHost());
	ui->bsPort->setText(QString::number((int)planes->getBSPort()));
	ui->useBSDB->setChecked(planes->isDumpingOldFlights());

	ui->bsStatus->setText(cachedBSStatus);
	ui->dbStatus->setText(cachedDBStatus);

	this->connect(ui->closeStelWindow, SIGNAL(clicked()), SLOT(close()));
	this->connect(&StelApp::getInstance(), SIGNAL(languageChanged()), SLOT(retranslate()));
	this->connect(ui->col_solid, SIGNAL(clicked()), SLOT(setSolidCol()));
	this->connect(ui->col_height, SIGNAL(clicked()), SLOT(setHeightCol()));
	this->connect(ui->col_velocity, SIGNAL(clicked()), SLOT(setVelocityCol()));

	this->connect(ui->trails_nothing, SIGNAL(clicked()), SLOT(setNoPaths()));
	this->connect(ui->trails_selected, SIGNAL(clicked()), SLOT(setSelectedPath()));
	this->connect(ui->trails_inView, SIGNAL(clicked()), SLOT(setInviewPaths()));
	this->connect(ui->trails_all, SIGNAL(clicked()), SLOT(setAllPaths()));

	this->connect(ui->minHeight, SIGNAL(valueChanged(double)), SLOT(setMinHeight(double)));
	this->connect(ui->maxHeight, SIGNAL(valueChanged(double)), SLOT(setMaxHeight(double)));
	this->connect(ui->minVel, SIGNAL(valueChanged(double)), SLOT(setMinVel(double)));
	this->connect(ui->maxVel, SIGNAL(valueChanged(double)), SLOT(setMaxVel(double)));
	this->connect(ui->minVertRate, SIGNAL(valueChanged(double)), SLOT(setMinVertRate(double)));
	this->connect(ui->maxVertRate, SIGNAL(valueChanged(double)), SLOT(setMaxVertRate(double)));

	this->connect(ui->importFileButton, SIGNAL(clicked()), SLOT(openFile()));

	this->connect(ui->showLabels, SIGNAL(clicked(bool)), SLOT(setLabels(bool)));
	this->connect(ui->useInterp, SIGNAL(clicked(bool)), SLOT(setInterp(bool)));
	this->connect(ui->dbConnectButton, SIGNAL(clicked()), SLOT(connectDB()));
	this->connect(ui->dbDisconnectButton, SIGNAL(clicked()), SLOT(disconnectDB()));

	this->connect(ui->databaseGroup, SIGNAL(clicked()), SLOT(setUseDB()));
	this->connect(ui->BSGroup, SIGNAL(clicked()), SLOT(setUseBS()));

	this->connect(ui->dbDriver, SIGNAL(currentIndexChanged(int)), SLOT(updateCreds()));
	this->connect(ui->dbHost, SIGNAL(textChanged(QString)), SLOT(updateCreds()));
	this->connect(ui->dbPort, SIGNAL(textChanged(QString)), SLOT(updateCreds()));
	this->connect(ui->dbUser, SIGNAL(textChanged(QString)), SLOT(updateCreds()));
	this->connect(ui->dbPass, SIGNAL(textChanged(QString)), SLOT(updateCreds()));
	this->connect(ui->dbName, SIGNAL(textChanged(QString)), SLOT(updateCreds()));

	this->connect(ui->bsHost, SIGNAL(textChanged(QString)), SLOT(setBSHost(QString)));
	this->connect(ui->bsPort, SIGNAL(textChanged(QString)), SLOT(setBSPort(QString)));
	this->connect(ui->useBSDB, SIGNAL(clicked(bool)), SLOT(setBSUseDB(bool)));
	this->connect(ui->bsConnectButton, SIGNAL(clicked()), SLOT(connectBS()));
	this->connect(ui->bsDisconnectButton, SIGNAL(clicked()), SLOT(disconnectBS()));

	this->connect(ui->connectOnStartup, SIGNAL(clicked()), SLOT(setConnectOnStartup()));
}

void PlanesDialog::updateDBFields()
{
	bool isSqLite = (ui->dbDriver->itemData(ui->dbDriver->currentIndex()).toString() == QStringLiteral("QSQLITE"));
	ui->dbHost->setHidden(isSqLite);
	ui->dbHostLabel->setHidden(isSqLite);
	ui->dbPort->setHidden(isSqLite);
	ui->dbPortLabel->setHidden(isSqLite);
	ui->dbUser->setHidden(isSqLite);
	ui->dbUserLabel->setHidden(isSqLite);
	ui->dbPass->setHidden(isSqLite);
	ui->dbPasswordLabel->setHidden(isSqLite);
	ui->dbNameLabel->setText(isSqLite ? QStringLiteral("File") : QStringLiteral("Database"));
}

void PlanesDialog::openFile()
{
	QString filePath = QFileDialog::getOpenFileName(&StelMainView::getInstance(), QStringLiteral("Open BaseStation Recording"), QStringLiteral("."), QStringLiteral("BaseStation Recordings (*.bst)"));
	if (!filePath.isNull())
	{
		emit fileSelected(filePath);
	}
}

void PlanesDialog::setUseDB()
{
	emit dbUsedSet(ui->databaseGroup->isChecked());
	if (!ui->databaseGroup->isChecked())
	{
		ui->useBSDB->setChecked(false);
		ui->useBSDB->setEnabled(false);
		emit bsUseDBChanged(false);
	}
	else
	{
		ui->useBSDB->setEnabled(ui->BSGroup->isChecked());
	}
}

void PlanesDialog::updateCreds()
{
	DBCredentials c;
	c.type = ui->dbDriver->itemData(ui->dbDriver->currentIndex()).toString();
	updateDBFields();
	c.host = ui->dbHost->text();
	c.port = ui->dbPort->text().toInt();
	c.user = ui->dbUser->text();
	c.pass = ui->dbPass->text();
	c.name = ui->dbName->text();
	emit credentialsChanged(c);
}

void PlanesDialog::setUseBS()
{
	emit bsUsedSet(ui->BSGroup->isChecked());
	ui->useBSDB->setEnabled(ui->databaseGroup->isChecked());
}

void PlanesDialog::setConnectOnStartup()
{
	emit connectOnStartupChanged(ui->connectOnStartup->isChecked());
}
