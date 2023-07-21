/*
 * Navigational Stars plug-in for Stellarium
 *
 * Copyright (C) 2016 Alexander Wolf
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NavStars.hpp"
#include "NavStarsWindow.hpp"
#include "ui_navStarsWindow.h"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"
#include "StelGui.hpp"
#include "SolarSystem.hpp"
#include "StelUtils.hpp"

#include <QComboBox>

NavStarsWindow::NavStarsWindow() : StelDialog("NavStars"), ns(Q_NULLPTR)
{
	ui = new Ui_navStarsWindowForm();
}

NavStarsWindow::~NavStarsWindow()
{
	delete ui;
}

void NavStarsWindow::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
		populateNavigationalStarsSets();
		populateNavigationalStarsSetDescription();
		populateToday();
	}
}

void NavStarsWindow::createDialogContent()
{
	ns = GETSTELMODULE(NavStars);
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	populateNavigationalStarsSets();
	populateNavigationalStarsSetDescription();
	QString currentNsSetKey = ns->getCurrentNavigationalStarsSetKey();
	int idx = ui->nsSetComboBox->findData(currentNsSetKey, Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use AngloAmerican as default
		idx = ui->nsSetComboBox->findData(QVariant("AngloAmerican"), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->nsSetComboBox->setCurrentIndex(idx);
	connect(ui->nsSetComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setNavigationalStarsSet(int)));

	connectBoolProperty(ui->displayAtStartupCheckBox,	"NavStars.displayAtStartup");
	connectBoolProperty(ui->highlightWhenVisible,		"NavStars.highlightWhenVisible");
	connectBoolProperty(ui->limitInfoToNavStars,		"NavStars.limitInfoToNavStars");
	connectBoolProperty(ui->upperLimb,			"NavStars.upperLimb");
	connectBoolProperty(ui->tabulatedDisplay,		"NavStars.tabulatedDisplay");
	connectBoolProperty(ui->showExtraDecimals,		"NavStars.showExtraDecimals");
	connectBoolProperty(ui->useUTCCheckBox,			"NavStars.useUTCTime");

	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(saveSettings()));	
	connect(ui->pushButtonReset, SIGNAL(clicked()), this, SLOT(resetSettings()));

	populateToday();
	connect(ui->refreshData, SIGNAL(clicked()), this, SLOT(populateToday()));
	StelCore* core = StelApp::getInstance().getCore();
	connect(core, SIGNAL(dateChanged()), this, SLOT(populateToday()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(populateToday()));

	// About tab
	setAboutHtml();
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
}

void NavStarsWindow::saveSettings()
{
	ns->saveConfiguration();
}

void NavStarsWindow::resetSettings()
{
	if (askConfirmation())
	{
		qDebug() << "[NavStars] restore defaults...";
		ns->restoreDefaultConfiguration();
	}
	else
		qDebug() << "[NavStars] restore defaults is canceled...";
}

void NavStarsWindow::populateToday()
{
	StelCore* core = StelApp::getInstance().getCore();
	const double utcShift = core->getUTCOffset(core->getJD()) / 24.; // Fix DST shift...
	StelLocaleMgr* localeMgr = &StelApp::getInstance().getLocaleMgr();
	PlanetP sun = GETSTELMODULE(SolarSystem)->getSun();
	double duration;
	QString moonrise, moonset, dayBegin, dayEnd, civilTwilightBegin, civilTwilightEnd, nauticalTwilightBegin,
		nauticalTwilightEnd, astronomicalTwilightBegin, astronomicalTwilightEnd, dayDuration,
		civilTwilightDuration, nauticalTwilightDuration, astronomicalTwilightDuration, dash = QChar(0x2014);

	// Moon
	Vec4d moon = GETSTELMODULE(SolarSystem)->getMoon()->getRTSTime(core, 0.);
	if (moon[3]==30 || moon[3]<0 || moon[3]>50) // no moonrise on current date
		moonrise = dash;
	else
		moonrise = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(moon[0]+utcShift), true);

	if (moon[3]==40 || moon[3]<0 || moon[3]>50) // no moonset on current date
		moonset = dash;
	else
		moonset = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(moon[2]+utcShift), true);

	// day
	Vec4d day = sun->getRTSTime(core, 0.);
	if (day[3]==0.)
	{
		dayBegin = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(day[0]+utcShift), true);
		dayEnd = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(day[2]+utcShift), true);
		duration = qAbs(day[2]-day[0])*24.;
	}
	else
	{
		dayBegin = dayEnd = dash;
		duration = (day[3]>99.) ? 24. : 0.;
	}
	dayDuration = StelUtils::hoursToHmsStr(duration, true);

	// civil twilight
	Vec4d civilTwilight = sun->getRTSTime(core, -6.);
	if (civilTwilight[3]==0.)
	{
		civilTwilightBegin = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(civilTwilight[0]+utcShift), true);
		civilTwilightEnd = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(civilTwilight[2]+utcShift), true);
		duration = qAbs(civilTwilight[2]-civilTwilight[0])*24.;
	}
	else
	{
		civilTwilightBegin = civilTwilightEnd = dash;
		duration = (civilTwilight[3]>99.) ? 24. : 0.;
	}
	civilTwilightDuration = StelUtils::hoursToHmsStr(duration, true);

	// nautical twilight
	Vec4d nauticalTwilight = sun->getRTSTime(core, -12.);
	if (nauticalTwilight[3]==0.)
	{
		nauticalTwilightBegin = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(nauticalTwilight[0]+utcShift), true);
		nauticalTwilightEnd = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(nauticalTwilight[2]+utcShift), true);
		duration = qAbs(nauticalTwilight[2]-nauticalTwilight[0])*24.;
	}
	else
	{
		nauticalTwilightBegin = nauticalTwilightEnd = dash;
		duration = (nauticalTwilight[3]>99.) ? 24. : 0.;
	}
	nauticalTwilightDuration = StelUtils::hoursToHmsStr(duration, true);

	// astronomical twilight
	Vec4d astronomicalTwilight = sun->getRTSTime(core, -18.);
	if (astronomicalTwilight[3]==0.)
	{
		astronomicalTwilightBegin = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(astronomicalTwilight[0]+utcShift), true);
		astronomicalTwilightEnd = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(astronomicalTwilight[2]+utcShift), true);
		duration = qAbs(astronomicalTwilight[2]-astronomicalTwilight[0])*24.;
	}
	else
	{
		astronomicalTwilightBegin = astronomicalTwilightEnd = dash;
		duration = (astronomicalTwilight[3]>99.) ? 24. : 0.;
	}
	astronomicalTwilightDuration = StelUtils::hoursToHmsStr(duration, true);

	// fill the data
	ui->labelToday->setText(localeMgr->getPrintableDateLocal(core->getJD()));
	ui->labelDayBegin->setText(dayBegin);
	ui->labelDayEnd->setText(dayEnd);
	ui->labelDayDuration->setText(dayDuration);
	ui->labelCivilTwilightBegin->setText(civilTwilightBegin);
	ui->labelCivilTwilightEnd->setText(civilTwilightEnd);
	ui->labelCivilTwilightDuration->setText(civilTwilightDuration);
	ui->labelNauticalTwilightBegin->setText(nauticalTwilightBegin);
	ui->labelNauticalTwilightEnd->setText(nauticalTwilightEnd);
	ui->labelNauticalTwilightDuration->setText(nauticalTwilightDuration);
	ui->labelAstronomicalTwilightBegin->setText(astronomicalTwilightBegin);
	ui->labelAstronomicalTwilightEnd->setText(astronomicalTwilightEnd);
	ui->labelAstronomicalTwilightDuration->setText(astronomicalTwilightDuration);
	ui->labelMoonRise->setText(moonrise);
	ui->labelMoonSet->setText(moonset);

	// tooltips
	// TRANSLATORS: full phrase is "XX° below the horizon"
	QString belowHorizon = q_("below the horizon");
	ui->labelCivilTwilight->setToolTip(QString("6° %1").arg(belowHorizon));
	ui->labelNauticalTwilight->setToolTip(QString("12° %1").arg(belowHorizon));
	ui->labelAstronomicalTwilight->setToolTip(QString("18° %1").arg(belowHorizon));
}

void NavStarsWindow::populateNavigationalStarsSets()
{
	Q_ASSERT(ui->nsSetComboBox);

	QComboBox* nsSets = ui->nsSetComboBox;

	//Save the current selection to be restored later
	nsSets->blockSignals(true);
	int index = nsSets->currentIndex();
	QVariant selectedNsSetId = nsSets->itemData(index);
	nsSets->clear();

	// TRANSLATORS: Part of full phrase: Anglo-American set of navigational stars
	nsSets->addItem(q_("Anglo-American"), "AngloAmerican");
	// TRANSLATORS: Part of full phrase: French set of navigational stars
	nsSets->addItem(q_("French"), "French");
	// TRANSLATORS: Part of full phrase: Russian set of navigational stars
	nsSets->addItem(q_("Russian"), "Russian");
	// TRANSLATORS: Part of full phrase: Soviet aviation set of navigational stars
	nsSets->addItem(q_("Soviet aviation"), "USSRAvia");	
	// TRANSLATORS: Part of full phrase: German set of navigational stars
	nsSets->addItem(q_("German"), "German");

	// TRANSLATORS: Part of full phrase: Voskhod and Soyuz manned space programs set of navigational stars
	nsSets->addItem(q_("Voskhod and Soyuz manned space programs"), "USSRSpace");
	// TRANSLATORS: Part of full phrase: Apollo space program set of navigational stars
	nsSets->addItem(q_("Apollo space program"), "Apollo");
	
	// Telescope alignment stars
	nsSets->addItem("Gemini APS", "GeminiAPS");
	nsSets->addItem("Meade LX200", "MeadeLX200");
	nsSets->addItem("Meade ETX", "MeadeETX");
	nsSets->addItem("Meade Autostar #494", "MeadeAS494");
	nsSets->addItem("Meade Autostar #497", "MeadeAS497");
	nsSets->addItem("Celestron NexStar", "CelestronNS");
	nsSets->addItem("Skywatcher SynScan", "SkywatcherSS");
	nsSets->addItem("Vixen Starbook", "VixenSB");
	nsSets->addItem("Argo Navis", "ArgoNavis");
	nsSets->addItem("Sky Commander DSC", "SkyCommander");

	//Restore the selection
	index = nsSets->findData(selectedNsSetId, Qt::UserRole, Qt::MatchCaseSensitive);
	nsSets->setCurrentIndex(index);
	nsSets->blockSignals(false);
}

void NavStarsWindow::setNavigationalStarsSet(int nsSetID)
{
	QString currentNsSetID = ui->nsSetComboBox->itemData(nsSetID).toString();
	ns->setCurrentNavigationalStarsSetKey(currentNsSetID);
	populateNavigationalStarsSetDescription();
}

void NavStarsWindow::populateNavigationalStarsSetDescription(void)
{
	ui->nsSetDescription->setText(ns->getCurrentNavigationalStarsSetDescription());
}

void NavStarsWindow::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Navigational Stars Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + NAVSTARS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + NAVSTARS_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td rowspan='2'><strong>" + q_("Authors") + ":</strong></td><td>Alexander Wolf</td></tr>";
	html += "<tr><td>Andy Kirkham &lt;kirkham.andy@gmail.com&gt;</td></tr>";
	html += "</table>";

	html += "<p>" + q_("This plugin marks navigational stars from a selected set.") + "</p>";
	html += "<p/>";

	html += "<p>";
	html += q_("Additional information fields can be displayed by selecting \"Information &gt; Additional coordinates (from plugins)\"");
	html += "</p>";

	html += "<p>";
	html += q_("These fields are:");
	html += "<ul>";
	html += "<li>" + q_("GHA%1: The Greenwich Hour Angle of the first point of Aries.").arg("&#9800;") + "</li>";
	html += "<li>" + q_("SHA: Sidereal Hour Angle of the navigation star.") + "</li>";
	html += "<li>" + q_("LHA: The observer's Local Hour Angle to the navigation star.") + "</li>";
	//! TRANSLATORS: In Celestial Navigation "GP" is Ground Point, astronomers often use "sub-point" which is the geodetic location of a point where the star is at zenith.
	html += "<li>" + q_("GP: GHA/DEC: The navigation star's ground point as Greenwich Hour Angle and Declination.") + "</li>";	
	//! TRANSLATORS: In Celestial Navigation "AP" is Assumed Position, a point close by where the observer knows themselves to be. For example, from dead reckoning.
	html += "<li>" + q_("AP: LAT/LON: The observer's Assumed Position as geodetic latitude and longitude.") + "</li>";
	//! TRANSLATORS: In Celestial Navigation Hc is "computed height" from the Nautical Almanac where "height" is altitude. Likewise, Zn is computed azimuth, as seen from the AP.
	html += "<li>" + q_("Hc/Zn: The calculated height (altitude) and computed azimuth of navigation star, seen from AP.") + "</li>";
	html += "</ul></p>";

	html += "<p>";
	html += q_("The user has two different view options selected by \"Show information as a tabulated list\".");
	html += "</p>";

	html += "<p>";
	html += q_("When 'tabulated list' is selected the fields more closely follow the <em>The Nautical Almanac</em> format:");
	html += "<ul>";

	//! TRANSLATORS: In Celestial Navigation "height" is used where astronomers would use "altitude", Ho Height Observed
	html += "<li>" + q_("Ho: Simulated observed altitude of navigation star using a sextant.") + "</li>";
	html += "<li>" + q_("GHA%1: The Greenwich Hour Angle of the first point of Aries.").arg("&#9800;") + "</li>";
	html += "<li>" + q_("LMST: Local mean sidereal time.") + "</li>";
	html += "<li>" + q_("SHA: Sidereal Hour Angle of navigation star.") + "</li>";
	//! TRANSLATORS: celestial coordinate system, declination.
	html += "<li>" + q_("DEC: The navigation star's declination.") + "</li>";
	html += "<li>" + q_("GHA: The navigation star's Greenwich Hour Angle.") + "</li>";
	html += "<li>" + q_("LHA: The observer's Local Hour Angle to the navigation star.") + "</li>";
	//! TRANSLATORS: Geodetic coordinate system, latitude.
	html += "<li>" + q_("LAT: The observer's geodetic latitude.") + "</li>";
	//! TRANSLATORS: Geodetic coordinate system, longitude.
	html += "<li>" + q_("LON: The observer's geodetic longitude.") + "</li>";
	//! TRANSLATORS: The process of Sight Reduction outputs computed values. Hc computed height (altitude) for the AP
	html += "<li>" + q_("Hc: The AP calculated height (altitude) of navigation star.") + "</li>";
	//! TRANSLATORS: The process of Sight Reduction outputs computed values. Zn computed azimuth
	html += "<li>" + q_("Zn: The AP calculated azimuth of navigation star.") + "</li>";
	html += "</ul></p>";

	html += "<p>";
	html += q_("For further information please refer to the Stellarium User Guide.");
	html += "</p>";

	html += StelApp::getInstance().getModuleMgr().getStandardSupportLinksInfo("Navigational Stars plugin");
	html += "</body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}

	ui->aboutTextBrowser->setHtml(html);
}
