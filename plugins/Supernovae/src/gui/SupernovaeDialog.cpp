/*
 * Stellarium Historical Supernovae Plug-in GUI
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
#include "ui_supernovaeDialog.h"
#include "SupernovaeDialog.hpp"
#include "Supernovae.hpp"
#include "StelModuleMgr.hpp"
#include "StelStyle.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"

SupernovaeDialog::SupernovaeDialog()
	: StelDialog("Supernovae")
	, sn(Q_NULLPTR)
	, updateTimer(Q_NULLPTR)
{
	ui = new Ui_supernovaeDialog;
}

SupernovaeDialog::~SupernovaeDialog()
{
	if (updateTimer)
	{
		updateTimer->stop();
		delete updateTimer;
		updateTimer = Q_NULLPTR;
	}
	delete ui;
}

void SupernovaeDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		refreshUpdateValues();
		setAboutHtml();
	}
}

// Initialize the dialog widgets and connect the signals/slots
void SupernovaeDialog::createDialogContent()
{
	sn = GETSTELMODULE(Supernovae);
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);	
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &SupernovaeDialog::retranslate);

	// Kinetic scrolling
	kineticScrollingList << ui->aboutTextBrowser;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, &StelGui::flagUseKineticScrollingChanged, this, &SupernovaeDialog::enableKineticScrolling);
	}

	// Settings tab / updates group
	connect(ui->internetUpdatesCheckbox, &QCheckBox::stateChanged, this, &SupernovaeDialog::setUpdatesEnabled);
	connect(ui->updateButton, &QPushButton::clicked, this, &SupernovaeDialog::updateJSON);
	connect(sn, &Supernovae::updateStateChanged, this, &SupernovaeDialog::updateStateReceiver);
	connect(sn, &Supernovae::jsonUpdateComplete, this, &SupernovaeDialog::updateCompleteReceiver);
	connect(ui->updateFrequencySpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &SupernovaeDialog::setUpdateValues);
	refreshUpdateValues(); // fetch values for last updated and so on
	// if the state didn't change, setUpdatesEnabled will not be called, so we force it
	setUpdatesEnabled(ui->internetUpdatesCheckbox->checkState());

	updateTimer = new QTimer(this);
	connect(updateTimer, &QTimer::timeout, this, &SupernovaeDialog::refreshUpdateValues);
	updateTimer->start(7000);

	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, &TitleBar::movedTo, this, &StelDialog::handleMovedTo);

	connect(ui->restoreDefaultsButton, &QPushButton::clicked, this, &SupernovaeDialog::restoreDefaults);
	connect(ui->saveSettingsButton, &QPushButton::clicked, this, &SupernovaeDialog::saveSettings);

	// About tab
	setAboutHtml();
	if(gui!=Q_NULLPTR)
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));

	updateGuiFromSettings();
}

void SupernovaeDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Historical Supernovae Plug-in") + "</h2><table class='layout' width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + SUPERNOVAE_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + SUPERNOVAE_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Alexander Wolf</td></tr>";
	html += "</table>";

	html += "<p>" + q_("This plugin allows you to see some bright historical supernovae: ");
	html += sn->getSupernovaeList();
	html += ". " + q_("This list altogether contains %1 stars.").arg(sn->getCountSupernovae());
	html += " " + q_("All those supernovae are brighter %1 at peak of brightness.").arg(QString::number(sn->getLowerLimitBrightness(), 'f', 2) + "<sup>m</sup>") + "</p>";

	html += "<h3>" + q_("Light curves") + "</h3>";
	html += "<p>" + QString(q_("This plugin implements a simple model of light curves for type I and type II supernovae. Figures and a description of the model can be found in the Stellarium User Guide.")) + "</p>";

	html += "<h3>" + q_("Acknowledgments") + "</h3>";
	html += "<p>" + q_("We thank the following people for their contribution and valuable comments:") + "</p><ul>";
	html += "<li>" + QString("%1 (<a href='%2'>%3</a> %4)").arg(
				 q_("Sergei Blinnikov"),
				 "http://www.itep.ru/",
				 q_("Institute for Theoretical and Experimental Physics"),
				 q_("in Russia")) + "</li>";
	html += "</ul>";

	html += StelApp::getInstance().getModuleMgr().getStandardSupportLinksInfo("Historical Supernovae plugin");
	html += "</body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}

	ui->aboutTextBrowser->setHtml(html);
}

void SupernovaeDialog::refreshUpdateValues(void)
{
	QString nextUpdate = q_("Next update");
	ui->lastUpdateDateTimeEdit->setDateTime(sn->getLastUpdate());
	ui->updateFrequencySpinBox->setValue(sn->getUpdateFrequencyDays());
	int secondsToUpdate = sn->getSecondsToUpdate();
	ui->internetUpdatesCheckbox->setChecked(sn->getUpdatesEnabled());
	if (!sn->getUpdatesEnabled())
		ui->nextUpdateLabel->setText(q_("Internet updates disabled"));
	else if (sn->getUpdateState() == Supernovae::Updating)
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

void SupernovaeDialog::setUpdateValues(int days)
{
	sn->setUpdateFrequencyDays(days);
	refreshUpdateValues();
}

void SupernovaeDialog::setUpdatesEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	sn->setUpdatesEnabled(b);
	ui->updateFrequencySpinBox->setEnabled(b);
	if(b)
		ui->updateButton->setText(q_("Update now"));
	else
		ui->updateButton->setText(q_("Update from files"));

	refreshUpdateValues();
}

void SupernovaeDialog::updateStateReceiver(Supernovae::UpdateState state)
{
	//qDebug() << "SupernovaeDialog::updateStateReceiver got a signal";
	if (state==Supernovae::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (state==Supernovae::DownloadError || state==Supernovae::OtherError)
	{
		ui->nextUpdateLabel->setText(q_("Update error"));
		updateTimer->start();  // make sure message is displayed for a while...
	}
}

void SupernovaeDialog::updateCompleteReceiver(void)
{
	ui->nextUpdateLabel->setText(QString(q_("Historical supernovae is updated")));
	// display the status for another full interval before refreshing status
	updateTimer->start();
	ui->lastUpdateDateTimeEdit->setDateTime(sn->getLastUpdate());
	QTimer *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &SupernovaeDialog::refreshUpdateValues);
	setAboutHtml();
}

void SupernovaeDialog::restoreDefaults(void)
{
	if (askConfirmation())
	{
		qDebug() << "[Supernovae] restore defaults...";
		sn->restoreDefaults();
		sn->readSettingsFromConfig();
		updateGuiFromSettings();
	}
	else
		qDebug() << "[Supernovae] restore defaults is canceled...";
}

void SupernovaeDialog::updateGuiFromSettings(void)
{
	ui->internetUpdatesCheckbox->setChecked(sn->getUpdatesEnabled());
	refreshUpdateValues();
}

void SupernovaeDialog::saveSettings(void)
{
	sn->saveSettingsToConfig();
}

void SupernovaeDialog::updateJSON(void)
{
	if(sn->getUpdatesEnabled())
	{
		sn->updateJSON();
	}
}
