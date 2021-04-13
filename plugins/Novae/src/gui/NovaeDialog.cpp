/*
 * Stellarium Novae Plug-in GUI
 *
 * Copyright (C) 2013 Alexander Wolf
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
#include "StelCore.hpp"
#include "ui_novaeDialog.h"
#include "NovaeDialog.hpp"
#include "Novae.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelStyle.hpp"
#include "StelGui.hpp"
#include "StelFileMgr.hpp"
#include "StelTranslator.hpp"

NovaeDialog::NovaeDialog()
	: StelDialog("Novae")
	, nova(Q_NULLPTR)
	, updateTimer(Q_NULLPTR)
{
	ui = new Ui_novaeDialog;
}

NovaeDialog::~NovaeDialog()
{
	if (updateTimer)
	{
		updateTimer->stop();
		delete updateTimer;
		updateTimer = Q_NULLPTR;
	}
	delete ui;
}

void NovaeDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		refreshUpdateValues();
		setAboutHtml();
	}
}

// Initialize the dialog widgets and connect the signals/slots
void NovaeDialog::createDialogContent()
{
	nova = GETSTELMODULE(Novae);
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
	connect(ui->internetUpdatesCheckbox, SIGNAL(stateChanged(int)), this, SLOT(setUpdatesEnabled(int)));
	connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(updateJSON()));
	connect(nova, SIGNAL(updateStateChanged(Novae::UpdateState)), this, SLOT(updateStateReceiver(Novae::UpdateState)));
	connect(nova, SIGNAL(jsonUpdateComplete(void)), this, SLOT(updateCompleteReceiver(void)));	
	connect(ui->updateFrequencySpinBox, SIGNAL(valueChanged(int)), this, SLOT(setUpdateValues(int)));
	refreshUpdateValues(); // fetch values for last updated and so on
	// if the state didn't change, setUpdatesEnabled will not be called, so we force it
	setUpdatesEnabled(ui->internetUpdatesCheckbox->checkState());

	updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
	updateTimer->start(7000);

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
	connect(ui->saveSettingsButton, SIGNAL(clicked()), this, SLOT(saveSettings()));

	// About tab
	setAboutHtml();

	updateGuiFromSettings();
}

void NovaeDialog::setAboutHtml(void)
{
	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Bright Novae Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + NOVAE_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + NOVAE_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td></tr>";
	html += "</table>";

	html += "<p>" + q_("A plugin that shows some bright novae in the Milky Way galaxy.");
	html += " " + q_("You can find novae via search tool by entering designation of nova or its common name (e.g. 'Nova Cygni 1975' or 'V1500 Cyg').") + "</p>";

	html += "<p>" + q_("This plugin allows you to see recent bright novae: ");
	html += nova->getNovaeList();
	html += ". " + q_("This list altogether contains %1 stars.").arg(nova->getCountNovae());
	html += " " + q_("All those novae are brighter than %1 at peak of brightness.").arg(QString::number(nova->getLowerLimitBrightness(), 'f', 2) + "<sup>m</sup>") + "</p>";
	html += "<h3>" + q_("Light curves") + "</h3>";
	html += q_("This plugin uses a very simple model for calculation of light curves for novae stars.") + " ";
	html += q_("This model is based on time for decay by %1 magnitudes from the maximum value, where %1 is 2, 3, 6 and 9.").arg("<em>N</em>") + " ";
	html += q_("If a nova has no values for decay of magnitude then this plugin will use generalized values for it.");
	html += "<p>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Github website.  Be sure to put \"%1\" in the subject when posting.")).arg("Bright Novae plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you have a question, you can {get an answer here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://groups.google.com/forum/#!forum/stellarium\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("Bug reports and feature requests can be made {here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/issues\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you want to read full information about this plugin and its history, you can {get info here}.").toHtmlEscaped().replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/Bright_Novae_plugin\">\\1</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}

	ui->aboutTextBrowser->setHtml(html);
}

void NovaeDialog::refreshUpdateValues(void)
{
	QString nextUpdate = q_("Next update");
	ui->lastUpdateDateTimeEdit->setDateTime(nova->getLastUpdate());
	ui->updateFrequencySpinBox->setValue(nova->getUpdateFrequencyDays());
	int secondsToUpdate = nova->getSecondsToUpdate();
	ui->internetUpdatesCheckbox->setChecked(nova->getUpdatesEnabled());
	if (!nova->getUpdatesEnabled())
		ui->nextUpdateLabel->setText(q_("Internet updates disabled"));
	else if (nova->getUpdateState() == Novae::Updating)
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

void NovaeDialog::setUpdateValues(int days)
{
	nova->setUpdateFrequencyDays(days);
	refreshUpdateValues();
}

void NovaeDialog::setUpdatesEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	nova->setUpdatesEnabled(b);
	ui->updateFrequencySpinBox->setEnabled(b);
	if(b)
		ui->updateButton->setText(q_("Update now"));
	else
		ui->updateButton->setText(q_("Update from files"));

	refreshUpdateValues();
}

void NovaeDialog::updateStateReceiver(Novae::UpdateState state)
{
	//qDebug() << "NovaeDialog::updateStateReceiver got a signal";
	if (state==Novae::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (state==Novae::DownloadError || state==Novae::OtherError)
	{
		ui->nextUpdateLabel->setText(q_("Update error"));
		updateTimer->start();  // make sure message is displayed for a while...
	}
}

void NovaeDialog::updateCompleteReceiver(void)
{
	ui->nextUpdateLabel->setText(QString(q_("Novae is updated")));
	// display the status for another full interval before refreshing status
	updateTimer->start();
	ui->lastUpdateDateTimeEdit->setDateTime(nova->getLastUpdate());
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
	setAboutHtml();
}

void NovaeDialog::restoreDefaults(void)
{
	if (askConfirmation())
	{
		qDebug() << "[Novae] restore defaults...";
		nova->restoreDefaults();
		nova->readSettingsFromConfig();
		updateGuiFromSettings();
	}
	else
		qDebug() << "[Novae] restore defaults is canceled...";
}

void NovaeDialog::updateGuiFromSettings(void)
{
	ui->internetUpdatesCheckbox->setChecked(nova->getUpdatesEnabled());
	refreshUpdateValues();
}

void NovaeDialog::saveSettings(void)
{
	nova->saveSettingsToConfig();
}

void NovaeDialog::updateJSON(void)
{
	if(nova->getUpdatesEnabled())
	{
		nova->updateJSON();
	}
}
