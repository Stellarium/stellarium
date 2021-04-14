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
#include <QColorDialog>

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "ui_pulsarsDialog.h"
#include "PulsarsDialog.hpp"
#include "Pulsars.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelStyle.hpp"
#include "StelGui.hpp"
#include "StelMainView.hpp"
#include "StelFileMgr.hpp"
#include "StelTranslator.hpp"

PulsarsDialog::PulsarsDialog()
	: StelDialog("Pulsars")
	, psr(Q_NULLPTR)
	, updateTimer(Q_NULLPTR)
{
	ui = new Ui_pulsarsDialog;
}

PulsarsDialog::~PulsarsDialog()
{
	if (updateTimer)
	{
		updateTimer->stop();
		delete updateTimer;
		updateTimer = Q_NULLPTR;
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
	psr = GETSTELMODULE(Pulsars);
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);	
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
		this, SLOT(retranslate()));

	// Kinetic scrolling
	kineticScrollingList << ui->aboutTextBrowser;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

	// Settings tab / updates group
	ui->displayModeCheckBox->setChecked(psr->getDisplayMode());
	connect(ui->displayModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setDistributionEnabled(int)));
	ui->displayAtStartupCheckBox->setChecked(psr->getEnableAtStartup());
	connect(ui->displayAtStartupCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setDisplayAtStartupEnabled(int)));
	ui->displayShowPulsarsButton->setChecked(psr->getFlagShowPulsarsButton());
	ui->displaySeparateColorsCheckBox->setChecked(psr->getGlitchFlag());
	ui->displayFilteredPulsarsCheckBox->setChecked(psr->getFilteredMode());
	ui->mJyDoubleSpinBox->setValue(psr->getFilterValue());
	connect(ui->displayShowPulsarsButton, SIGNAL(stateChanged(int)), this, SLOT(setDisplayShowPulsarsButton(int)));
	connect(ui->displaySeparateColorsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setSeparateColorsFlag(int)));
	connect(ui->displayFilteredPulsarsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setFilteringEnabled(int)));
	connect(ui->mJyDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setFilterValue(double)));
	connect(ui->internetUpdatesCheckbox, SIGNAL(stateChanged(int)), this, SLOT(setUpdatesEnabled(int)));
	connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(updateJSON()));
	connect(psr, SIGNAL(updateStateChanged(Pulsars::UpdateState)), this, SLOT(updateStateReceiver(Pulsars::UpdateState)));
	connect(psr, SIGNAL(jsonUpdateComplete(void)), this, SLOT(updateCompleteReceiver(void)));	
	connect(ui->updateFrequencySpinBox, SIGNAL(valueChanged(int)), this, SLOT(setUpdateValues(int)));
	refreshUpdateValues(); // fetch values for last updated and so on
	// if the state didn't change, setUpdatesEnabled will not be called, so we force it
	setUpdatesEnabled(ui->internetUpdatesCheckbox->checkState());

	connectColorButton(ui->pulsarMarkerColor,         "Pulsars.markerColor", "Pulsars/marker_color");
	connectColorButton(ui->pulsarGlitchesMarkerColor, "Pulsars.glitchColor", "Pulsars/glitch_color");

	updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
	updateTimer->start(7000);

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
	connect(ui->saveSettingsButton, SIGNAL(clicked()), this, SLOT(saveSettings()));

	// About tab
	setAboutHtml();
	if(gui!=Q_NULLPTR)
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));

	updateGuiFromSettings();
}

void PulsarsDialog::setAboutHtml(void)
{
	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Pulsars Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + PULSARS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + PULSARS_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td></tr>";
	html += "</table>";

	html += "<p>" + q_("This plugin plots the position of various pulsars, with object information about each one.") + "</p>";
	html += "<p>" + QString(q_("Pulsar data is derived from 'The ATNF Pulsar Catalogue'  (Manchester, R. N., Hobbs, G. B., Teoh, A. & Hobbs, M., Astron. J., 129, 1993-2006 (2005) (%1astro-ph/0412641%2))."))
			.arg("<a href=\"http://arxiv.org/abs/astro-ph/0412641\">")
			.arg("</a>") + "</p>";
	html += "<p>" + q_("Current catalog contains info about %1 pulsars.").arg(psr->getCountPulsars()) + "</p>";
	html += "<p>" + QString("<strong>%1:</strong> %2")
			.arg(q_("Note"))
			.arg(q_("pulsar identifiers have the prefix 'PSR'")) + "</p>";
	html += "<h3>" + q_("Acknowledgment") + "</h3>";
	html += "<p>" + q_("We thank the following people for their contribution and valuable comments:") + "</p><ul>";
	html += "<li>" + QString("%1 (<a href='%2'>%3</a> %4)")
			.arg(q_("Vladimir Samodourov"))
			.arg("http://www.prao.ru/")
			.arg(q_("Pushchino Radio Astronomy Observatory"))
			.arg(q_("in Russia")) + "</li>";
	html += "<li>" + QString("%1 (<a href='%2'>%3</a> %4)")
			.arg(q_("Maciej Serylak"))
			.arg("http://www.obs-nancay.fr/")
			.arg(q_("Nancay Radioastronomical Observatory"))
			.arg(q_("in France")) + "</li>";
	html += "</ul><h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Github website.  Be sure to put \"%1\" in the subject when posting.")).arg("Pulsars plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you have a question, you can {get an answer here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://groups.google.com/forum/#!forum/stellarium\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("Bug reports and feature requests can be made {here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/issues\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you want to read full information about this plugin and its history, you can {get info here}.").toHtmlEscaped().replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/Pulsars_plugin\">\\1</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}

	ui->aboutTextBrowser->setHtml(html);
}

void PulsarsDialog::refreshUpdateValues(void)
{
	QString nextUpdate = q_("Next update");
	ui->lastUpdateDateTimeEdit->setDateTime(psr->getLastUpdate());
	ui->updateFrequencySpinBox->setValue(psr->getUpdateFrequencyDays());
	int secondsToUpdate = psr->getSecondsToUpdate();
	ui->internetUpdatesCheckbox->setChecked(psr->getUpdatesEnabled());
	if (!psr->getUpdatesEnabled())
		ui->nextUpdateLabel->setText(q_("Internet updates disabled"));
	else if (psr->getUpdateState() == Pulsars::Updating)
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

void PulsarsDialog::setUpdateValues(int days)
{
	psr->setUpdateFrequencyDays(days);
	refreshUpdateValues();
}

void PulsarsDialog::setUpdatesEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	psr->setUpdatesEnabled(b);
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
	psr->setDisplayMode(b);
}

void PulsarsDialog::setFilteringEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	psr->setFilteredMode(b);
}

void PulsarsDialog::setFilterValue(double v)
{
	psr->setFilterValue((float)v);
}

void PulsarsDialog::setDisplayAtStartupEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	psr->setEnableAtStartup(b);
}

void PulsarsDialog::setDisplayShowPulsarsButton(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	psr->setFlagShowPulsarsButton(b);
}

void PulsarsDialog::setSeparateColorsFlag(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	psr->setGlitchFlag(b);
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
	ui->lastUpdateDateTimeEdit->setDateTime(psr->getLastUpdate());
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
	setAboutHtml();
}

void PulsarsDialog::restoreDefaults(void)
{
	if (askConfirmation())
	{
		qDebug() << "[Pulsars] restore defaults...";
		psr->restoreDefaults();
		psr->readSettingsFromConfig();
		updateGuiFromSettings();
	}
	else
		qDebug() << "[Pulsars] restore defaults is canceled...";
}

void PulsarsDialog::updateGuiFromSettings(void)
{
	ui->internetUpdatesCheckbox->setChecked(psr->getUpdatesEnabled());
	refreshUpdateValues();
}

void PulsarsDialog::saveSettings(void)
{
	psr->saveSettingsToConfig();
}

void PulsarsDialog::updateJSON(void)
{
	if(psr->getUpdatesEnabled())
	{
		psr->updateJSON();
	}
}
