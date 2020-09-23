/*
 * ArchaeoLines plug-in for Stellarium
 *
 * Copyright (C) 2014 Georg Zotti
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

#include "ArchaeoLines.hpp"
#include "ArchaeoLinesDialog.hpp"
#include "ui_archaeoLinesDialog.h"

#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainView.hpp"
#include "StelOpenGL.hpp"

ArchaeoLinesDialog::ArchaeoLinesDialog()
	: StelDialog("ArchaeoLines")
	, al(Q_NULLPTR)
{
	ui = new Ui_archaeoLinesDialog();
}

ArchaeoLinesDialog::~ArchaeoLinesDialog()
{
	delete ui;          ui=Q_NULLPTR;
}

void ArchaeoLinesDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

void ArchaeoLinesDialog::createDialogContent()
{
	al = GETSTELMODULE(ArchaeoLines);
	ui->setupUi(dialog);

	// Kinetic scrolling
	kineticScrollingList << ui->aboutTextBrowser;
	StelGui* gui= static_cast<StelGui*>(StelApp::getInstance().getGui());
	enableKineticScrolling(gui->getFlagUseKineticScrolling());
	connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connectIntProperty(ui->lineWidthSpinBox, "ArchaeoLines.lineWidth");

	connectBoolProperty(ui->equinoxCheckBox,         "ArchaeoLines.flagShowEquinox");
	connectBoolProperty(ui->solsticesCheckBox,       "ArchaeoLines.flagShowSolstices");
	connectBoolProperty(ui->crossquarterCheckBox,    "ArchaeoLines.flagShowCrossquarters");
	connectBoolProperty(ui->majorStandstillCheckBox, "ArchaeoLines.flagShowMajorStandstills");
	connectBoolProperty(ui->minorStandstillCheckBox, "ArchaeoLines.flagShowMinorStandstills");
	connectBoolProperty(ui->zenithPassageCheckBox,   "ArchaeoLines.flagShowZenithPassage");
	connectBoolProperty(ui->nadirPassageCheckBox,    "ArchaeoLines.flagShowNadirPassage");
	connectBoolProperty(ui->selectedObjectCheckBox,  "ArchaeoLines.flagShowSelectedObject");
	connectBoolProperty(ui->selectedObjectAzimuthCheckBox,    "ArchaeoLines.flagShowSelectedObjectAzimuth");
	connectBoolProperty(ui->selectedObjectHourAngleCheckBox,  "ArchaeoLines.flagShowSelectedObjectHourAngle");
	connectBoolProperty(ui->currentSunCheckBox,      "ArchaeoLines.flagShowCurrentSun");
	connectBoolProperty(ui->currentMoonCheckBox,     "ArchaeoLines.flagShowCurrentMoon");

	connectIntProperty(ui->currentPlanetComboBox, "ArchaeoLines.enumShowCurrentPlanet");

	connectBoolProperty(ui->geographicLocation1CheckBox,                 "ArchaeoLines.flagShowGeographicLocation1");
	connectBoolProperty(ui->geographicLocation2CheckBox,                 "ArchaeoLines.flagShowGeographicLocation2");
	connectDoubleProperty(ui->geographicLocation1LongitudeDoubleSpinBox, "ArchaeoLines.geographicLocation1Longitude");
	connectDoubleProperty(ui->geographicLocation1LatitudeDoubleSpinBox,  "ArchaeoLines.geographicLocation1Latitude");
	connectDoubleProperty(ui->geographicLocation2LongitudeDoubleSpinBox, "ArchaeoLines.geographicLocation2Longitude");
	connectDoubleProperty(ui->geographicLocation2LatitudeDoubleSpinBox,  "ArchaeoLines.geographicLocation2Latitude");
	ui->geographicLocation1LineEdit->setText(al->getLineLabel(ArchaeoLine::GeographicLocation1));
	ui->geographicLocation2LineEdit->setText(al->getLineLabel(ArchaeoLine::GeographicLocation2));
	connect(ui->geographicLocation1LineEdit, SIGNAL(textChanged(QString)), al, SLOT(setGeographicLocation1Label(QString)));
	connect(ui->geographicLocation2LineEdit, SIGNAL(textChanged(QString)), al, SLOT(setGeographicLocation2Label(QString)));

	connectBoolProperty(ui->customAzimuth1CheckBox,        "ArchaeoLines.flagShowCustomAzimuth1");
	connectBoolProperty(ui->customAzimuth2CheckBox,        "ArchaeoLines.flagShowCustomAzimuth2");
	connectDoubleProperty(ui->customAzimuth1DoubleSpinBox, "ArchaeoLines.customAzimuth1");
	connectDoubleProperty(ui->customAzimuth2DoubleSpinBox, "ArchaeoLines.customAzimuth2");
	ui->customAzimuth1LineEdit->setText(al->getLineLabel(ArchaeoLine::CustomAzimuth1));
	ui->customAzimuth2LineEdit->setText(al->getLineLabel(ArchaeoLine::CustomAzimuth2));
	connect(ui->customAzimuth1LineEdit, SIGNAL(textChanged(QString)), al, SLOT(setCustomAzimuth1Label(QString)));
	connect(ui->customAzimuth2LineEdit, SIGNAL(textChanged(QString)), al, SLOT(setCustomAzimuth2Label(QString)));

	connectBoolProperty(ui->customDeclination1CheckBox,        "ArchaeoLines.flagShowCustomDeclination1");
	connectBoolProperty(ui->customDeclination2CheckBox,        "ArchaeoLines.flagShowCustomDeclination2");
	connectDoubleProperty(ui->customDeclination1DoubleSpinBox, "ArchaeoLines.customDeclination1");
	connectDoubleProperty(ui->customDeclination2DoubleSpinBox, "ArchaeoLines.customDeclination2");
	ui->customDeclination1LineEdit->setText(al->getLineLabel(ArchaeoLine::CustomDeclination1));
	ui->customDeclination2LineEdit->setText(al->getLineLabel(ArchaeoLine::CustomDeclination2));
	connect(ui->customDeclination1LineEdit, SIGNAL(textChanged(QString)), al, SLOT(setCustomDeclination1Label(QString)));
	connect(ui->customDeclination2LineEdit, SIGNAL(textChanged(QString)), al, SLOT(setCustomDeclination2Label(QString)));

	connectColorButton(ui->equinoxColorToolButton,                 "ArchaeoLines.equinoxColor",                 "ArchaeoLines/color_equinox");
	connectColorButton(ui->solsticesColorToolButton,               "ArchaeoLines.solsticesColor",               "ArchaeoLines/color_solstices");
	connectColorButton(ui->crossquarterColorToolButton,            "ArchaeoLines.crossquartersColor",           "ArchaeoLines/color_crossquarters");
	connectColorButton(ui->majorStandstillColorToolButton,         "ArchaeoLines.majorStandstillColor",         "ArchaeoLines/color_major_standstill");
	connectColorButton(ui->minorStandstillColorToolButton,         "ArchaeoLines.minorStandstillColor",         "ArchaeoLines/color_minor_standstill");
	connectColorButton(ui->zenithPassageColorToolButton,           "ArchaeoLines.zenithPassageColor",           "ArchaeoLines/color_zenith_passage");
	connectColorButton(ui->nadirPassageColorToolButton,            "ArchaeoLines.nadirPassageColor",            "ArchaeoLines/color_nadir_passage");
	connectColorButton(ui->selectedObjectColorToolButton,          "ArchaeoLines.selectedObjectColor",          "ArchaeoLines/color_selected_object");
	connectColorButton(ui->selectedObjectAzimuthColorToolButton,   "ArchaeoLines.selectedObjectAzimuthColor",   "ArchaeoLines/color_selected_object_azimuth");
	connectColorButton(ui->selectedObjectHourAngleColorToolButton, "ArchaeoLines.selectedObjectHourAngleColor", "ArchaeoLines/color_selected_object_hour_angle");
	connectColorButton(ui->currentSunColorToolButton,              "ArchaeoLines.currentSunColor",              "ArchaeoLines/color_current_sun");
	connectColorButton(ui->currentMoonColorToolButton,             "ArchaeoLines.currentMoonColor",             "ArchaeoLines/color_current_moon");
	connectColorButton(ui->currentPlanetColorToolButton,           "ArchaeoLines.currentPlanetColor",           "ArchaeoLines/color_current_planet");
	connectColorButton(ui->geographicLocation1ColorToolButton,     "ArchaeoLines.geographicLocation1Color",     "ArchaeoLines/color_geographic_location_1");
	connectColorButton(ui->geographicLocation2ColorToolButton,     "ArchaeoLines.geographicLocation2Color",     "ArchaeoLines/color_geographic_location_2");
	connectColorButton(ui->customAzimuth1ColorToolButton,          "ArchaeoLines.customAzimuth1Color",          "ArchaeoLines/color_custom_azimuth_1");
	connectColorButton(ui->customAzimuth2ColorToolButton,          "ArchaeoLines.customAzimuth2Color",          "ArchaeoLines/color_custom_azimuth_2");
	connectColorButton(ui->customDeclination1ColorToolButton,      "ArchaeoLines.customDeclination1Color",      "ArchaeoLines/color_custom_declination_1");
	connectColorButton(ui->customDeclination2ColorToolButton,      "ArchaeoLines.customDeclination2Color",      "ArchaeoLines/color_custom_declination_2");

	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(resetArchaeoLinesSettings()));
	connect(ui->restoreDefaultsButtonCL, SIGNAL(clicked()), this, SLOT(resetArchaeoLinesSettings()));

	setAboutHtml();
}

void ArchaeoLinesDialog::setAboutHtml(void)
{
	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("ArchaeoLines Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + ARCHAEOLINES_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + ARCHAEOLINES_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Georg Zotti</td></tr>";
	//html += "<tr><td><strong>" + q_("Contributors") + ":</strong></td><td> List with br separators </td></tr>";
	html += "</table>";

	html += "<p>" + q_("The ArchaeoLines plugin displays any combination of declination arcs most relevant to archaeo- or ethnoastronomical studies.") + "</p>";
	html += "<ul><li>" + q_("Declinations of equinoxes (i.e. equator) and the solstices") + "</li>";
	html += "<li>" + q_("Declinations of the crossquarter days (days right between solstices and equinoxes)") + "</li>";
	html += "<li>" + q_("Declinations of the Major Lunar Standstills") + "</li>";
	html += "<li>" + q_("Declinations of the Minor Lunar Standstills") + "</li>";
	html += "<li>" + q_("Declination of the Zenith passage") + "</li>";
	html += "<li>" + q_("Declination of the Nadir passage") + "</li>";
	html += "<li>" + q_("Declination of the currently selected object") + "</li>";
	html += "<li>" + q_("Azimuth of the currently selected object") + "</li>";
	html += "<li>" + q_("Hour Angle of the currently selected object") + "</li>";
	html += "<li>" + q_("Current declination of the sun") + "</li>";
	html += "<li>" + q_("Current declination of the moon") + "</li>";
	html += "<li>" + q_("Current declination of a naked-eye planet") + "</li></ul>";
	html += "<p>" + q_("The lunar lines include horizon parallax effects. "
			   "There are two lines each drawn, for maximum and minimum distance of the moon. "
			   "Note that declination of the moon at the major standstill can exceed the "
			   "indicated limits if it is high in the sky due to parallax effects.") + "</p>";	
	html += "<p>" + q_("Some religions, most notably Islam, adhere to a practice of observing a prayer direction towards a particular location. "
			   "Azimuth lines for two locations can be shown. Default locations are Mecca (Kaaba) and Jerusalem. "
			   "The directions are computed based on spherical trigonometry on a spherical Earth.") + "</p>";
	html += "<p>" + q_("In addition, up to two vertical lines with arbitrary azimuth and declination lines with custom label can be shown.") + "</p>";

	html += "<h3>" + q_("Publications") + "</h3>";
	html += "<p>"  + q_("If you use this plugin in your publications, please cite:") + "</p>";
	html += "<p><ul>";
	html += "<li>" + QString("{Georg Zotti: Open Source Virtual Archaeoastronomy}. Mediterranean Archaeology and Archaeometry, Vol. 16, No 4 (2016), pp. 17-24.")
			.toHtmlEscaped().replace(a_rx, "<a href=\"http://maajournal.com/Issues/2016/Vol16-4/Full3.pdf\">\\1</a>") + "</li>";
	html += "</ul></p>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Github website.  Be sure to put \"%1\" in the subject when posting.")).arg("ArchaeoLines plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you have a question, you can {get an answer here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://groups.google.com/forum/#!forum/stellarium\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("Bug reports and feature requests can be made {here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/issues\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you want to read full information about this plugin and its history, you can {get info here}.").toHtmlEscaped().replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/ArchaeoLines_plugin\">\\1</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}

	ui->aboutTextBrowser->setHtml(html);
}


void ArchaeoLinesDialog::resetArchaeoLinesSettings()
{
	if (askConfirmation())
	{
		qDebug() << "[ArchaeoLines] restore defaults...";
		al->restoreDefaultSettings();
		ui->geographicLocation1LineEdit->setText(al->getLineLabel(ArchaeoLine::GeographicLocation1));
		ui->geographicLocation2LineEdit->setText(al->getLineLabel(ArchaeoLine::GeographicLocation2));
		ui->customAzimuth1LineEdit->setText(al->getLineLabel(ArchaeoLine::CustomAzimuth1));
		ui->customAzimuth2LineEdit->setText(al->getLineLabel(ArchaeoLine::CustomAzimuth2));
		ui->customDeclination1LineEdit->setText(al->getLineLabel(ArchaeoLine::CustomDeclination1));
		ui->customDeclination2LineEdit->setText(al->getLineLabel(ArchaeoLine::CustomDeclination2));
	}
	else
		qDebug() << "[ArchaeoLines] restore defaults is canceled...";
}

// Notes/Observations by GZ in 2015-04 with Qt5.4.0/MinGW on Windows7SP1.
// (1) There are issues in calling the QColorPanel that seem to be related to QTBUG-35302,
// although it was reportedly fixed at least for X11 in Qt5.3.0.
// On Win7 with NVidia Geforce and Win8.1 on Radeon, the color panel hides behind the Stellarium main window if set to fullscreen.
// On Win7 with Intel HD4600, and various Linuces, no problem is seen, the color panel is right on top of the fullscreen main window.
// It seems not to depend on MinGW vs. MSVC builds, but on details in GPU drivers and Qt.

// (2) Likely another bug in QColorDialog: If you choose one of the preconfigured colors (left half),
// on next change of that color it will have toggled one high bit of one component.
// On next change, it will be toggled again.
// If you configure a color from the right color field, all is OK (unless you "hit" a preconfigured color in the right field).
