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
	// TRANSLATORS: Part of full phrase: German set of navigational stars
	nsSets->addItem(q_("German"), "German");

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
	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Navigational Stars Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + NAVSTARS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + NAVSTARS_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td rowspan='2'><strong>" + q_("Authors") + ":</strong></td><td>Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td></tr>";
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

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Github website.  Be sure to put \"%1\" in the subject when posting.")).arg("Navigational Stars plugin") + "</p>";
	html += "<p/><ul>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you have a question, you can {get an answer here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://groups.google.com/forum/#!forum/stellarium\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("Bug reports and feature requests can be made {here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/issues\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you want to read full information about this plugin and its history, you can {get info here}.").toHtmlEscaped().replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/Navigational_Stars_plugin\">\\1</a>") + "</li>";
	html += "</ul></p><br/></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}

	ui->aboutTextBrowser->setHtml(html);
}
