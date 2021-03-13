/*
 * Calendars plug-in for Stellarium
 *
 * Copyright (C) 2020 Georg Zotti
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

#include "Calendars.hpp"
#include "CalendarsDialog.hpp"
#include "ui_calendarsDialog.h"

#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainView.hpp"

// We only need to include calendars when we have to call special functions.
#include "JulianCalendar.hpp"
#include "GregorianCalendar.hpp"
//#include "MayaHaabCalendar.hpp"
//#include "MayaTzolkinCalendar.hpp"
//#include "AztecXihuitlCalendar.hpp"
//#include "AztecTonalpohualliCalendar.hpp"


CalendarsDialog::CalendarsDialog()
	: StelDialog("Calendars")
	, cal(Q_NULLPTR)
{
	ui = new Ui_calendarsDialog();
}

CalendarsDialog::~CalendarsDialog()
{
	delete ui;          ui=Q_NULLPTR;
}

void CalendarsDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

void CalendarsDialog::createDialogContent()
{
	cal = GETSTELMODULE(Calendars);
	ui->setupUi(dialog);

	// Kinetic scrolling
	kineticScrollingList << ui->aboutTextBrowser;
	StelGui* gui= static_cast<StelGui*>(StelApp::getInstance().getGui());
	enableKineticScrolling(gui->getFlagUseKineticScrolling());
	connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(resetCalendarsSettings()));
	setAboutHtml();

	// DISABLE Chinese for now, TBD!
	ui->chineseCheckBox->hide();
	ui->tabs->removeTab(2);
	ui->newHinduLunarCheckBox->hide();
	ui->newHinduSolarCheckBox->hide();

	// MAKE SURE to connect each calendar's partsChanged to a respective populate... method here.
	connect(cal->getCal("Julian"),             SIGNAL(partsChanged(QVector<int>)), this, SLOT(populateJulianParts(QVector<int>)));
	connect(cal->getCal("Gregorian"),          SIGNAL(partsChanged(QVector<int>)), this, SLOT(populateGregorianParts(QVector<int>)));
	connect(cal->getCal("ISO"),                SIGNAL(partsChanged(QVector<int>)), this, SLOT(populateISOParts(QVector<int>)));
	connect(cal->getCal("MayaLongCount"),      SIGNAL(partsChanged(QVector<int>)), this, SLOT(populateMayaLongCountParts(QVector<int>)));
	connect(cal->getCal("MayaHaab"),           SIGNAL(partsChanged(QVector<int>)), this, SLOT(populateMayaHaabParts(QVector<int>)));
	connect(cal->getCal("MayaTzolkin"),        SIGNAL(partsChanged(QVector<int>)), this, SLOT(populateMayaTzolkinParts(QVector<int>)));
	connect(cal->getCal("AztecXihuitl"),       SIGNAL(partsChanged(QVector<int>)), this, SLOT(populateAztecXihuitlParts(QVector<int>)));
	connect(cal->getCal("AztecTonalpohualli"), SIGNAL(partsChanged(QVector<int>)), this, SLOT(populateAztecTonalpohualliParts(QVector<int>)));
	//connect(cal->getCal("Chinese"), SIGNAL(partsChanged(QVector<int>)), this, SLOT(populateChineseParts(QVector<int>)));

	connectBoolProperty(ui->julianCheckBox,             "Calendars.flagShowJulian");
	connectBoolProperty(ui->gregorianCheckBox,          "Calendars.flagShowGregorian");
	connectBoolProperty(ui->gregorianCheckBox,          "Calendars.flagShowGregorian");
	connectBoolProperty(ui->isoCheckBox,                "Calendars.flagShowISO");
	connectBoolProperty(ui->romanCheckBox,              "Calendars.flagShowRoman");
	connectBoolProperty(ui->olympicCheckBox,            "Calendars.flagShowOlympic");
	connectBoolProperty(ui->egyptianCheckBox,           "Calendars.flagShowEgyptian");
	connectBoolProperty(ui->armenianCheckBox,           "Calendars.flagShowArmenian");
	connectBoolProperty(ui->zoroastrianCheckBox,        "Calendars.flagShowZoroastrian");
	connectBoolProperty(ui->copticCheckBox,             "Calendars.flagShowCoptic");
	connectBoolProperty(ui->ethiopicCheckBox,           "Calendars.flagShowEthiopic");
	connectBoolProperty(ui->icelandicCheckBox,          "Calendars.flagShowIcelandic");
	connectBoolProperty(ui->chineseCheckBox,            "Calendars.flagShowChinese");
	connectBoolProperty(ui->islamicCheckBox,            "Calendars.flagShowIslamic");
	connectBoolProperty(ui->hebrewCheckBox,             "Calendars.flagShowHebrew");
	connectBoolProperty(ui->oldHinduSolarCheckBox,      "Calendars.flagShowOldHinduSolar");
	connectBoolProperty(ui->oldHinduLunarCheckBox,      "Calendars.flagShowOldHinduLunar");
	connectBoolProperty(ui->mayaLCCheckBox,             "Calendars.flagShowMayaLongCount");
	connectBoolProperty(ui->mayaHaabCheckBox,           "Calendars.flagShowMayaHaab");
	connectBoolProperty(ui->mayaTzolkinCheckBox,        "Calendars.flagShowMayaTzolkin");
	connectBoolProperty(ui->aztecXihuitlCheckBox,       "Calendars.flagShowAztecXihuitl");
	connectBoolProperty(ui->aztecTonalpohualliCheckBox, "Calendars.flagShowAztecTonalpohualli");
	connectBoolProperty(ui->balineseCheckBox,           "Calendars.flagShowBalinese");
	connectBoolProperty(ui->frenchArithmeticCheckBox,   "Calendars.flagShowFrenchArithmetic");
	connectBoolProperty(ui->persianArithmeticCheckBox,  "Calendars.flagShowPersianArithmetic");

	// MAKE SURE to connect all part edit elements respective ...Changed() method here.
	connect(ui->julianYearSpinBox,      SIGNAL(valueChanged(int)), this, SLOT(julianChanged()));
	connect(ui->julianMonthSpinBox,     SIGNAL(valueChanged(int)), this, SLOT(julianChanged()));
	connect(ui->julianDaySpinBox,       SIGNAL(valueChanged(int)), this, SLOT(julianChanged()));
	connect(ui->gregorianYearSpinBox,   SIGNAL(valueChanged(int)), this, SLOT(gregorianChanged()));
	connect(ui->gregorianMonthSpinBox,  SIGNAL(valueChanged(int)), this, SLOT(gregorianChanged()));
	connect(ui->gregorianDaySpinBox,    SIGNAL(valueChanged(int)), this, SLOT(gregorianChanged()));
	connect(ui->isoYearSpinBox,         SIGNAL(valueChanged(int)), this, SLOT(isoChanged()));
	connect(ui->isoWeekSpinBox,         SIGNAL(valueChanged(int)), this, SLOT(isoChanged()));
	connect(ui->isoDaySpinBox,          SIGNAL(valueChanged(int)), this, SLOT(isoChanged()));
	connect(ui->baktunSpinBox,          SIGNAL(valueChanged(int)), this, SLOT(mayaLongCountChanged()));
	connect(ui->katunSpinBox,           SIGNAL(valueChanged(int)), this, SLOT(mayaLongCountChanged()));
	connect(ui->tunSpinBox,             SIGNAL(valueChanged(int)), this, SLOT(mayaLongCountChanged()));
	connect(ui->uinalSpinBox,           SIGNAL(valueChanged(int)), this, SLOT(mayaLongCountChanged()));
	connect(ui->kinSpinBox,             SIGNAL(valueChanged(int)), this, SLOT(mayaLongCountChanged()));
	// TODO: Indirect handling of Haab/Tzolkin and Xihuitl/Tonalpohualli, with going back and forth to dates set in the GUI elements.
	//connect(ui->haabMonthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(mayaHaabChanged()));
	//connect(ui->haabDaySpinBox,   SIGNAL(valueChanged(int)), this, SLOT(mayaHaabChanged()));
	//connect(ui->tzolkinNumberSpinBox, SIGNAL(valueChanged(int)), this, SLOT(mayaTzolkinChanged()));
	//connect(ui->tzolkinNameSpinBox,   SIGNAL(valueChanged(int)), this, SLOT(mayaTzolkinChanged()));
	// In the first version, only switch full Tzolkin/Haab/Xihuitl/Tonalpohualli cycles. Later versions should allow configuring a date combination and trigger previous/next.
	StelCore *core=StelApp::getInstance().getCore();
	connect(ui->previousHaabButton,          &QPushButton::clicked, this, [=](){ core->addSolarDays(-365.);});
	connect(ui->nextHaabButton,              &QPushButton::clicked, this, [=](){ core->addSolarDays(365.);});
	connect(ui->previousTzolkinButton,       &QPushButton::clicked, this, [=](){ core->addSolarDays(-260.);});
	connect(ui->nextTzolkinButton,           &QPushButton::clicked, this, [=](){ core->addSolarDays(260.);});
	connect(ui->previousXihuitlButton,       &QPushButton::clicked, this, [=](){ core->addSolarDays(-365.);});
	connect(ui->nextXihuitlButton,           &QPushButton::clicked, this, [=](){ core->addSolarDays(365.);});
	connect(ui->previousTonalpohualliButton, &QPushButton::clicked, this, [=](){ core->addSolarDays(-260.);});
	connect(ui->nextTonalpohualliButton,     &QPushButton::clicked, this, [=](){ core->addSolarDays(260.);});
}

void CalendarsDialog::setAboutHtml(void)
{
	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Calendars Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + CALENDARS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + CALENDARS_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Georg Zotti</td></tr>";
	//html += "<tr><td><strong>" + q_("Contributors") + ":</strong></td><td> List with br separators </td></tr>";
	html += "</table>";

	html += "<p>" + q_("The Calendars plugin provides an interface to various calendars used around the world.") + "</p>";
	html += "<ul><li>" + q_("Julian Calendar") + "</li>";
	html += "<li>" + q_("Gregorian Calendar") + "</li>";
	html += "<li>" + q_("ISO Weeks") + "</li>";
	html += "<li>" + q_("Icelandic calendar") + "</li>";
	html += "<li>" + q_("Roman (Julian) calendar") + "</li>";
	html += "<li>" + q_("Olympiad calendar") + "</li>";
	html += "<li>" + q_("Egyptian calendar") + "</li>";
	html += "<li>" + q_("Armenian calendar") + "</li>";
	html += "<li>" + q_("Zoroastrian calendar") + "</li>";
	html += "<li>" + q_("Coptic calendar") + "</li>";
	html += "<li>" + q_("Ethiopic calendar") + "</li>";

	html += "<li>" + q_("Islamic Calendar (algorithmic)") + "</li>";
	html += "<li>" + q_("Hebrew Calendar") + "</li>";
	html += "<li>" + q_("French Revolution calendar (arithmetic version of 1795)") + "</li>";
	html += "<li>" + q_("Persian calendar (arithmetic version)") + "</li>";

//	html += "<li>" + q_("Chinese calendars") + "</li>";
//	html += "<li>" + q_("Tibetan calendars") + "</li>";
	html += "<li>" + q_("Old Hindu Solar and Lunar calendars") + "</li>";
//	html += "<li>" + q_("New Hindu Solar and Lunar calendars") + "</li>";
	html += "<li>" + q_("Maya calendars") + "</li>";
	html += "<li>" + q_("Aztec calendars") + "</li>";
	html += "<li>" + q_("Balinese Pawukon calendar") + "</li>";
	html += "</ul>";
	html += "<p>" + q_("The plugin is in an early stage of development. Please cross-check results and report errors.") + "</p>";
	html += "<p>" + q_("For some calendars, we welcome proper formatting suggestions by actual users.") + "</p>";

	html += "<h3>" + q_("Publications") + "</h3>";
	html += "<p>"  + q_("If you use this plugin in your publications, please cite:") + "</p>";
	html += "<p><ul>";
	html += "<li>" + QString("{Georg Zotti, Susanne Hoffmann, Alexander Wolf, Fabien Chéreau, Guillaume Chéreau: The simulated sky: Stellarium for cultural astronomy research.} Journal for Skyscape Archaeology, 6.2, 2020, pp. XX-XX.")
			.toHtmlEscaped().replace(a_rx, "<a href=\"https://equinoxpub.com/PATH-TO.pdf\">\\1</a>") + "</li>";
	html += "</ul></p>";

	html += "<h3>" + q_("References") + "</h3>";
	html += "<p>"  + q_("This plugin is based on:");
	html += "<ul>";
	html += "<li>" + QString("{Edward M. Reingold, Nachum Dershowitz: Calendrical Calculations.} The Ultimate Edition. Cambridge University Press 2018.")
			.toHtmlEscaped().replace(a_rx, "<a href=\"https://doi.org/10.1017/9781107057623\">\\1</a>") + "</li>";
	html += "</ul></p>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Github website.  Be sure to put \"%1\" in the subject when posting.")).arg("Calendars plugin");
	html += "<ul>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you have a question, you can {get an answer here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://groups.google.com/forum/#!forum/stellarium\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("Bug reports and feature requests can be made {here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/issues\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you want to read full information about this plugin and its history, see the Stellarium User Guide") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->aboutTextBrowser->setHtml(html);
}

void CalendarsDialog::resetCalendarsSettings()
{
	if (askConfirmation())
	{
		qDebug() << "[Calendars] restore defaults...";
		cal->restoreDefaultSettings();
	}
	else
		qDebug() << "[Calendars] restore defaults cancelled.";
}

void CalendarsDialog::populateJulianParts(QVector<int> parts)
{
	ui->julianYearSpinBox->setValue(parts.at(0));
	ui->julianMonthSpinBox->setValue(parts.at(1));
	ui->julianDaySpinBox->setValue(parts.at(2));

	// If the GUI wants to show other related data, we can find the sender. (nullptr when this slot is called directly!)
	JulianCalendar *jul=dynamic_cast<JulianCalendar*>(sender());
	if (jul)
		ui->julianWeekdayLineEdit->setText(jul->weekday(jul->getJD()));
}

void CalendarsDialog::populateGregorianParts(QVector<int> parts)
{
	ui->gregorianYearSpinBox->setValue(parts.at(0));
	ui->gregorianMonthSpinBox->setValue(parts.at(1));
	ui->gregorianDaySpinBox->setValue(parts.at(2));

	GregorianCalendar *greg=dynamic_cast<GregorianCalendar*>(sender());
	if (greg)
		ui->gregorianWeekdayLineEdit->setText(greg->weekday(greg->getJD()));
}

void CalendarsDialog::populateISOParts(QVector<int> parts)
{
	ui->isoYearSpinBox->setValue(parts.at(0));
	ui->isoWeekSpinBox->setValue(parts.at(1));
	ui->isoDaySpinBox->setValue(parts.at(2));
}

void CalendarsDialog::populateMayaLongCountParts(QVector<int> parts)
{
	ui->baktunSpinBox->setValue(parts.at(0));
	ui->katunSpinBox->setValue(parts.at(1));
	ui->tunSpinBox->setValue(parts.at(2));
	ui->uinalSpinBox->setValue(parts.at(3));
	ui->kinSpinBox->setValue(parts.at(4));
}

void CalendarsDialog::populateMayaHaabParts(QVector<int> parts)
{
	ui->haabMonthSpinBox->setValue(parts.at(0));
	ui->haabDaySpinBox->setValue(parts.at(1));
	Calendar *haab=dynamic_cast<Calendar*>(sender());
	if (haab)
		ui->haabLineEdit->setText(haab->getFormattedDateString());
}

void CalendarsDialog::populateMayaTzolkinParts(QVector<int> parts)
{
	ui->tzolkinNumberSpinBox->setValue(parts.at(0));
	ui->tzolkinNameSpinBox->setValue(parts.at(1));
	Calendar *tzolkin=dynamic_cast<Calendar*>(sender());
	if (tzolkin)
		ui->tzolkinLineEdit->setText(tzolkin->getFormattedDateString());
}

void CalendarsDialog::populateAztecXihuitlParts(QVector<int> parts)
{
	ui->xihuitlMonthSpinBox->setValue(parts.at(0));
	ui->xihuitlDaySpinBox->setValue(parts.at(1));
	Calendar *xihuitl=dynamic_cast<Calendar*>(sender());
	if (xihuitl)
		ui->xihuitlLineEdit->setText(xihuitl->getFormattedDateString());
}

void CalendarsDialog::populateAztecTonalpohualliParts(QVector<int> parts)
{
	ui->tonalpohualliNumberSpinBox->setValue(parts.at(0));
	ui->tonalpohualliNameSpinBox->setValue(parts.at(1));
	Calendar *tonalpohualli=dynamic_cast<Calendar*>(sender());
	if (tonalpohualli)
		ui->tonalpohualliLineEdit->setText(tonalpohualli->getFormattedDateString());
}

/*
 * These slots set the calendar from the associated GUI elements
 */

void CalendarsDialog::julianChanged()
{
	cal->getCal("Julian")->setDate({ui->julianYearSpinBox->value(),
					 ui->julianMonthSpinBox->value(),
					 ui->julianDaySpinBox->value()});
}

void CalendarsDialog::gregorianChanged()
{
	cal->getCal("Gregorian")->setDate({ui->gregorianYearSpinBox->value(),
					    ui->gregorianMonthSpinBox->value(),
					    ui->gregorianDaySpinBox->value()});
}

void CalendarsDialog::isoChanged()
{
	// TODO proper handling of ISO weekday combo box.
	cal->getCal("ISO")->setDate({ui->isoYearSpinBox->value(),
					ui->isoWeekSpinBox->value(),
					ui->isoDaySpinBox->value()});
}

void CalendarsDialog::mayaLongCountChanged()
{
	cal->getCal("MayaLongCount")->setDate({ui->baktunSpinBox->value(),
					       ui->katunSpinBox->value(),
					       ui->tunSpinBox->value(),
					       ui->uinalSpinBox->value(),
					       ui->kinSpinBox->value()});
}

void CalendarsDialog::mayaHaabChanged()
{
	cal->getCal("MayaHaab")->setDate({ui->haabMonthSpinBox->value(),
					   ui->haabDaySpinBox->value()});
}

void CalendarsDialog::mayaTzolkinChanged()
{
	cal->getCal("MayaTzolkin")->setDate({ui->tzolkinNumberSpinBox->value(),
					      ui->tzolkinNameSpinBox->value()});
}

void CalendarsDialog::aztecXihuitlChanged()
{
	cal->getCal("AztecXihuitl")->setDate({ui->xihuitlMonthSpinBox->value(),
					   ui->xihuitlDaySpinBox->value()});
}

void CalendarsDialog::aztecTonalpohualliChanged()
{
	cal->getCal("AztecTonalpohualli")->setDate({ui->tonalpohualliNumberSpinBox->value(),
					      ui->tonalpohualliNameSpinBox->value()});
}
