/*
 * Stellarium Meteor Shower plugin config dialog
 *
 * Copyright (C) 2013 Marcos Cardinot
 * Copyright (C) 2011 Alexander Wolf
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

#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QUrl>
#include <QFileDialog>

#include "StelApp.hpp"
#include "ui_meteorShowerDialog.h"
#include "MeteorShowerDialog.hpp"
#include "MeteorShowers.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelStyle.hpp"
#include "StelGui.hpp"
#include "StelMainView.hpp"
#include "StelFileMgr.hpp"
#include "StelTranslator.hpp"

MeteorShowerDialog::MeteorShowerDialog() : updateTimer(NULL)
{
    ui = new Ui_meteorShowerDialog;
}

MeteorShowerDialog::~MeteorShowerDialog()
{
	if (updateTimer)
	{
		updateTimer->stop();
		delete updateTimer;
		updateTimer = NULL;
	}
	delete ui;
}

void MeteorShowerDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		refreshUpdateValues();
		setAboutHtml();
	}
}

// Initialize the dialog widgets and connect the signals/slots
void MeteorShowerDialog::createDialogContent()
{
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(languageChanged()));

	// Settings tab / updates group
	ui->displayModeCheckBox->setChecked(GETSTELMODULE(MeteorShowers)->getFlagShowMS());
	connect(ui->displayModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setDistributionEnabled(int)));
	ui->displayAtStartupCheckBox->setChecked(GETSTELMODULE(MeteorShowers)->getEnableAtStartup());
	connect(ui->displayAtStartupCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setDisplayAtStartupEnabled(int)));
	ui->displayShowMeteorShowerButton->setChecked(GETSTELMODULE(MeteorShowers)->getFlagShowMSButton());
	connect(ui->displayShowMeteorShowerButton, SIGNAL(stateChanged(int)), this, SLOT(setDisplayShowMeteorShowerButton(int)));
	connect(ui->internetUpdatesCheckbox, SIGNAL(stateChanged(int)), this, SLOT(setUpdatesEnabled(int)));
	connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(updateJSON()));
	connect(GETSTELMODULE(MeteorShowers), SIGNAL(updateStateChanged(MeteorShowers::UpdateState)), this, SLOT(updateStateReceiver(MeteorShowers::UpdateState)));
	connect(GETSTELMODULE(MeteorShowers), SIGNAL(jsonUpdateComplete(void)), this, SLOT(updateCompleteReceiver(void)));
	connect(ui->updateFrequencySpinBox, SIGNAL(valueChanged(int)), this, SLOT(setUpdateValues(int)));
	refreshUpdateValues(); // fetch values for last updated and so on
	// if the state didn't change, setUpdatesEnabled will not be called, so we force it
	setUpdatesEnabled(ui->internetUpdatesCheckbox->checkState());

	updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
	updateTimer->start(7000);

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
	connect(ui->saveSettingsButton, SIGNAL(clicked()), this, SLOT(saveSettings()));

	// About tab
	setAboutHtml();
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));

	updateGuiFromSettings();

}

void MeteorShowerDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Meteor Showers Plugin") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td>" + q_("Version:") + "</td><td>" + METEORSHOWERS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td>" + q_("Author:") + "</td><td>Marcos Cardinot &lt;mcardinot@gmail.com&gt;</td></tr></table>";

	html += "<p>" + q_("The Meteor Showers plugin show markers of radiants and information for general meteor showers.") + "</p>";
	html += "</body></html>";

	ui->aboutTextBrowser->setHtml(html);
}

void MeteorShowerDialog::refreshUpdateValues(void)
{
	ui->lastUpdateDateTimeEdit->setDateTime(GETSTELMODULE(MeteorShowers)->getLastUpdate());
	ui->updateFrequencySpinBox->setValue(GETSTELMODULE(MeteorShowers)->getUpdateFrequencyHours());
	int secondsToUpdate = GETSTELMODULE(MeteorShowers)->getSecondsToUpdate();
	ui->internetUpdatesCheckbox->setChecked(GETSTELMODULE(MeteorShowers)->getUpdatesEnabled());
	if (!GETSTELMODULE(MeteorShowers)->getUpdatesEnabled())
		ui->nextUpdateLabel->setText(q_("Internet updates disabled"));
	else if (GETSTELMODULE(MeteorShowers)->getUpdateState() == MeteorShowers::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (secondsToUpdate <= 60)
		ui->nextUpdateLabel->setText(q_("Next update: < 1 minute"));
	else if (secondsToUpdate < 3600)
		ui->nextUpdateLabel->setText(QString(q_("Next update: %1 minutes")).arg((secondsToUpdate/60)+1));
	else
		ui->nextUpdateLabel->setText(QString(q_("Next update: %1 hours")).arg((secondsToUpdate/3600)+1));
}

void MeteorShowerDialog::setUpdateValues(int hours)
{
	GETSTELMODULE(MeteorShowers)->setUpdateFrequencyHours(hours);
	refreshUpdateValues();
}

void MeteorShowerDialog::setUpdatesEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(MeteorShowers)->setUpdatesEnabled(b);
	ui->updateFrequencySpinBox->setEnabled(b);
	if(b)
		ui->updateButton->setText(q_("Update now"));
	else
		ui->updateButton->setText(q_("Update from files"));

	refreshUpdateValues();
}

void MeteorShowerDialog::setDistributionEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(MeteorShowers)->setFlagShowMS(b);
}

void MeteorShowerDialog::setDisplayAtStartupEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(MeteorShowers)->setEnableAtStartup(b);
}

void MeteorShowerDialog::setDisplayShowMeteorShowerButton(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(MeteorShowers)->setFlagShowMSButton(b);
}

void MeteorShowerDialog::updateStateReceiver(MeteorShowers::UpdateState state)
{
	//qDebug() << "MeteorShowerDialog::updateStateReceiver got a signal";
	if (state==MeteorShowers::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (state==MeteorShowers::DownloadError || state==MeteorShowers::OtherError)
	{
		ui->nextUpdateLabel->setText(q_("Update error"));
		updateTimer->start();  // make sure message is displayed for a while...
	}
}

void MeteorShowerDialog::updateCompleteReceiver(void)
{
        ui->nextUpdateLabel->setText(QString(q_("Meteor showers is updated")));
	// display the status for another full interval before refreshing status
	updateTimer->start();
	ui->lastUpdateDateTimeEdit->setDateTime(GETSTELMODULE(MeteorShowers)->getLastUpdate());
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
}

void MeteorShowerDialog::restoreDefaults(void)
{
	qDebug() << "MeteorShowers::restoreDefaults";
	GETSTELMODULE(MeteorShowers)->restoreDefaults();
	GETSTELMODULE(MeteorShowers)->readSettingsFromConfig();
	updateGuiFromSettings();
}

void MeteorShowerDialog::updateGuiFromSettings(void)
{
	ui->internetUpdatesCheckbox->setChecked(GETSTELMODULE(MeteorShowers)->getUpdatesEnabled());
	refreshUpdateValues();
}

void MeteorShowerDialog::saveSettings(void)
{
	GETSTELMODULE(MeteorShowers)->saveSettingsToConfig();
}

void MeteorShowerDialog::updateJSON(void)
{
	if(GETSTELMODULE(MeteorShowers)->getUpdatesEnabled())
	{
		GETSTELMODULE(MeteorShowers)->updateJSON();
	}
}
