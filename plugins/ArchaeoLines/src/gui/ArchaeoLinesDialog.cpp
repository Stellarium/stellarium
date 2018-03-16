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

#ifdef Q_OS_WIN
	//Kinetic scrolling for tablet pc and pc
	QList<QWidget *> addscroll;
	addscroll << ui->aboutTextBrowser;
	installKineticScrolling(addscroll);
#endif

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connectBoolProperty(ui->equinoxCheckBox,         "ArchaeoLines.flagShowEquinox");
	connectBoolProperty(ui->solsticesCheckBox,       "ArchaeoLines.flagShowSolstices");
	connectBoolProperty(ui->crossquarterCheckBox,    "ArchaeoLines.flagShowCrossquarters");
	connectBoolProperty(ui->majorStandstillCheckBox, "ArchaeoLines.flagShowMajorStandstills");
	connectBoolProperty(ui->minorStandstillCheckBox, "ArchaeoLines.flagShowMinorStandstills");
	connectBoolProperty(ui->zenithPassageCheckBox,   "ArchaeoLines.flagShowZenithPassage");
	connectBoolProperty(ui->nadirPassageCheckBox,    "ArchaeoLines.flagShowNadirPassage");
	connectBoolProperty(ui->selectedObjectCheckBox,  "ArchaeoLines.flagShowSelectedObject");
	connectBoolProperty(ui->currentSunCheckBox,      "ArchaeoLines.flagShowCurrentSun");
	connectBoolProperty(ui->currentMoonCheckBox,     "ArchaeoLines.flagShowCurrentMoon");
	// Planet ComboBox requires special handling!
	setCurrentPlanetFromApp();
	//connect(al, SIGNAL(currentPlanetChanged(ArchaeoLine::Line)), this, SLOT(setCurrentPlanetFromApp()));
	//connect(ui->currentPlanetComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setCurrentPlanetFromGUI(int)));
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

	{ // just to allow code folding.
	equinoxColor         = al->getLineColor(ArchaeoLine::Equinox);
	solsticeColor        = al->getLineColor(ArchaeoLine::Solstices);
	crossquarterColor    = al->getLineColor(ArchaeoLine::Crossquarters);
	majorStandstillColor = al->getLineColor(ArchaeoLine::MajorStandstill);
	minorStandstillColor = al->getLineColor(ArchaeoLine::MinorStandstill);
	zenithPassageColor   = al->getLineColor(ArchaeoLine::ZenithPassage);
	nadirPassageColor    = al->getLineColor(ArchaeoLine::NadirPassage);
	selectedObjectColor  = al->getLineColor(ArchaeoLine::SelectedObject);
	currentSunColor      = al->getLineColor(ArchaeoLine::CurrentSun);
	currentMoonColor     = al->getLineColor(ArchaeoLine::CurrentMoon);
	currentPlanetColor   = al->getLineColor(ArchaeoLine::CurrentPlanetNone);
	geographicLocation1Color  = al->getLineColor(ArchaeoLine::GeographicLocation1);
	geographicLocation2Color  = al->getLineColor(ArchaeoLine::GeographicLocation2);
	customAzimuth1Color  = al->getLineColor(ArchaeoLine::CustomAzimuth1);
	customAzimuth2Color  = al->getLineColor(ArchaeoLine::CustomAzimuth2);
	customDeclination1Color  = al->getLineColor(ArchaeoLine::CustomDeclination1);
	customDeclination2Color  = al->getLineColor(ArchaeoLine::CustomDeclination2);
	equinoxColorPixmap=QPixmap(48, 12);
	equinoxColorPixmap.fill(equinoxColor);
	ui->equinoxColorToolButton->setIconSize(QSize(48, 12));
	ui->equinoxColorToolButton->setIcon(QIcon(equinoxColorPixmap));
	solsticeColorPixmap=QPixmap(48, 12);
	solsticeColorPixmap.fill(solsticeColor);
	ui->solsticesColorToolButton->setIconSize(QSize(48, 12));
	ui->solsticesColorToolButton->setIcon(QIcon(solsticeColorPixmap));
	crossquarterColorPixmap=QPixmap(48, 12);
	crossquarterColorPixmap.fill(crossquarterColor);
	ui->crossquarterColorToolButton->setIconSize(QSize(48, 12));
	ui->crossquarterColorToolButton->setIcon(QIcon(crossquarterColorPixmap));
	minorStandstillColorPixmap=QPixmap(48, 12);
	minorStandstillColorPixmap.fill(minorStandstillColor);
	ui->minorStandstillColorToolButton->setIconSize(QSize(48, 12));
	ui->minorStandstillColorToolButton->setIcon(QIcon(minorStandstillColorPixmap));
	majorStandstillColorPixmap=QPixmap(48, 12);
	majorStandstillColorPixmap.fill(majorStandstillColor);
	ui->majorStandstillColorToolButton->setIconSize(QSize(48, 12));
	ui->majorStandstillColorToolButton->setIcon(QIcon(majorStandstillColorPixmap));
	zenithPassageColorPixmap=QPixmap(48, 12);
	zenithPassageColorPixmap.fill(zenithPassageColor);
	ui->zenithPassageColorToolButton->setIconSize(QSize(48, 12));
	ui->zenithPassageColorToolButton->setIcon(QIcon(zenithPassageColorPixmap));
	nadirPassageColorPixmap=QPixmap(48, 12);
	nadirPassageColorPixmap.fill(nadirPassageColor);
	ui->nadirPassageColorToolButton->setIconSize(QSize(48, 12));
	ui->nadirPassageColorToolButton->setIcon(QIcon(nadirPassageColorPixmap));
	selectedObjectColorPixmap=QPixmap(48, 12);
	selectedObjectColorPixmap.fill(selectedObjectColor);
	ui->selectedObjectColorToolButton->setIconSize(QSize(48, 12));
	ui->selectedObjectColorToolButton->setIcon(QIcon(selectedObjectColorPixmap));
	currentSunColorPixmap=QPixmap(48, 12);
	currentSunColorPixmap.fill(currentSunColor);
	ui->currentSunColorToolButton->setIconSize(QSize(48, 12));
	ui->currentSunColorToolButton->setIcon(QIcon(currentSunColorPixmap));
	currentMoonColorPixmap=QPixmap(48, 12);
	currentMoonColorPixmap.fill(currentMoonColor);
	ui->currentMoonColorToolButton->setIconSize(QSize(48, 12));
	ui->currentMoonColorToolButton->setIcon(QIcon(currentMoonColorPixmap));
	currentPlanetColorPixmap=QPixmap(48, 12);
	currentPlanetColorPixmap.fill(currentPlanetColor);
	ui->currentPlanetColorToolButton->setIconSize(QSize(48, 12));
	ui->currentPlanetColorToolButton->setIcon(QIcon(currentPlanetColorPixmap));
	geographicLocation1ColorPixmap=QPixmap(48, 12);
	geographicLocation1ColorPixmap.fill(geographicLocation1Color);
	ui->geographicLocation1ColorToolButton->setIconSize(QSize(48, 12));
	ui->geographicLocation1ColorToolButton->setIcon(QIcon(geographicLocation1ColorPixmap));
	geographicLocation2ColorPixmap=QPixmap(48, 12);
	geographicLocation2ColorPixmap.fill(geographicLocation2Color);
	ui->geographicLocation2ColorToolButton->setIconSize(QSize(48, 12));
	ui->geographicLocation2ColorToolButton->setIcon(QIcon(geographicLocation2ColorPixmap));
	customAzimuth1ColorPixmap=QPixmap(48, 12);
	customAzimuth1ColorPixmap.fill(customAzimuth1Color);
	ui->customAzimuth1ColorToolButton->setIconSize(QSize(48, 12));
	ui->customAzimuth1ColorToolButton->setIcon(QIcon(customAzimuth1ColorPixmap));
	customAzimuth2ColorPixmap=QPixmap(48, 12);
	customAzimuth2ColorPixmap.fill(customAzimuth2Color);
	ui->customAzimuth2ColorToolButton->setIconSize(QSize(48, 12));
	ui->customAzimuth2ColorToolButton->setIcon(QIcon(customAzimuth2ColorPixmap));
	customDeclination1ColorPixmap=QPixmap(48, 12);
	customDeclination1ColorPixmap.fill(customDeclination1Color);
	ui->customDeclination1ColorToolButton->setIconSize(QSize(48, 12));
	ui->customDeclination1ColorToolButton->setIcon(QIcon(customDeclination1ColorPixmap));
	customDeclination2ColorPixmap=QPixmap(48, 12);
	customDeclination2ColorPixmap.fill(customDeclination2Color);
	ui->customDeclination2ColorToolButton->setIconSize(QSize(48, 12));
	ui->customDeclination2ColorToolButton->setIcon(QIcon(customDeclination2ColorPixmap));
}

	connect(ui->equinoxColorToolButton,             SIGNAL(released()), this, SLOT(askEquinoxColor()));
	connect(ui->solsticesColorToolButton,           SIGNAL(released()), this, SLOT(askSolsticeColor()));
	connect(ui->crossquarterColorToolButton,        SIGNAL(released()), this, SLOT(askCrossquarterColor()));
	connect(ui->majorStandstillColorToolButton,     SIGNAL(released()), this, SLOT(askMajorStandstillColor()));
	connect(ui->minorStandstillColorToolButton,     SIGNAL(released()), this, SLOT(askMinorStandstillColor()));
	connect(ui->zenithPassageColorToolButton,       SIGNAL(released()), this, SLOT(askZenithPassageColor()));
	connect(ui->nadirPassageColorToolButton,        SIGNAL(released()), this, SLOT(askNadirPassageColor()));
	connect(ui->selectedObjectColorToolButton,      SIGNAL(released()), this, SLOT(askSelectedObjectColor()));
	connect(ui->currentSunColorToolButton,          SIGNAL(released()), this, SLOT(askCurrentSunColor()));
	connect(ui->currentMoonColorToolButton,         SIGNAL(released()), this, SLOT(askCurrentMoonColor()));
	connect(ui->currentPlanetColorToolButton,       SIGNAL(released()), this, SLOT(askCurrentPlanetColor()));
	connect(ui->geographicLocation1ColorToolButton, SIGNAL(released()), this, SLOT(askGeographicLocation1Color()));
	connect(ui->geographicLocation2ColorToolButton, SIGNAL(released()), this, SLOT(askGeographicLocation2Color()));
	connect(ui->customAzimuth1ColorToolButton,      SIGNAL(released()), this, SLOT(askCustomAzimuth1Color()));
	connect(ui->customAzimuth2ColorToolButton,      SIGNAL(released()), this, SLOT(askCustomAzimuth2Color()));
	connect(ui->customDeclination1ColorToolButton,  SIGNAL(released()), this, SLOT(askCustomDeclination1Color()));
	connect(ui->customDeclination2ColorToolButton,  SIGNAL(released()), this, SLOT(askCustomDeclination2Color()));

	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(resetArchaeoLinesSettings()));

	// We must apparently warn about a potential problem, but only on Windows in OpenGL mode. (QTBUG-35302?)
	#ifndef Q_OS_WIN
	ui->switchToWindowedModeLabel->hide();
	#else
	// don't call GL functions in GUI code please
	if (StelMainView::getInstance().getGLInformation().renderer.startsWith("ANGLE", Qt::CaseSensitive))
		ui->switchToWindowedModeLabel->hide();
	#endif
	setAboutHtml();
}

void ArchaeoLinesDialog::setCurrentPlanetFromGUI(int index)
{
	Q_ASSERT(al);
	al->showCurrentPlanet((ArchaeoLine::Line) (ArchaeoLine::CurrentPlanetNone+index));
}

void ArchaeoLinesDialog::setCurrentPlanetFromApp()
{
	Q_ASSERT(al);
	ui->currentPlanetComboBox->setCurrentIndex(al->whichCurrentPlanetDisplayed()-ArchaeoLine::CurrentPlanetNone);
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
	al->restoreDefaultSettings();
	equinoxColor         = al->getLineColor(ArchaeoLine::Equinox);
	solsticeColor        = al->getLineColor(ArchaeoLine::Solstices);
	crossquarterColor    = al->getLineColor(ArchaeoLine::Crossquarters);
	majorStandstillColor = al->getLineColor(ArchaeoLine::MajorStandstill);
	minorStandstillColor = al->getLineColor(ArchaeoLine::MinorStandstill);
	zenithPassageColor   = al->getLineColor(ArchaeoLine::ZenithPassage);
	nadirPassageColor    = al->getLineColor(ArchaeoLine::NadirPassage);
	selectedObjectColor  = al->getLineColor(ArchaeoLine::SelectedObject);
	currentSunColor      = al->getLineColor(ArchaeoLine::CurrentSun);
	currentMoonColor     = al->getLineColor(ArchaeoLine::CurrentMoon);
	currentPlanetColor   = al->getLineColor(ArchaeoLine::CurrentPlanetNone);
	geographicLocation1Color  = al->getLineColor(ArchaeoLine::GeographicLocation1);
	geographicLocation2Color  = al->getLineColor(ArchaeoLine::GeographicLocation2);
	customAzimuth1Color  = al->getLineColor(ArchaeoLine::CustomAzimuth1);
	customAzimuth2Color  = al->getLineColor(ArchaeoLine::CustomAzimuth2);
	customDeclination1Color  = al->getLineColor(ArchaeoLine::CustomDeclination1);
	customDeclination2Color  = al->getLineColor(ArchaeoLine::CustomDeclination2);
	equinoxColorPixmap.fill(equinoxColor);
	ui->equinoxColorToolButton->setIcon(QIcon(equinoxColorPixmap));
	solsticeColorPixmap.fill(solsticeColor);
	ui->solsticesColorToolButton->setIcon(QIcon(solsticeColorPixmap));
	crossquarterColorPixmap.fill(crossquarterColor);
	ui->crossquarterColorToolButton->setIcon(QIcon(crossquarterColorPixmap));
	minorStandstillColorPixmap.fill(minorStandstillColor);
	ui->minorStandstillColorToolButton->setIcon(QIcon(minorStandstillColorPixmap));
	majorStandstillColorPixmap.fill(majorStandstillColor);
	ui->majorStandstillColorToolButton->setIcon(QIcon(majorStandstillColorPixmap));
	zenithPassageColorPixmap.fill(zenithPassageColor);
	ui->zenithPassageColorToolButton->setIcon(QIcon(zenithPassageColorPixmap));
	nadirPassageColorPixmap.fill(nadirPassageColor);
	ui->nadirPassageColorToolButton->setIcon(QIcon(nadirPassageColorPixmap));
	selectedObjectColorPixmap.fill(selectedObjectColor);
	ui->selectedObjectColorToolButton->setIcon(QIcon(selectedObjectColorPixmap));
	currentSunColorPixmap.fill(currentSunColor);
	ui->currentSunColorToolButton->setIcon(QIcon(currentSunColorPixmap));
	currentMoonColorPixmap.fill(currentMoonColor);
	ui->currentMoonColorToolButton->setIcon(QIcon(currentMoonColorPixmap));
	currentPlanetColorPixmap.fill(currentPlanetColor);
	ui->currentPlanetColorToolButton->setIcon(QIcon(currentPlanetColorPixmap));
	geographicLocation1ColorPixmap.fill(geographicLocation1Color);
	ui->geographicLocation1ColorToolButton->setIcon(QIcon(geographicLocation1ColorPixmap));
	geographicLocation2ColorPixmap.fill(geographicLocation2Color);
	ui->geographicLocation2ColorToolButton->setIcon(QIcon(geographicLocation2ColorPixmap));
	customAzimuth1ColorPixmap.fill(customAzimuth1Color);
	ui->customAzimuth1ColorToolButton->setIcon(QIcon(customAzimuth1ColorPixmap));
	customAzimuth2ColorPixmap.fill(customAzimuth2Color);
	ui->customAzimuth2ColorToolButton->setIcon(QIcon(customAzimuth2ColorPixmap));
	customDeclination1ColorPixmap.fill(customDeclination1Color);
	ui->customDeclination1ColorToolButton->setIcon(QIcon(customDeclination1ColorPixmap));
	customDeclination2ColorPixmap.fill(customDeclination2Color);
	ui->customDeclination2ColorToolButton->setIcon(QIcon(customDeclination2ColorPixmap));

	ui->geographicLocation1LineEdit->setText(al->getLineLabel(ArchaeoLine::GeographicLocation1));
	ui->geographicLocation2LineEdit->setText(al->getLineLabel(ArchaeoLine::GeographicLocation2));
	ui->customAzimuth1LineEdit->setText(al->getLineLabel(ArchaeoLine::CustomAzimuth1));
	ui->customAzimuth2LineEdit->setText(al->getLineLabel(ArchaeoLine::CustomAzimuth2));
	ui->customDeclination1LineEdit->setText(al->getLineLabel(ArchaeoLine::CustomDeclination1));
	ui->customDeclination2LineEdit->setText(al->getLineLabel(ArchaeoLine::CustomDeclination2));
}

// These are called by the respective buttons.
void ArchaeoLinesDialog::askEquinoxColor()
{
	QColor c=QColorDialog::getColor(equinoxColor, Q_NULLPTR, q_("Select color for equinox line"));
	if (c.isValid())
	{
		equinoxColor=c;
		al->setLineColor(ArchaeoLine::Equinox, c);
		equinoxColorPixmap.fill(c);
		ui->equinoxColorToolButton->setIcon(QIcon(equinoxColorPixmap));
	}
}
void ArchaeoLinesDialog::askSolsticeColor()
{
	QColor c=QColorDialog::getColor(solsticeColor, Q_NULLPTR, q_("Select color for solstice lines"));
	if (c.isValid())
	{
		solsticeColor=c;
		al->setLineColor(ArchaeoLine::Solstices, c);
		solsticeColorPixmap.fill(c);
		ui->solsticesColorToolButton->setIcon(QIcon(solsticeColorPixmap));
	}
}
void ArchaeoLinesDialog::askCrossquarterColor()
{
	QColor c=QColorDialog::getColor(crossquarterColor, Q_NULLPTR, q_("Select color for crossquarter lines"));
	if (c.isValid())
	{
		crossquarterColor=c;
		al->setLineColor(ArchaeoLine::Crossquarters, c);
		crossquarterColorPixmap.fill(c);
		ui->crossquarterColorToolButton->setIcon(QIcon(crossquarterColorPixmap));
	}
}
void ArchaeoLinesDialog::askMajorStandstillColor()
{
	QColor c=QColorDialog::getColor(majorStandstillColor, Q_NULLPTR, q_("Select color for major standstill lines"));
	if (c.isValid())
	{
		majorStandstillColor=c;
		al->setLineColor(ArchaeoLine::MajorStandstill, c);
		majorStandstillColorPixmap.fill(c);
		ui->majorStandstillColorToolButton->setIcon(QIcon(majorStandstillColorPixmap));
	}
}
void ArchaeoLinesDialog::askMinorStandstillColor()
{
	QColor c=QColorDialog::getColor(minorStandstillColor, Q_NULLPTR, q_("Select color for minor standstill lines"));
	if (c.isValid())
	{
		minorStandstillColor=c;
		al->setLineColor(ArchaeoLine::MinorStandstill, c);
		minorStandstillColorPixmap.fill(c);
		ui->minorStandstillColorToolButton->setIcon(QIcon(minorStandstillColorPixmap));
	}
}
void ArchaeoLinesDialog::askZenithPassageColor()
{
	QColor c=QColorDialog::getColor(zenithPassageColor, Q_NULLPTR, q_("Select color for zenith passage line"));
	if (c.isValid())
	{
		zenithPassageColor=c;
		al->setLineColor(ArchaeoLine::ZenithPassage, c);
		zenithPassageColorPixmap.fill(c);
		ui->zenithPassageColorToolButton->setIcon(QIcon(zenithPassageColorPixmap));
	}
}
void ArchaeoLinesDialog::askNadirPassageColor()
{
	QColor c=QColorDialog::getColor(nadirPassageColor, Q_NULLPTR, q_("Select color for nadir passage line"));
	if (c.isValid())
	{
		nadirPassageColor=c;
		al->setLineColor(ArchaeoLine::NadirPassage, c);
		nadirPassageColorPixmap.fill(c);
		ui->nadirPassageColorToolButton->setIcon(QIcon(nadirPassageColorPixmap));
	}
}
void ArchaeoLinesDialog::askSelectedObjectColor()
{
	QColor c=QColorDialog::getColor(selectedObjectColor, Q_NULLPTR, q_("Select color for selected object line"));
	if (c.isValid())
	{
		selectedObjectColor=c;
		al->setLineColor(ArchaeoLine::SelectedObject, c);
		selectedObjectColorPixmap.fill(c);
		ui->selectedObjectColorToolButton->setIcon(QIcon(selectedObjectColorPixmap));
	}
}
void ArchaeoLinesDialog::askCurrentSunColor()
{
	QColor c=QColorDialog::getColor(currentSunColor, Q_NULLPTR, q_("Select color for current sun line"));
	if (c.isValid())
	{
		currentSunColor=c;
		al->setLineColor(ArchaeoLine::CurrentSun, c);
		currentSunColorPixmap.fill(c);
		ui->currentSunColorToolButton->setIcon(QIcon(currentSunColorPixmap));
	}
}
void ArchaeoLinesDialog::askCurrentMoonColor()
{
	QColor c=QColorDialog::getColor(currentMoonColor, Q_NULLPTR, q_("Select color for current moon line"));
	if (c.isValid())
	{
		currentMoonColor=c;
		al->setLineColor(ArchaeoLine::CurrentMoon, c);
		currentMoonColorPixmap.fill(c);
		ui->currentMoonColorToolButton->setIcon(QIcon(currentMoonColorPixmap));
	}
}
void ArchaeoLinesDialog::askCurrentPlanetColor()
{
	QColor c=QColorDialog::getColor(currentPlanetColor, Q_NULLPTR, q_("Select color for current planet line"));
	if (c.isValid())
	{
		currentPlanetColor=c;
		al->setLineColor(ArchaeoLine::CurrentPlanetNone, c);
		currentPlanetColorPixmap.fill(c);
		ui->currentPlanetColorToolButton->setIcon(QIcon(currentPlanetColorPixmap));
	}
}
void ArchaeoLinesDialog::askGeographicLocation1Color()
{
	QColor c=QColorDialog::getColor(geographicLocation1Color, Q_NULLPTR, q_("Select color for Geographic Location 1 line"));
	if (c.isValid())
	{
		geographicLocation1Color=c;
		al->setLineColor(ArchaeoLine::GeographicLocation1, c);
		geographicLocation1ColorPixmap.fill(c);
		ui->geographicLocation1ColorToolButton->setIcon(QIcon(geographicLocation1ColorPixmap));
	}
}
void ArchaeoLinesDialog::askGeographicLocation2Color()
{
	QColor c=QColorDialog::getColor(geographicLocation2Color, Q_NULLPTR, q_("Select color for Geographic Location 2 line"));
	if (c.isValid())
	{
		geographicLocation2Color=c;
		al->setLineColor(ArchaeoLine::GeographicLocation2, c);
		geographicLocation2ColorPixmap.fill(c);
		ui->geographicLocation2ColorToolButton->setIcon(QIcon(geographicLocation2ColorPixmap));
	}
}
void ArchaeoLinesDialog::askCustomAzimuth1Color()
{
	QColor c=QColorDialog::getColor(customAzimuth1Color, Q_NULLPTR, q_("Select color for Custom Azimuth 1 line"));
	if (c.isValid())
	{
		customAzimuth1Color=c;
		al->setLineColor(ArchaeoLine::CustomAzimuth1, c);
		customAzimuth1ColorPixmap.fill(c);
		ui->customAzimuth1ColorToolButton->setIcon(QIcon(customAzimuth1ColorPixmap));
	}
}
void ArchaeoLinesDialog::askCustomAzimuth2Color()
{
	QColor c=QColorDialog::getColor(customAzimuth2Color, Q_NULLPTR, q_("Select color for Custom Azimuth 2 line"));
	if (c.isValid())
	{
		customAzimuth2Color=c;
		al->setLineColor(ArchaeoLine::CustomAzimuth2, c);
		customAzimuth2ColorPixmap.fill(c);
		ui->customAzimuth2ColorToolButton->setIcon(QIcon(customAzimuth2ColorPixmap));
	}
}
void ArchaeoLinesDialog::askCustomDeclination1Color()
{
	QColor c=QColorDialog::getColor(customDeclination1Color, Q_NULLPTR, q_("Select color for Custom Declination 1 line"));
	if (c.isValid())
	{
		customDeclination1Color=c;
		al->setLineColor(ArchaeoLine::CustomDeclination1, c);
		customDeclination1ColorPixmap.fill(c);
		ui->customDeclination1ColorToolButton->setIcon(QIcon(customDeclination1ColorPixmap));
	}
}
void ArchaeoLinesDialog::askCustomDeclination2Color()
{
	QColor c=QColorDialog::getColor(customDeclination2Color, Q_NULLPTR, q_("Select color for Custom Declination 2 line"));
	if (c.isValid())
	{
		customDeclination2Color=c;
		al->setLineColor(ArchaeoLine::CustomDeclination2, c);
		customDeclination2ColorPixmap.fill(c);
		ui->customDeclination2ColorToolButton->setIcon(QIcon(customDeclination2ColorPixmap));
	}
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
