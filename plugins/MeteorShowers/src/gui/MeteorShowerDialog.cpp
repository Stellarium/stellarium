/*
 * Stellarium: Meteor Showers Plug-in
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

MeteorShowerDialog::MeteorShowerDialog()
	: plugin(NULL)
	, updateTimer(NULL)
	, treeWidget(NULL)
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
		setHeaderNames();

		//Retranslate name and datatype strings
		QTreeWidgetItemIterator it(treeWidget);
		while (*it)
		{
			//Name
			(*it)->setText(ColumnName, q_((*it)->text(ColumnName)));
			//Data type
			(*it)->setText(ColumnDataType, q_((*it)->text(ColumnDataType)));
			++it;
		}
	}
}

// Initialize the dialog widgets and connect the signals/slots
void MeteorShowerDialog::createDialogContent()
{
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	plugin = GETSTELMODULE(MeteorShowers);

#ifdef Q_OS_WIN
	//Kinetic scrolling for tablet pc and pc
	QList<QWidget *> addscroll;
	addscroll << ui->listEvents << ui->aboutTextBrowser;
	installKineticScrolling(addscroll);
#endif

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
	connect(ui->searchButton, SIGNAL(clicked()), this, SLOT(checkDates()));
	refreshRangeDates();

	treeWidget = ui->listEvents;
	initListEvents();
	connect(treeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectEvent(QModelIndex)));

	// bug #1350669 (https://bugs.launchpad.net/stellarium/+bug/1350669)
	connect(treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), treeWidget, SLOT(repaint()));

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
	refreshMarkersColor();
	connect(ui->changeColorARG, SIGNAL(clicked()), this, SLOT(setColorARG()));
	connect(ui->changeColorARR, SIGNAL(clicked()), this, SLOT(setColorARR()));
	connect(ui->changeColorIR, SIGNAL(clicked()), this, SLOT(setColorIR()));

	// About tab
	setAboutHtml();
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui != NULL)
	{
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	}

	updateGuiFromSettings();
}

void MeteorShowerDialog::initListEvents(void)
{
	treeWidget->clear();
	treeWidget->setColumnCount(ColumnCount);
	setHeaderNames();
	treeWidget->header()->setSectionsMovable(false);
	treeWidget->header()->setStretchLastSection(true);
}

void MeteorShowerDialog::setHeaderNames(void)
{
	QStringList headerStrings;
	headerStrings << q_("Name");
	headerStrings << q_("ZHR");
	headerStrings << q_("Data Type");
	headerStrings << q_("Peak");
	treeWidget->setHeaderLabels(headerStrings);
}

void MeteorShowerDialog::checkDates(void)
{
	double jdFrom = StelUtils::qDateTimeToJd((QDateTime) ui->dateFrom->date());
	double jdTo = StelUtils::qDateTimeToJd((QDateTime) ui->dateTo->date());

	if (jdFrom > jdTo)
	{
		QMessageBox::warning(0, "Stellarium", q_("Start date greater than end date!"));
	}
	else if (jdTo-jdFrom > 365)
	{
		QMessageBox::warning(0, "Stellarium", q_("Time interval must be less than one year!"));
	}
	else
	{
		searchEvents();
	}
}

void MeteorShowerDialog::searchEvents(void)
{
	QList<MeteorShowerP> searchResult = plugin->searchEvents(ui->dateFrom->date(), ui->dateTo->date());

	//Fill list of events
	initListEvents();
	foreach (const MeteorShowerP& r, searchResult)
	{
		TreeWidgetItem *treeItem = new TreeWidgetItem(treeWidget);
		//Name
		treeItem->setText(ColumnName, r->getNameI18n());
		//ZHR
		QString zhr = r->getZHR()==-1?q_("Variable"):QString::number(r->getZHR());
		treeItem->setText(ColumnZHR, zhr);
		//Data Type
		QString dataType = r->getStatus()==1?q_("Real"):q_("Generic");
		treeItem->setText(ColumnDataType, dataType);
		//Peak
		QString peak = r->getPeak().toString("dd/MMM/yyyy");
		treeItem->setText(ColumnPeak, peak);
	}
}

void MeteorShowerDialog::selectEvent(const QModelIndex &modelIndex)
{
	Q_UNUSED(modelIndex);
	StelCore *core = StelApp::getInstance().getCore();

	// if user select an event but the plugin is disabled,
	// plugin is enabled automatically
	if (!plugin->getFlagShowMS())
	{
		plugin->setFlagShowMS(true);
	}

	//Change date
	QString dateString = treeWidget->currentItem()->text(ColumnPeak);
	QDateTime qDateTime = QDateTime::fromString(dateString, "dd/MMM/yyyy");
	core->setJDay(StelUtils::qDateTimeToJd(qDateTime));

	//Select object
	QString namel18n = treeWidget->currentItem()->text(ColumnName);
	StelObjectMgr* objectMgr = GETSTELMODULE(StelObjectMgr);
	if (objectMgr->findAndSelectI18n(namel18n) || objectMgr->findAndSelect(namel18n))
	{
		//Move to object
		StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
		mvmgr->moveToObject(objectMgr->getSelectedObject()[0], mvmgr->getAutoMoveDuration());
		mvmgr->setFlagTracking(true);
	}
}

void MeteorShowerDialog::refreshRangeDates(void)
{
	int year = plugin->getSkyDate().toString("yyyy").toInt();
	ui->dateFrom->setDate(QDate(year, 1, 1)); // first date in the range - first day of the year
	ui->dateTo->setDate(QDate(year, 12, 31)); // second date in the range - last day of the year
}

void MeteorShowerDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Meteor Showers Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + METEORSHOWERS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Marcos Cardinot &lt;mcardinot@gmail.com&gt;</td></tr>";
	html += "</table>";

	html += "<p>" + q_("This plugin displays meteor showers and a marker for each active and inactive radiant, showing real information about its activity.") + "</p>";
	html += "<h3>" + q_("Terms") + "</h3>";
	html += "<p><b>" + q_("Meteor shower") + "</b>";
	html += "<br />" + q_("A meteor shower is a celestial event in which a number of meteors are observed to radiate, or originate, from one point in the night sky. These meteors are caused by streams of cosmic debris called meteoroids entering Earth's atmosphere at extremely high speeds on parallel trajectories. Most meteors are smaller than a grain of sand, so almost all of them disintegrate and never hit the Earth's surface. Intense or unusual meteor showers are known as meteor outbursts and meteor storms, which may produce greater than 1,000 meteors an hour.") + "</p>";
	html += "<p><b>" + q_("Radiant") + "</b>";
	html += "<br />" + q_("The radiant or apparent radiant of a meteor shower is the point in the sky, from which (to a planetary observer) meteors appear to originate. The Perseids, for example, are meteors which appear to come from a point within the constellation of Perseus.") + "</p>";
	html += "<p>" + q_("An observer might see such a meteor anywhere in the sky but the direction of motion, when traced back, will point to the radiant. A meteor that does not point back to the known radiant for a given shower is known as a sporadic and is not considered part of that shower.") + "</p>";
	html += "<p>" + q_("Many showers have a radiant point that changes position during the interval when it appears. For example, the radiant point for the Delta Aurigids drifts by more than a degree per night.") + "</p>";
	html += "<p><b>" + q_("Zenithal Hourly Rate (ZHR)") + "</b>";
	html += "<br />" + q_("In astronomy, the Zenithal Hourly Rate (ZHR) of a meteor shower is the number of meteors a single observer would see in one hour under a clear, dark sky (limiting apparent magnitude of 6.5) if the radiant of the shower were at the zenith. The rate that can effectively be seen is nearly always lower and decreases the closer the radiant is to the horizon.") + "</p>";
	html += "<p><b>" + q_("Population index") + "</b>";
	html += "<br />" + q_("The population index indicates the magnitude distribution of the meteor showers. The values below 2.5 correspond to distributions where bright meteors are more frequent than average, while values above 3.0 mean that the share of faint meteors is larger than usual.") + "</p>";
	html += "<h3>" + q_("Notes") + "</h3>";
	html += "<p>" + q_("This plugin was created as project of ESA Summer of Code in Space 2013.") + "</p>";
	html += "<h3>" + q_("Info") + "</h3>";
	html += "<p>" + q_("Info about meteor showers you can get here:") + "</p>";
	html += "<ul>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("%1Meteor shower%2 - article in Wikipedia").arg("<a href=\"https://en.wikipedia.org/wiki/Meteor_Showers\">")).arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("%1International Meteor Organization%2").arg("<a href=\"http://www.imo.net/\">")).arg("</a>") + "</li>";
	html += "</ul>";
	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Launchpad website.  Be sure to put \"%1\" in the subject when posting.")).arg("Meteor Showers Plugin") + "</p>";
	html += "<ul>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("If you have a question, you can %1get an answer here%2").arg("<a href=\"https://answers.launchpad.net/stellarium\">")).arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("Bug reports can be made %1here%2.")).arg("<a href=\"https://bugs.launchpad.net/stellarium\">").arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you would like to make a feature request, you can create a bug report, and set the severity to \"wishlist\".") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you want to read full information about the plugin, its history and format of the catalog you can %1get info here%2.").arg("<a href=\"http://stellarium.org/wiki/index.php/Meteor_Showers_plugin\">").arg("</a>") + "</li>";
	html += "</ul></body></html>";

	ui->aboutTextBrowser->setHtml(html);
}

void MeteorShowerDialog::refreshUpdateValues(void)
{
	ui->lastUpdateDateTimeEdit->setDateTime(plugin->getLastUpdate());
	ui->updateFrequencySpinBox->setValue(plugin->getUpdateFrequencyHours());
	int secondsToUpdate = plugin->getSecondsToUpdate();
	if (!plugin->getUpdatesEnabled())
	{
		ui->nextUpdateLabel->setText(q_("Internet updates disabled"));
	}
	else if (plugin->getUpdateState() == MeteorShowers::Updating)
	{
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	}
	else if (secondsToUpdate <= 60)
	{
		ui->nextUpdateLabel->setText(q_("Next update: < 1 minute"));
	}
	else if (secondsToUpdate < 3600)
	{
		ui->nextUpdateLabel->setText(QString(q_("Next update: %1 minutes")).arg((secondsToUpdate/60)+1));
	}
	else
	{
		ui->nextUpdateLabel->setText(QString(q_("Next update: %1 hours")).arg((secondsToUpdate/3600)+1));
	}
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
	{
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	}
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
	refreshMarkersColor();
}

void MeteorShowerDialog::saveSettings(void)
{
	plugin->saveSettingsToConfig();
}

void MeteorShowerDialog::updateJSON(void)
{
	if (plugin->getUpdatesEnabled())
	{
		plugin->updateJSON();
	}
}

void MeteorShowerDialog::refreshMarkersColor(void)
{
	ui->changeColorARG->setStyleSheet("background-color:" + plugin->getColorARG().name() + ";");
	ui->changeColorARR->setStyleSheet("background-color:" + plugin->getColorARR().name() + ";");
	ui->changeColorIR->setStyleSheet("background-color:" + plugin->getColorIR().name() + ";");
}

void MeteorShowerDialog::setColorARG()
{
	QColor color = QColorDialog::getColor(plugin->getColorARG());
	if (color.isValid())
	{
		ui->changeColorARG->setStyleSheet("background-color:" + color.name() + ";");
		plugin->setColorARG(color);
	}
}

void MeteorShowerDialog::setColorARR()
{
	QColor color = QColorDialog::getColor(plugin->getColorARR());
	if (color.isValid())
	{
		ui->changeColorARR->setStyleSheet("background-color:" + color.name() + ";");
		plugin->setColorARR(color);
	}
}

void MeteorShowerDialog::setColorIR()
{
	QColor color = QColorDialog::getColor(plugin->getColorIR());
	if (color.isValid())
	{
		ui->changeColorIR->setStyleSheet("background-color:" + color.name() + ";");
		plugin->setColorIR(color);
	}
}
