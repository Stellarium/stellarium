/*
 * Stellarium Quasars Plug-in GUI
 *
 * Copyright (C) 2012, 2018 Alexander Wolf
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
#include <QColorDialog>

#include "StelApp.hpp"
#include "ui_quasarsDialog.h"
#include "QuasarsDialog.hpp"
#include "Quasars.hpp"
#include "StelModuleMgr.hpp"
#include "StelStyle.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"

QuasarsDialog::QuasarsDialog()
	: StelDialog("Quasars")
	, qsr(Q_NULLPTR)
	, updateTimer(Q_NULLPTR)
{
	ui = new Ui_quasarsDialog;
}

QuasarsDialog::~QuasarsDialog()
{
	if (updateTimer)
	{
		updateTimer->stop();
		delete updateTimer;
		updateTimer = Q_NULLPTR;
	}
	delete ui;
}

void QuasarsDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		refreshUpdateValues();
		setAboutHtml();
	}
}

// Initialize the dialog widgets and connect the signals/slots
void QuasarsDialog::createDialogContent()
{
	qsr = GETSTELMODULE(Quasars);
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);	
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &QuasarsDialog::retranslate);

	// Settings tab / updates group
	ui->displayModeCheckBox->setChecked(qsr->getDisplayMode());
	connect(ui->displayModeCheckBox, &QCheckBox::stateChanged, this, &QuasarsDialog::setDistributionEnabled);
	ui->displayAtStartupCheckBox->setChecked(qsr->getEnableAtStartup());
	connect(ui->displayAtStartupCheckBox, &QCheckBox::stateChanged, this, &QuasarsDialog::setDisplayAtStartupEnabled);
	ui->displayShowQuasarsButton->setChecked(qsr->getFlagShowQuasarsButton());
	connect(ui->displayShowQuasarsButton, &QCheckBox::stateChanged, this, &QuasarsDialog::setDisplayShowQuasarsButton);
	ui->displayUseQuasarMarkersButton->setChecked(qsr->getFlagUseQuasarMarkers());
	connect(ui->displayUseQuasarMarkersButton, &QCheckBox::stateChanged, this, &QuasarsDialog::setDisplayUseQuasarMarkersButton);
	connect(ui->internetUpdatesCheckbox, &QCheckBox::stateChanged, this, &QuasarsDialog::setUpdatesEnabled);
	connect(ui->updateButton, &QPushButton::clicked, this, &QuasarsDialog::updateJSON);
	connect(qsr, &Quasars::updateStateChanged, this, &QuasarsDialog::updateStateReceiver);
	connect(qsr, &Quasars::jsonUpdateComplete, this, &QuasarsDialog::updateCompleteReceiver);
	connect(ui->updateFrequencySpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QuasarsDialog::setUpdateValues);
	refreshUpdateValues(); // fetch values for last updated and so on
	// if the state didn't change, setUpdatesEnabled will not be called, so we force it
	setUpdatesEnabled(ui->internetUpdatesCheckbox->checkState());

	ui->quasarMarkerColor->setup("Quasars.quasarsColor", "Quasars/marker_color");

	updateTimer = new QTimer(this);
	connect(updateTimer, &QTimer::timeout, this, &QuasarsDialog::refreshUpdateValues);
	updateTimer->start(7000);

	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, &TitleBar::movedTo, this, &StelDialog::handleMovedTo);

	connect(ui->restoreDefaultsButton, &QPushButton::clicked, this, &QuasarsDialog::restoreDefaults);
	connect(ui->saveSettingsButton, &QPushButton::clicked, this, &QuasarsDialog::saveSettings);

	// About tab
	setAboutHtml();
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));

	updateGuiFromSettings();
}

void QuasarsDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Quasars Plug-in") + "</h2><table class='layout' width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + QUASARS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + QUASARS_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Alexander Wolf</td></tr>";
	html += "</table>";

	html += QString("<p>%1 (<a href=\"%2\">%3</a>)</p>").arg(
			q_("The Quasars plugin provides visualization of some quasars brighter than visual magnitude 18. The catalogue of quasars was compiled from \"Quasars and Active Galactic Nuclei\" (13th Ed.)"),
			"http://adsabs.harvard.edu/abs/2010A%26A...518A..10V",
			q_("Veron+ 2010"));

	html += "</ul><p>" + q_("The current catalog contains info about %1 quasars.").arg(qsr->getCountQuasars()) + "</p>";
	html += StelApp::getInstance().getModuleMgr().getStandardSupportLinksInfo("Quasars plugin");
	html += "</body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}

	ui->aboutTextBrowser->setHtml(html);
}

void QuasarsDialog::refreshUpdateValues(void)
{
	QString nextUpdate = q_("Next update");
	ui->lastUpdateDateTimeEdit->setDateTime(qsr->getLastUpdate());
	ui->updateFrequencySpinBox->setValue(qsr->getUpdateFrequencyDays());
	int secondsToUpdate = qsr->getSecondsToUpdate();
	ui->internetUpdatesCheckbox->setChecked(qsr->getUpdatesEnabled());
	if (!qsr->getUpdatesEnabled())
		ui->nextUpdateLabel->setText(q_("Internet updates disabled"));
	else if (qsr->getUpdateState() == Quasars::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (secondsToUpdate <= 60)
		ui->nextUpdateLabel->setText(QString("%1: %2").arg(nextUpdate, q_("< 1 minute")));
	else if (secondsToUpdate < 3600)
	{
		int n = (secondsToUpdate/60)+1;
		// TRANSLATORS: minutes.
		ui->nextUpdateLabel->setText(QString("%1: %2 %3").arg(nextUpdate, QString::number(n), qc_("m", "time")));
	}
	else if (secondsToUpdate < 86400)
	{
		int n = (secondsToUpdate/3600)+1;
		// TRANSLATORS: hours.
		ui->nextUpdateLabel->setText(QString("%1: %2 %3").arg(nextUpdate, QString::number(n), qc_("h", "time")));
	}
	else
	{
		int n = (secondsToUpdate/86400)+1;
		// TRANSLATORS: days.
		ui->nextUpdateLabel->setText(QString("%1: %2 %3").arg(nextUpdate, QString::number(n), qc_("d", "time")));
	}
}

void QuasarsDialog::setUpdateValues(int days)
{
	qsr->setUpdateFrequencyDays(days);
	refreshUpdateValues();
}

void QuasarsDialog::setUpdatesEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	qsr->setUpdatesEnabled(b);
	ui->updateFrequencySpinBox->setEnabled(b);
	if(b)
		ui->updateButton->setText(q_("Update now"));
	else
		ui->updateButton->setText(q_("Update from files"));

	refreshUpdateValues();
}

void QuasarsDialog::setDistributionEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	qsr->setDisplayMode(b);
}

void QuasarsDialog::setDisplayAtStartupEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	qsr->setEnableAtStartup(b);
}

void QuasarsDialog::setDisplayShowQuasarsButton(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	qsr->setFlagShowQuasarsButton(b);
}

void QuasarsDialog::setDisplayUseQuasarMarkersButton(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	qsr->setFlagUseQuasarMarkers(b);
}

void QuasarsDialog::updateStateReceiver(Quasars::UpdateState state)
{
	//qDebug() << "QuasarsDialog::updateStateReceiver got a signal";
	if (state==Quasars::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (state==Quasars::DownloadError || state==Quasars::OtherError)
	{
		ui->nextUpdateLabel->setText(q_("Update error"));
		updateTimer->start();  // make sure message is displayed for a while...
	}
}

void QuasarsDialog::updateCompleteReceiver(void)
{
	ui->nextUpdateLabel->setText(QString(q_("Quasars is updated")));
	// display the status for another full interval before refreshing status
	updateTimer->start();
	ui->lastUpdateDateTimeEdit->setDateTime(qsr->getLastUpdate());
	QTimer *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &QuasarsDialog::refreshUpdateValues);
	setAboutHtml();
}

void QuasarsDialog::restoreDefaults(void)
{
	if (askConfirmation())
	{
		qDebug() << "[Quasars] restore defaults...";
		qsr->restoreDefaults();
		qsr->readSettingsFromConfig();
		updateGuiFromSettings();
	}
	else
		qDebug() << "[Quasars] restore defaults is canceled...";
}

void QuasarsDialog::updateGuiFromSettings(void)
{
	ui->internetUpdatesCheckbox->setChecked(qsr->getUpdatesEnabled());
	refreshUpdateValues();
}

void QuasarsDialog::saveSettings(void)
{
	qsr->saveSettingsToConfig();
}

void QuasarsDialog::updateJSON(void)
{
	if(qsr->getUpdatesEnabled())
	{
		qsr->updateJSON();
	}
}
