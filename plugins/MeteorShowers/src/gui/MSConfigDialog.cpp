/*
 * Stellarium: Meteor Showers Plug-in
 * Copyright (C) 2013-2015 Marcos Cardinot
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

#include "MSConfigDialog.hpp"
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
#include "ui_MSConfigDialog.h"

MSConfigDialog::MSConfigDialog(MeteorShowersMgr* mgr)
	: m_ui(new Ui_MSConfigDialog)
	, m_updateTimer(NULL)
	, treeWidget(NULL)
{
}

MSConfigDialog::~MSConfigDialog()
{
	if (m_updateTimer)
	{
		m_updateTimer->stop();
		delete m_updateTimer;
		m_updateTimer = NULL;
	}
	delete m_ui;
}

void MSConfigDialog::retranslate()
{
	if (dialog)
	{
		m_ui->retranslateUi(dialog);
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

void MSConfigDialog::createDialogContent()
{
	m_ui->setupUi(dialog);
	m_ui->tabs->setCurrentIndex(0);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));

#ifdef Q_OS_WIN
	// Kinetic scrolling for tablet pc and pc
	QList<QWidget *> addscroll;
	addscroll << m_ui->listEvents << m_ui->aboutTextBrowser;
	installKineticScrolling(addscroll);
#endif

	// Settings tab / updates group	
	connect(m_ui->internetUpdates, SIGNAL(clicked(bool)), this, SLOT(setUpdatesEnabled(bool)));
	connect(m_ui->updateButton, SIGNAL(clicked()), this, SLOT(updateJSON()));
	//connect(m_mgr, SIGNAL(updateStateChanged(MeteorShowers::UpdateState)), this, SLOT(updateStateReceiver(MeteorShowers::UpdateState)));
	//connect(m_mgr, SIGNAL(jsonUpdateComplete(void)), this, SLOT(updateCompleteReceiver(void)));
	connect(m_ui->updateFrequencySpinBox, SIGNAL(valueChanged(int)), this, SLOT(setUpdateValues(int)));
	refreshUpdateValues(); // fetch values for last updated and so on

	m_updateTimer = new QTimer(this);
	connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
	m_updateTimer->start(7000);

	// Settings tab / event group
	connect(m_ui->searchButton, SIGNAL(clicked()), this, SLOT(checkDates()));
	refreshRangeDates(StelApp::getInstance().getCore());

	treeWidget = m_ui->listEvents;
	initListEvents();
	connect(treeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectEvent(QModelIndex)));
	connect(treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(repaintTreeWidget()));
/*
	// Settings tab / radiant group
	m_ui->displayRadiant->setChecked(m_mgr->getFlagRadiant());
	connect(m_ui->displayRadiant, SIGNAL(clicked(bool)), m_mgr, SLOT(setFlagRadiant(bool)));
	m_ui->activeRadiantsOnly->setChecked(m_mgr->getFlagActiveRadiant());
	connect(m_ui->activeRadiantsOnly, SIGNAL(clicked(bool)), m_mgr, SLOT(setFlagActiveRadiant(bool)));
	m_ui->radiantLabels->setChecked(m_mgr->getFlagLabels());
	connect(m_ui->radiantLabels, SIGNAL(clicked(bool)), m_mgr, SLOT(setFlagLabels(bool)));
	m_ui->fontSizeSpinBox->setValue(m_mgr->getLabelFont().pixelSize());
	connect(m_ui->fontSizeSpinBox, SIGNAL(valueChanged(int)), m_mgr, SLOT(setLabelFontSize(int)));

	// Settings tab / meteor showers group
	m_ui->displayMeteorShower->setChecked(m_mgr->getEnableAtStartup());
	connect(m_ui->displayMeteorShower, SIGNAL(clicked(bool)), m_mgr, SLOT(setEnableAtStartup(bool)));
	m_ui->displayShowMeteorShowerButton->setChecked(m_mgr->getFlagShowMSButton());
	connect(m_ui->displayShowMeteorShowerButton, SIGNAL(clicked(bool)), m_mgr, SLOT(setFlagShowMSButton(bool)));
*/
	// /////////////////////////////////////////

	connect(m_ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(m_ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
	connect(m_ui->saveSettingsButton, SIGNAL(clicked()), this, SLOT(saveSettings()));

	// Markers tab
	refreshMarkersColor();
	connect(m_ui->changeColorARG, SIGNAL(clicked()), this, SLOT(setColorARG()));
	connect(m_ui->changeColorARR, SIGNAL(clicked()), this, SLOT(setColorARR()));
	connect(m_ui->changeColorIR, SIGNAL(clicked()), this, SLOT(setColorIR()));

	// About tab
	setAboutHtml();
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui != NULL)
	{
		m_ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	}

	updateGuiFromSettings();
}

void MSConfigDialog::repaintTreeWidget()
{
	// Enable force repaint listEvents to avoiding artifacts
	// Seems bug in Qt5. Details: https://bugs.launchpad.net/stellarium/+bug/1350669
	treeWidget->repaint();
}

void MSConfigDialog::initListEvents(void)
{
	treeWidget->clear();
	treeWidget->setColumnCount(ColumnCount);
	setHeaderNames();
	treeWidget->header()->setSectionsMovable(false);
	treeWidget->header()->setStretchLastSection(true);
}

void MSConfigDialog::setHeaderNames(void)
{
	QStringList headerStrings;
	headerStrings << q_("Name");
	headerStrings << q_("ZHR");
	headerStrings << q_("Data Type");
	headerStrings << q_("Peak");
	treeWidget->setHeaderLabels(headerStrings);
}

void MSConfigDialog::checkDates(void)
{
	double jdFrom = StelUtils::qDateTimeToJd((QDateTime) m_ui->dateFrom->date());
	double jdTo = StelUtils::qDateTimeToJd((QDateTime) m_ui->dateTo->date());

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

void MSConfigDialog::searchEvents(void)
{
	QList<MeteorShowerP> searchResult = GETSTELMODULE(MeteorShowers)->searchEvents(m_ui->dateFrom->date(), m_ui->dateTo->date());

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

void MSConfigDialog::selectEvent(const QModelIndex &modelIndex)
{
	Q_UNUSED(modelIndex);
	StelCore *core = StelApp::getInstance().getCore();

	// if user select an event but the m_mgr is disabled,
	// 'MeteorShowers' is enabled automatically
	if (!GETSTELMODULE(MeteorShowers)->getEnablePlugin())
	{
		//GETSTELMODULE(MeteorShowers)->setEnableMeteorShowers(true);
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

void MSConfigDialog::refreshRangeDates(StelCore* core)
{
	double JD = core->getJDay();
	QDateTime skyDate = StelUtils::jdToQDateTime(JD+StelUtils::getGMTShiftFromQT(JD)/24-core->getDeltaT(JD)/86400);
	int year = skyDate.toString("yyyy").toInt();
	m_ui->dateFrom->setDate(QDate(year, 1, 1)); // first date in the range - first day of the year
	m_ui->dateTo->setDate(QDate(year, 12, 31)); // second date in the range - last day of the year
}

void MSConfigDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Meteor Showers Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + METEORSHOWERS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Marcos Cardinot &lt;mcardinot@gmail.com&gt;</td></tr>";
	html += "</table>";

	html += "<p>" + q_("This m_mgr displays meteor showers and a marker for each active and inactive radiant, showing real information about its activity.") + "</p>";
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
	html += "<p>" + q_("This m_mgr was created as project of ESA Summer of Code in Space 2013.") + "</p>";
	html += "<h3>" + q_("Info") + "</h3>";
	html += "<p>" + q_("Info about meteor showers you can get here:") + "</p>";
	html += "<ul>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("%1Meteor shower%2 - article in Wikipedia").arg("<a href=\"https://en.wikipedia.org/wiki/Meteor_Showers\">")).arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("%1International Meteor Organization%2").arg("<a href=\"http://www.imo.net/\">")).arg("</a>") + "</li>";
	html += "</ul>";
	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Launchpad website.  Be sure to put \"%1\" in the subject when posting.")).arg("Meteor Showers m_mgr") + "</p>";
	html += "<ul>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("If you have a question, you can %1get an answer here%2").arg("<a href=\"https://answers.launchpad.net/stellarium\">")).arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("Bug reports can be made %1here%2.")).arg("<a href=\"https://bugs.launchpad.net/stellarium\">").arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you would like to make a feature request, you can create a bug report, and set the severity to \"wishlist\".") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you want to read full information about the m_mgr, its history and format of the catalog you can %1get info here%2.").arg("<a href=\"http://stellarium.org/wiki/index.php/Meteor_Showers_m_mgr\">").arg("</a>") + "</li>";
	html += "</ul></body></html>";

	m_ui->aboutTextBrowser->setHtml(html);
}

void MSConfigDialog::refreshUpdateValues(void)
{
	m_ui->lastUpdateDateTimeEdit->setDateTime(GETSTELMODULE(MeteorShowers)->getLastUpdate());
	m_ui->updateFrequencySpinBox->setValue(GETSTELMODULE(MeteorShowers)->getUpdateFrequencyHours());
	int secondsToUpdate = GETSTELMODULE(MeteorShowers)->getSecondsToNextUpdate();
	if (!GETSTELMODULE(MeteorShowers)->getEnableUpdates())
	{
		m_ui->nextUpdateLabel->setText(q_("Internet updates disabled"));
	}
	else if (GETSTELMODULE(MeteorShowers)->isUpdating())
	{
		m_ui->nextUpdateLabel->setText(q_("Updating now..."));
	}
	else if (secondsToUpdate <= 60)
	{
		m_ui->nextUpdateLabel->setText(q_("Next update: < 1 minute"));
	}
	else if (secondsToUpdate < 3600)
	{
		m_ui->nextUpdateLabel->setText(QString(q_("Next update: %1 minutes")).arg((secondsToUpdate/60)+1));
	}
	else
	{
		m_ui->nextUpdateLabel->setText(QString(q_("Next update: %1 hours")).arg((secondsToUpdate/3600)+1));
	}
}

void MSConfigDialog::setUpdateValues(int hours)
{
	GETSTELMODULE(MeteorShowers)->setUpdateFrequencyHours(hours);
	refreshUpdateValues();
}

void MSConfigDialog::setUpdatesEnabled(bool checkState)
{
	GETSTELMODULE(MeteorShowers)->setEnableUpdates(checkState);
	m_ui->updateFrequencySpinBox->setEnabled(checkState);
	m_ui->updateButton->setText(q_("Update now"));
	refreshUpdateValues();
}

/*
void MSConfigDialog::updateStateReceiver(MeteorShowers::UpdateState state)
{
	//qDebug() << "MSConfigDialog::updateStateReceiver got a signal";
	if (state==MeteorShowers::Updating)
	{
		m_ui->nextUpdateLabel->setText(q_("Updating now..."));
	}
	else if (state==MeteorShowers::DownloadError || state==MeteorShowers::OtherError)
	{
		m_ui->nextUpdateLabel->setText(q_("Update error"));
		m_updateTimer->start();  // make sure message is displayed for a while...
	}
}
*/
void MSConfigDialog::updateCompleteReceiver(void)
{
	m_ui->nextUpdateLabel->setText(QString(q_("Meteor showers is updated")));
	// display the status for another full interval before refreshing status
	m_updateTimer->start();
	m_ui->lastUpdateDateTimeEdit->setDateTime(GETSTELMODULE(MeteorShowers)->getLastUpdate());
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
}

void MSConfigDialog::restoreDefaults(void)
{
	qDebug() << "MeteorShowers::restoreDefaults";
	GETSTELMODULE(MeteorShowers)->restoreDefaultSettings();
	updateGuiFromSettings();
}

void MSConfigDialog::updateGuiFromSettings(void)
{	
	refreshUpdateValues();
	refreshMarkersColor();
}

void MSConfigDialog::saveSettings(void)
{
	//GETSTELMODULE(MeteorShowers)->saveSettingsToConfig();
}

void MSConfigDialog::updateJSON(void)
{
	//if (GETSTELMODULE(MeteorShowers)->getUpdatesEnabled())
	//{
//		GETSTELMODULE(MeteorShowers)->updateJSON();
	//}
}

void MSConfigDialog::refreshMarkersColor(void)
{
	// TODO
	/*
	m_ui->changeColorARG->setStyleSheet("background-color:" + GETSTELMODULE(MeteorShowers)->getColorARG().name() + ";");
	m_ui->changeColorARR->setStyleSheet("background-color:" + GETSTELMODULE(MeteorShowers)->getColorARR().name() + ";");
	m_ui->changeColorIR->setStyleSheet("background-color:" + GETSTELMODULE(MeteorShowers)->getColorIR().name() + ";");
	*/
}

void MSConfigDialog::setColorARG()
{
	// TODO
	/*
	QColor color = QColorDialog::getColor(GETSTELMODULE(MeteorShowers)->getColorARG());
	if (color.isValid())
	{
		m_ui->changeColorARG->setStyleSheet("background-color:" + color.name() + ";");
		GETSTELMODULE(MeteorShowers)->setColorARG(color);
	}
	*/
}

void MSConfigDialog::setColorARR()
{
	// TODO
	/*
	QColor color = QColorDialog::getColor(GETSTELMODULE(MeteorShowers)->getColorARR());
	if (color.isValid())
	{
		m_ui->changeColorARR->setStyleSheet("background-color:" + color.name() + ";");
		GETSTELMODULE(MeteorShowers)->setColorARR(color);
	}
	*/
}

void MSConfigDialog::setColorIR()
{
	// TODO
	/*
	QColor color = QColorDialog::getColor(GETSTELMODULE(MeteorShowers)->getColorIR());
	if (color.isValid())
	{
		m_ui->changeColorIR->setStyleSheet("background-color:" + color.name() + ";");
		GETSTELMODULE(MeteorShowers)->setColorIR(color);
	}
	*/
}
