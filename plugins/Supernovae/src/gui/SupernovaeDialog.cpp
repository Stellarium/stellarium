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
#include "StelObjectMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelStyle.hpp"
#include "StelGui.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelFileMgr.hpp"
#include "StelTranslator.hpp"

SupernovaeDialog::SupernovaeDialog() : updateTimer(NULL)
{
	ui = new Ui_supernovaeDialog;
}

SupernovaeDialog::~SupernovaeDialog()
{
	if (updateTimer)
	{
		updateTimer->stop();
		delete updateTimer;
		updateTimer = NULL;
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
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);	
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
		this, SLOT(retranslate()));

	// Settings tab / updates group
	connect(ui->internetUpdatesCheckbox, SIGNAL(stateChanged(int)), this, SLOT(setUpdatesEnabled(int)));
	connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(updateJSON()));
	connect(GETSTELMODULE(Supernovae), SIGNAL(updateStateChanged(Supernovae::UpdateState)), this, SLOT(updateStateReceiver(Supernovae::UpdateState)));
	connect(GETSTELMODULE(Supernovae), SIGNAL(jsonUpdateComplete(void)), this, SLOT(updateCompleteReceiver(void)));
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

void SupernovaeDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Historical Supernovae Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + SUPERNOVAE_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td></tr>";
	html += "</table>";

	html += "<p>" + q_("A plugin that shows some historical supernovae brighter than visual magnitude 10: ");
	html += GETSTELMODULE(Supernovae)->getSupernovaeList();
	html += ".</p>";

	html += "<h3>" + q_("Light curves") + "</h3>";
	html += "<p>" + QString(q_("This plugin implements a simple model of light curves for different supernovae. Typical views of light curves for type I and type II supernova can be seen %1here%2 (right scale in days), and this model is used for this plugin.")).arg("<a href=\"http://stellarium.org/wiki/index.php/Historical_Supernovae_plugin#Light_curves\">").arg("</a>") + "</p>";

	html += "<h3>" + q_("Acknowledgments") + "</h3>";
	html += "<p>" + q_("We thank the following people for their contribution and valuable comments:") + "</p><ul>";
	html += "<li>" + QString("%1 (<a href='%2'>%3</a> %4)")
			.arg(q_("Sergei Blinnikov"))
			.arg("http://www.itep.ru/")
			.arg(q_("Institute for Theoretical and Experimental Physics"))
			.arg(q_("in Russia")) + "</li>";
	html += "</ul><h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Launchpad website.  Be sure to put \"%1\" in the subject when posting.")).arg("Historical Supernovae plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("If you have a question, you can %1get an answer here%2").arg("<a href=\"https://answers.launchpad.net/stellarium\">")).arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("Bug reports can be made %1here%2.")).arg("<a href=\"https://bugs.launchpad.net/stellarium\">").arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you would like to make a feature request, you can create a bug report, and set the severity to \"wishlist\".") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you want to read full information about this plugin, its history and format of catalog, you can %1get info here%2.").arg("<a href=\"http://stellarium.org/wiki/index.php/Historical_Supernovae_plugin\">").arg("</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
	ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);

	ui->aboutTextBrowser->setHtml(html);
}

void SupernovaeDialog::refreshUpdateValues(void)
{
	ui->lastUpdateDateTimeEdit->setDateTime(GETSTELMODULE(Supernovae)->getLastUpdate());
	ui->updateFrequencySpinBox->setValue(GETSTELMODULE(Supernovae)->getUpdateFrequencyDays());
	int secondsToUpdate = GETSTELMODULE(Supernovae)->getSecondsToUpdate();
	ui->internetUpdatesCheckbox->setChecked(GETSTELMODULE(Supernovae)->getUpdatesEnabled());
	if (!GETSTELMODULE(Supernovae)->getUpdatesEnabled())
		ui->nextUpdateLabel->setText(q_("Internet updates disabled"));
	else if (GETSTELMODULE(Supernovae)->getUpdateState() == Supernovae::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (secondsToUpdate <= 60)
		ui->nextUpdateLabel->setText(q_("Next update: < 1 minute"));
	else if (secondsToUpdate < 3600)
		ui->nextUpdateLabel->setText(QString(q_("Next update: %1 minutes")).arg((secondsToUpdate/60)+1));
	else if (secondsToUpdate < 86400)
		ui->nextUpdateLabel->setText(QString(q_("Next update: %1 hours")).arg((secondsToUpdate/3600)+1));
	else
		ui->nextUpdateLabel->setText(QString(q_("Next update: %1 days")).arg((secondsToUpdate/86400)+1));
}

void SupernovaeDialog::setUpdateValues(int days)
{
	GETSTELMODULE(Supernovae)->setUpdateFrequencyDays(days);
	refreshUpdateValues();
}

void SupernovaeDialog::setUpdatesEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Supernovae)->setUpdatesEnabled(b);
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
	ui->lastUpdateDateTimeEdit->setDateTime(GETSTELMODULE(Supernovae)->getLastUpdate());
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
}

void SupernovaeDialog::restoreDefaults(void)
{
	qDebug() << "Supernovae::restoreDefaults";
	GETSTELMODULE(Supernovae)->restoreDefaults();
	GETSTELMODULE(Supernovae)->readSettingsFromConfig();
	updateGuiFromSettings();
}

void SupernovaeDialog::updateGuiFromSettings(void)
{
	ui->internetUpdatesCheckbox->setChecked(GETSTELMODULE(Supernovae)->getUpdatesEnabled());
	refreshUpdateValues();
}

void SupernovaeDialog::saveSettings(void)
{
	GETSTELMODULE(Supernovae)->saveSettingsToConfig();
}

void SupernovaeDialog::updateJSON(void)
{
	if(GETSTELMODULE(Supernovae)->getUpdatesEnabled())
	{
		GETSTELMODULE(Supernovae)->updateJSON();
	}
}
