/*
 * Stellarium
 * Copyright (C) 2016 Alexander Wolf
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include <QDesktopServices>
#include <QUrl>
#include <QSettings>

#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "StelUtils.hpp"
#include "StelLocaleMgr.hpp"
#include "GreatRedSpotDialog.hpp"

#include "ui_greatRedSpotDialog.h"

GreatRedSpotDialog::GreatRedSpotDialog() : StelDialog("GreatRedSpot")
{
	ui = new Ui_GreatRedSpotDialogForm;
}

GreatRedSpotDialog::~GreatRedSpotDialog()
{
	delete ui;
	ui=Q_NULLPTR;
}

void GreatRedSpotDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void GreatRedSpotDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Signals and slots
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &GreatRedSpotDialog::retranslate);
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, &TitleBar::movedTo, this, &GreatRedSpotDialog::handleMovedTo);

	SolarSystem* ss = GETSTELMODULE(SolarSystem);
	connectIntProperty(ui->longitudeSpinBox, "SolarSystem.grsLongitude");
	connectDoubleProperty(ui->driftDoubleSpinBox, "SolarSystem.grsDrift");

	const StelLocaleMgr& locmgr = StelApp::getInstance().getLocaleMgr();
	QString fmt = QString("%1 hh:mm").arg(locmgr.getQtDateFormatStr());
	ui->jdDateTimeEdit->setDisplayFormat(fmt);
	ui->jdDateTimeEdit->setDateTime(StelUtils::jdToQDateTime(ss->getGrsJD(), Qt::UTC));
	connect(ui->jdDateTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &GreatRedSpotDialog::setGrsJD);

	connect(ui->recentGrsMeasurementPushButton, &QPushButton::clicked, this, &GreatRedSpotDialog::openRecentGrsMeasurement);
}

void GreatRedSpotDialog::setGrsJD(QDateTime dt)
{
	GETSTELMODULE(SolarSystem)->setGrsJD(StelUtils::qDateTimeToJd(dt));
}

void GreatRedSpotDialog::openRecentGrsMeasurement()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	QDesktopServices::openUrl(QUrl(conf->value("astro/grs_measurements_url", "https://jupos.hier-im-netz.de/rGrs.htm").toString()));
}
