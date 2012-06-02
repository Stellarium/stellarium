/*
 * Stellarium Pulsars Plug-in GUI
 *
 * Copyright (C) 2012 Alexander Wolf
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
#include "ui_pulsarsDialog.h"
#include "PulsarsDialog.hpp"
#include "Pulsars.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelStyle.hpp"
#include "StelGui.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelFileMgr.hpp"
#include "StelTranslator.hpp"

PulsarsDialog::PulsarsDialog() : updateTimer(NULL)
{
	ui = new Ui_pulsarsDialog;
}

PulsarsDialog::~PulsarsDialog()
{
	if (updateTimer)
	{
		updateTimer->stop();
		delete updateTimer;
		updateTimer = NULL;
	}
	delete ui;
}

void PulsarsDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		refreshUpdateValues();
		setAboutHtml();
	}
}

// Initialize the dialog widgets and connect the signals/slots
void PulsarsDialog::createDialogContent()
{
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);	
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
		this, SLOT(retranslate()));

	// Settings tab / updates group
	ui->displayModeCheckBox->setChecked(GETSTELMODULE(Pulsars)->getDisplayMode());
	connect(ui->displayModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setDistributionEnabled(int)));
	connect(ui->internetUpdatesCheckbox, SIGNAL(stateChanged(int)), this, SLOT(setUpdatesEnabled(int)));
	connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(updateJSON()));
	connect(GETSTELMODULE(Pulsars), SIGNAL(updateStateChanged(Pulsars::UpdateState)), this, SLOT(updateStateReceiver(Pulsars::UpdateState)));
	connect(GETSTELMODULE(Pulsars), SIGNAL(jsonUpdateComplete(void)), this, SLOT(updateCompleteReceiver(void)));
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

void PulsarsDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Pulsars Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + PULSARS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td></tr>";
	html += "<tr width=\"30%\"><td><strong>" + q_("Homepage") + ":</strong></td><td><a href='http://stellarium.org/wiki/index.php/Pulsars_plugin'>http://stellarium.org/wiki/index.php/Pulsars_plugin</a></td></tr>";
	html += "</table>";

	html += "<p>" + q_("This plugin plots the position of various pulsars, with object information about each one.") + "</p>";
	html += "<p>" + QString(q_("Pulsar data is derived from 'The ATNF Pulsar Catalogue'  (Manchester, R. N., Hobbs, G. B., Teoh, A. & Hobbs, M., Astron. J., 129, 1993-2006 (2005) (%1astro-ph/0412641%2))."))
			.arg("<a href='http://arxiv.org/abs/astro-ph/0412641'>")
			.arg("</a>") + "</p>";
	html += "<p>" + QString("<strong>%1:</strong> %2")
			.arg(q_("Note"))
			.arg(q_("pulsar identifiers have the prefix 'PSR'")) + "</p>";
	html += "</body></html>";

	ui->aboutTextBrowser->setHtml(html);
}

void PulsarsDialog::refreshUpdateValues(void)
{
	ui->lastUpdateDateTimeEdit->setDateTime(GETSTELMODULE(Pulsars)->getLastUpdate());
	ui->updateFrequencySpinBox->setValue(GETSTELMODULE(Pulsars)->getUpdateFrequencyHours());
	int secondsToUpdate = GETSTELMODULE(Pulsars)->getSecondsToUpdate();
	ui->internetUpdatesCheckbox->setChecked(GETSTELMODULE(Pulsars)->getUpdatesEnabled());
	if (!GETSTELMODULE(Pulsars)->getUpdatesEnabled())
		ui->nextUpdateLabel->setText(q_("Internet updates disabled"));
	else if (GETSTELMODULE(Pulsars)->getUpdateState() == Pulsars::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (secondsToUpdate <= 60)
		ui->nextUpdateLabel->setText(q_("Next update: < 1 minute"));
	else if (secondsToUpdate < 3600)
		ui->nextUpdateLabel->setText(QString(q_("Next update: %1 minutes")).arg((secondsToUpdate/60)+1));
	else
		ui->nextUpdateLabel->setText(QString(q_("Next update: %1 hours")).arg((secondsToUpdate/3600)+1));
}

void PulsarsDialog::setUpdateValues(int hours)
{
	GETSTELMODULE(Pulsars)->setUpdateFrequencyHours(hours);
	refreshUpdateValues();
}

void PulsarsDialog::setUpdatesEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Pulsars)->setUpdatesEnabled(b);
	ui->updateFrequencySpinBox->setEnabled(b);
	if(b)
		ui->updateButton->setText(q_("Update now"));
	else
		ui->updateButton->setText(q_("Update from files"));

	refreshUpdateValues();
}

void PulsarsDialog::setDistributionEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Pulsars)->setDisplayMode(b);
}

void PulsarsDialog::updateStateReceiver(Pulsars::UpdateState state)
{
	//qDebug() << "PulsarsDialog::updateStateReceiver got a signal";
	if (state==Pulsars::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (state==Pulsars::DownloadError || state==Pulsars::OtherError)
	{
		ui->nextUpdateLabel->setText(q_("Update error"));
		updateTimer->start();  // make sure message is displayed for a while...
	}
}

void PulsarsDialog::updateCompleteReceiver(void)
{
	ui->nextUpdateLabel->setText(QString(q_("Pulsars is updated")));
	// display the status for another full interval before refreshing status
	updateTimer->start();
	ui->lastUpdateDateTimeEdit->setDateTime(GETSTELMODULE(Pulsars)->getLastUpdate());
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
}

void PulsarsDialog::restoreDefaults(void)
{
	qDebug() << "Pulsars::restoreDefaults";
	GETSTELMODULE(Pulsars)->restoreDefaults();
	GETSTELMODULE(Pulsars)->readSettingsFromConfig();
	updateGuiFromSettings();
}

void PulsarsDialog::updateGuiFromSettings(void)
{
	ui->internetUpdatesCheckbox->setChecked(GETSTELMODULE(Pulsars)->getUpdatesEnabled());
	refreshUpdateValues();
}

void PulsarsDialog::saveSettings(void)
{
	GETSTELMODULE(Pulsars)->saveSettingsToConfig();
}

void PulsarsDialog::updateJSON(void)
{
	if(GETSTELMODULE(Pulsars)->getUpdatesEnabled())
	{
		GETSTELMODULE(Pulsars)->updateJSON();
	}
}
