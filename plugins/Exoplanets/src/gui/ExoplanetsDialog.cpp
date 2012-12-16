/*
 * Stellarium Exoplanets Plug-in GUI
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
#include "ui_exoplanetsDialog.h"
#include "ExoplanetsDialog.hpp"
#include "Exoplanets.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelStyle.hpp"
#include "StelGui.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelFileMgr.hpp"
#include "StelTranslator.hpp"

ExoplanetsDialog::ExoplanetsDialog() : updateTimer(NULL)
{
        ui = new Ui_exoplanetsDialog;
}

ExoplanetsDialog::~ExoplanetsDialog()
{
	if (updateTimer)
	{
		updateTimer->stop();
		delete updateTimer;
		updateTimer = NULL;
	}
	delete ui;
}

void ExoplanetsDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		refreshUpdateValues();
		setAboutHtml();
	}
}

// Initialize the dialog widgets and connect the signals/slots
void ExoplanetsDialog::createDialogContent()
{
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);	
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
		this, SLOT(retranslate()));

	// Settings tab / updates group
	ui->displayModeCheckBox->setChecked(GETSTELMODULE(Exoplanets)->getDisplayMode());
	connect(ui->displayModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setDistributionEnabled(int)));
	ui->timelineModeCheckBox->setChecked(GETSTELMODULE(Exoplanets)->getTimelineMode());
	connect(ui->timelineModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setTimelineEnabled(int)));
	connect(ui->internetUpdatesCheckbox, SIGNAL(stateChanged(int)), this, SLOT(setUpdatesEnabled(int)));
	connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(updateJSON()));
	connect(GETSTELMODULE(Exoplanets), SIGNAL(updateStateChanged(Exoplanets::UpdateState)), this, SLOT(updateStateReceiver(Exoplanets::UpdateState)));
	connect(GETSTELMODULE(Exoplanets), SIGNAL(jsonUpdateComplete(void)), this, SLOT(updateCompleteReceiver(void)));
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

void ExoplanetsDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Exoplanets Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + EXOPLANETS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td></tr></table>";

	html += "<p>" + QString(q_("This plugin plots the position of stars with exoplanets. Exoplanets data is derived from \"%1The Extrasolar Planets Encyclopaedia%2\"")).arg("<a href=\"http://exoplanet.eu/\">").arg("</a>") + "</p>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Launchpad website.  Be sure to put \"%1\" in the subject when posting.")).arg("Exoplanets plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("If you have a question, you can %1get an answer here%2").arg("<a href=\"https://answers.launchpad.net/stellarium\">")).arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("Bug reports can be made %1here%2.")).arg("<a href=\"https://bugs.launchpad.net/stellarium\">").arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you would like to make a feature request, you can create a bug report, and set the severity to \"wishlist\".") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you want read full information about plugin, his history and format of catalog you can %1get info here%2.").arg("<a href=\"http://stellarium.org/wiki/index.php/Exoplanets_plugin\">").arg("</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
	ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	ui->aboutTextBrowser->setHtml(html);
}

void ExoplanetsDialog::refreshUpdateValues(void)
{
	ui->lastUpdateDateTimeEdit->setDateTime(GETSTELMODULE(Exoplanets)->getLastUpdate());
	ui->updateFrequencySpinBox->setValue(GETSTELMODULE(Exoplanets)->getUpdateFrequencyHours());
	int secondsToUpdate = GETSTELMODULE(Exoplanets)->getSecondsToUpdate();
	ui->internetUpdatesCheckbox->setChecked(GETSTELMODULE(Exoplanets)->getUpdatesEnabled());
	if (!GETSTELMODULE(Exoplanets)->getUpdatesEnabled())
		ui->nextUpdateLabel->setText(q_("Internet updates disabled"));
	else if (GETSTELMODULE(Exoplanets)->getUpdateState() == Exoplanets::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (secondsToUpdate <= 60)
		ui->nextUpdateLabel->setText(q_("Next update: < 1 minute"));
	else if (secondsToUpdate < 3600)
		ui->nextUpdateLabel->setText(QString(q_("Next update: %1 minutes")).arg((secondsToUpdate/60)+1));
	else
		ui->nextUpdateLabel->setText(QString(q_("Next update: %1 hours")).arg((secondsToUpdate/3600)+1));
}

void ExoplanetsDialog::setUpdateValues(int hours)
{
	GETSTELMODULE(Exoplanets)->setUpdateFrequencyHours(hours);
	refreshUpdateValues();
}

void ExoplanetsDialog::setDistributionEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Exoplanets)->setDisplayMode(b);
}

void ExoplanetsDialog::setTimelineEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Exoplanets)->setTimelineMode(b);
}

void ExoplanetsDialog::setUpdatesEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Exoplanets)->setUpdatesEnabled(b);
	ui->updateFrequencySpinBox->setEnabled(b);
	if(b)
		ui->updateButton->setText(q_("Update now"));
	else
		ui->updateButton->setText(q_("Update from files"));

	refreshUpdateValues();
}

void ExoplanetsDialog::updateStateReceiver(Exoplanets::UpdateState state)
{
	//qDebug() << "ExoplanetsDialog::updateStateReceiver got a signal";
	if (state==Exoplanets::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (state==Exoplanets::DownloadError || state==Exoplanets::OtherError)
	{
		ui->nextUpdateLabel->setText(q_("Update error"));
		updateTimer->start();  // make sure message is displayed for a while...
	}
}

void ExoplanetsDialog::updateCompleteReceiver(void)
{
        ui->nextUpdateLabel->setText(QString(q_("Exoplanets is updated")));
	// display the status for another full interval before refreshing status
	updateTimer->start();
	ui->lastUpdateDateTimeEdit->setDateTime(GETSTELMODULE(Exoplanets)->getLastUpdate());
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
}

void ExoplanetsDialog::restoreDefaults(void)
{
	qDebug() << "Exoplanets::restoreDefaults";
	GETSTELMODULE(Exoplanets)->restoreDefaults();
	GETSTELMODULE(Exoplanets)->readSettingsFromConfig();
	updateGuiFromSettings();
}

void ExoplanetsDialog::updateGuiFromSettings(void)
{
	ui->internetUpdatesCheckbox->setChecked(GETSTELMODULE(Exoplanets)->getUpdatesEnabled());
	refreshUpdateValues();
}

void ExoplanetsDialog::saveSettings(void)
{
	GETSTELMODULE(Exoplanets)->saveSettingsToConfig();
}

void ExoplanetsDialog::updateJSON(void)
{
	if(GETSTELMODULE(Exoplanets)->getUpdatesEnabled())
	{
		GETSTELMODULE(Exoplanets)->updateJSON();
	}
}
