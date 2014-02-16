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

#include <QColorDialog>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QGraphicsColorizeEffect>
#include <QMessageBox>
#include <QTimer>
#include <QUrl>

#include "MeteorShowerDialog.hpp"
#include "MeteorShowers.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelMainView.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelStyle.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "ui_meteorShowerDialog.h"

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
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	plugin = GETSTELMODULE(MeteorShowers);

	// Settings tab / updates group	
	connect(ui->internetUpdates, SIGNAL(clicked(bool)), this, SLOT(setUpdatesEnabled(bool)));
	connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(updateJSON()));
	connect(plugin, SIGNAL(updateStateChanged(MeteorShowers::UpdateState)), this, SLOT(updateStateReceiver(MeteorShowers::UpdateState)));
	connect(plugin, SIGNAL(jsonUpdateComplete(void)), this, SLOT(updateCompleteReceiver(void)));
	connect(ui->updateFrequencySpinBox, SIGNAL(valueChanged(int)), this, SLOT(setUpdateValues(int)));
	refreshUpdateValues(); // fetch values for last updated and so on

	updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
	updateTimer->start(7000);

	// Settings tab / event group
	int year, month, day;
	// Get current JD and convert it to year, month and day
	// TODO: Add changes for the range of dates when year in main app will be changed?
	StelUtils::getDateFromJulianDay(StelApp::getInstance().getCore()->getJDay(), &year, &month, &day);
	ui->dateFrom->setDate(QDate(year, 1, 1)); // first date in the range - first day of the year
	ui->dateTo->setDate(QDate(year, 12, 31)); // second date in the range - last day of the year
	connect(ui->searchButton, SIGNAL(clicked()), this, SLOT(checkDates()));
	connect(ui->listEvents, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectEvent(QModelIndex)));

	// Settings tab / radiant group
	ui->displayRadiant->setChecked(plugin->getFlagRadiant());
	connect(ui->displayRadiant, SIGNAL(clicked(bool)), plugin, SLOT(setFlagRadiant(bool)));
	ui->activeRadiantsOnly->setChecked(plugin->getFlagActiveRadiant());
	connect(ui->activeRadiantsOnly, SIGNAL(clicked(bool)), plugin, SLOT(setFlagActiveRadiant(bool)));
	ui->radiantLabels->setChecked(plugin->getFlagLabels());
	connect(ui->radiantLabels, SIGNAL(clicked(bool)), plugin, SLOT(setFlagLabels(bool)));
	ui->fontSizeSpinBox->setValue(plugin->getLabelFontSize());
	connect(ui->fontSizeSpinBox, SIGNAL(valueChanged(int)), plugin, SLOT(setLabelFontSize(int)));

	// Settings tab / meteor showers group
	ui->displayMeteorShower->setChecked(plugin->getEnableAtStartup());
	connect(ui->displayMeteorShower, SIGNAL(clicked(bool)), plugin, SLOT(setEnableAtStartup(bool)));
	ui->displayShowMeteorShowerButton->setChecked(plugin->getFlagShowMSButton());
	connect(ui->displayShowMeteorShowerButton, SIGNAL(clicked(bool)), plugin, SLOT(setFlagShowMSButton(bool)));

	// /////////////////////////////////////////

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
	connect(ui->saveSettingsButton, SIGNAL(clicked()), this, SLOT(saveSettings()));

	// Markers tab
	refreshColorMarkers();
	connect(ui->changeColorARG, SIGNAL(clicked()), this, SLOT(setColorARG()));
	connect(ui->changeColorARR, SIGNAL(clicked()), this, SLOT(setColorARR()));
	connect(ui->changeColorIR, SIGNAL(clicked()), this, SLOT(setColorIR()));

	// About tab
	setAboutHtml();
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));

	updateGuiFromSettings();
}

void MeteorShowerDialog::checkDates(void)
{
	double jdFrom = StelUtils::qDateTimeToJd((QDateTime) ui->dateFrom->date());
	double jdTo = StelUtils::qDateTimeToJd((QDateTime) ui->dateTo->date());

	if(jdFrom > jdTo)
		QMessageBox::warning(0, "Stellarium", q_("Start date greater than end date!"));
	else if (jdTo-jdFrom > 365)
		QMessageBox::warning(0, "Stellarium", q_("Time interval must be less than one year!"));
	else
		searchEvents();
}

void MeteorShowerDialog::searchEvents(void)
{
	ui->listEvents->clear();
	searchResult = plugin->searchEvents(ui->dateFrom->date(), ui->dateTo->date());
	foreach(const MeteorShowerP& r, searchResult)
		ui->listEvents->addItem(r->getNameI18n());
}

void MeteorShowerDialog::selectEvent(const QModelIndex &modelIndex)
{
	StelCore *core = StelApp::getInstance().getCore();
	core->setJDay(StelUtils::qDateTimeToJd(searchResult.value(modelIndex.row())->getPeak()));
}

void MeteorShowerDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Meteor Showers Plugin") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td>" + q_("Version:") + "</td><td>" + METEORSHOWERS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td>" + q_("Author:") + "</td><td>Marcos Cardinot &lt;mcardinot@gmail.com&gt;</td></tr></table>";

	html += "<p>" + q_("The Meteor Showers plugin give visualization of the meteor showers, show information about meteor showers and displays marker for radiants in activity range for each meteor showers.") + "</p>";
	html += "<p>" + q_("This plugin was created as project of ESA Summer of Code in Space 2013.") + "</p>";
	html += "</body></html>";

	ui->aboutTextBrowser->setHtml(html);
}

void MeteorShowerDialog::refreshUpdateValues(void)
{
	ui->lastUpdateDateTimeEdit->setDateTime(plugin->getLastUpdate());
	ui->updateFrequencySpinBox->setValue(plugin->getUpdateFrequencyHours());
	int secondsToUpdate = plugin->getSecondsToUpdate();
	if (!plugin->getUpdatesEnabled())
		ui->nextUpdateLabel->setText(q_("Internet updates disabled"));
	else if (plugin->getUpdateState() == MeteorShowers::Updating)
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
	plugin->setUpdateFrequencyHours(hours);
	refreshUpdateValues();
}

void MeteorShowerDialog::setUpdatesEnabled(bool checkState)
{
	plugin->setUpdatesEnabled(checkState);
	ui->updateFrequencySpinBox->setEnabled(checkState);
	ui->updateButton->setText(q_("Update now"));
	refreshUpdateValues();
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
	plugin->restoreDefaults();
	plugin->readSettingsFromConfig();
	updateGuiFromSettings();
}

void MeteorShowerDialog::updateGuiFromSettings(void)
{	
	refreshUpdateValues();
	refreshColorMarkers();
}

void MeteorShowerDialog::saveSettings(void)
{
	plugin->saveSettingsToConfig();
}

void MeteorShowerDialog::updateJSON(void)
{
	if(plugin->getUpdatesEnabled())
	{
		plugin->updateJSON();
	}
}

void MeteorShowerDialog::refreshColorMarkers(void)
{
	setTextureColor(ui->textureARG, plugin->getColorARG());
	setTextureColor(ui->textureARR, plugin->getColorARR());
	setTextureColor(ui->textureIR, plugin->getColorIR());
}

void MeteorShowerDialog::setTextureColor(QLabel *texture, QColor color)
{
	QGraphicsColorizeEffect *e = new QGraphicsColorizeEffect(texture);
	e->setColor(color);
	texture->setGraphicsEffect(e);
}

void MeteorShowerDialog::setColorARG()
{
	QColor color = QColorDialog::getColor();
	setTextureColor(ui->textureARG, color);
	plugin->setColorARG(color);
}

void MeteorShowerDialog::setColorARR()
{
	QColor color = QColorDialog::getColor();
	setTextureColor(ui->textureARR, color);
	plugin->setColorARR(color);
}

void MeteorShowerDialog::setColorIR()
{
	QColor color = QColorDialog::getColor();
	setTextureColor(ui->textureIR, color);
	plugin->setColorIR(color);
}
