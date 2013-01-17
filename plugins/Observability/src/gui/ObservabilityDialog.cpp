/*
 * Stellarium Observability Plug-in GUI
 *
 * Copyright (C) 2012 Ivan Marti-Vidal (based on code by Alexander Wolf)
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
#include <QFileDialog>

#include "StelApp.hpp"
#include "ui_ObservabilityDialog.h"
#include "ObservabilityDialog.hpp"
#include "Observability.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelStyle.hpp"
#include "StelGui.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelFileMgr.hpp"
#include "StelTranslator.hpp"

ObservabilityDialog::ObservabilityDialog()
{
        ui = new Ui_ObservabilityDialog;
}

ObservabilityDialog::~ObservabilityDialog()
{
	delete ui;
}

void ObservabilityDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

// Initialize the dialog widgets and connect the signals/slots
void ObservabilityDialog::createDialogContent()
{
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));

	// Settings:
	connect(ui->Today, SIGNAL(stateChanged(int)), this, SLOT(setTodayFlag(int)));
	connect(ui->AcroCos, SIGNAL(stateChanged(int)), this, SLOT(setAcroCosFlag(int)));
	connect(ui->Opposition, SIGNAL(stateChanged(int)), this, SLOT(setOppositionFlag(int)));
	connect(ui->Goods, SIGNAL(stateChanged(int)), this, SLOT(setGoodDatesFlag(int)));
	connect(ui->FullMoon, SIGNAL(stateChanged(int)), this, SLOT(setFullMoonFlag(int)));
//	connect(ui->Crescent, SIGNAL(stateChanged(int)), this, SLOT(setCrescentMoonFlag(int)));
//	connect(ui->SuperMoon, SIGNAL(stateChanged(int)), this, SLOT(setSuperMoonFlag(int)));

	connect(ui->Red, SIGNAL(sliderMoved(int)), this, SLOT(setRed(int)));
	connect(ui->Green, SIGNAL(sliderMoved(int)), this, SLOT(setGreen(int)));
	connect(ui->Blue, SIGNAL(sliderMoved(int)), this, SLOT(setBlue(int)));
	connect(ui->fontSize, SIGNAL(sliderMoved(int)), this, SLOT(setSize(int)));
	connect(ui->SunAltitude, SIGNAL(sliderMoved(int)), this, SLOT(setAltitude(int)));
	connect(ui->HorizAltitude, SIGNAL(sliderMoved(int)), this, SLOT(setHorizon(int)));

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

void ObservabilityDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";

	html += "<h2>" + q_("Observability Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + OBSERVABILITY_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Ivan Marti-Vidal &lt;i.martividal@gmail.com&gt;</td></tr></table>";

	html += "<p>" + q_("Plugin that analyzes the observability of the selected source (or the screen center, if no source is selected). The plugin can show rise, transit, and set times, as well as the best epoch of the year (i.e., largest angular separation from the Sun), the date range when the source is above the horizon at dark night, and the dates of Acronychal and Cosmical rise/set.<br>Ephemeris of the Solar-System objects and parallax effects are taken into account.<br><br> The author thanks Alexander Wolf and Georg Zotti for their advice.<br><br>Ivan Marti-Vidal (Onsala Space Observatory)") + "</p>";

	html += "<h3>" + q_("Explanation of some parameters") + "</h3><table width=\"90%\">";
	html += QString("<tr><td>%1</td><td>%2</td></tr>").arg(q_("Sun altitude at twilight:")).arg(q_("Any celestial object will be considered visible when the Sun is below this altitude. The altitude at astronomical twilight ranges usually between -12 and -18 degrees. This parameter is only used for the estimate of the range of observable epochs (see below)."));

	html += QString("<tr><td>%1</td><td>%2</td></tr>").arg(q_("Horizon altitude:")).arg(q_("Minimum observable altitude (due to mountains, buildings, or just a limited telescope mount)."));

	html += QString("<tr><td>%1</td><td>%2</td></tr>").arg(q_("Today ephemeris:")).arg(q_("Self-explanatory. The program will show the rise, set, and culmination (transit) times. The exact times for these ephemeris are given in two ways: as time spans (referred to the current time) and as clock hours (in local time)."));
	html += QString("<tr><td>%1</td><td>%2</td></tr>").arg(q_("Acronychal/Cosmical rise/set:")).arg(q_("The Acronychal rise (or set) of an object happens when the object rises (or sets) just when the Sun sets (or rises), respectively. The exact dates of these ephemeris depend on the Observer's location. The dates between the Acronychal set and rise are those when the altitude of the celestial object uses to be high when the Sun is well below the horizon (hence the object can be well observed). On the contrary, the Cosmical rise (or set) happens when both, the object and the Sun, rise (or set) simultaneously. It is obvious that the source is hardly observable (or not observable at all) in the dates between Cosmical set and rise."));
	html += QString("<tr><td>%1</td><td>%2</td></tr>").arg(q_("Largest Sun separation:")).arg(q_("Happens when the angular separation between the Sun and the celestial object are maximum. In most cases, this is equivalent to say that the Equatorial longitudes of the Sun and the object differ by 180 degrees, so the Sun is in opposition to the object. When an object is at its maximum possible angular separation from the Sun (no matter if it is a planet or a star), it culminates roughly at midnight, and on the darkest possible area of the Sky at that declination. Hence, that is the 'best' night to observe a particular object."));
	html += QString("<tr><td>%1</td><td>%2</td></tr>").arg(q_("Nights with source above horizon:")).arg(q_("The program computes the range of dates when the celestial object is above the horizon at least during one moment of the night. By 'night', the program considers the time span when the Sun altitude is below that of the twilight (which can be set by the user; see above). When the objects are fixed on the sky (or are exterior planets), the range of observable epochs for the current year can have two possible forms: either a range from one date to another (e.g., 20 Jan to 15 Sep) or in two steps (from 1 Jan to a given date and from another date to 31 Dec). In the first case, the first date (20 Jan in our example) shall be close to the so-called 'Heliacal rise of a star' and the second date (15 Sep in our example) shall be close to the 'Heliacal set'. In the second case (e.g., a range in the form 1 Jan to 20 May and 21 Sep to 31 Dec), the first date (20 May in our example) would be close to the Heliacal set and the second one (21 Sep in our example) to the Heliacal rise. More exact equations to estimate the Heliacal rise/set of stars and planets (which will not depend on the mere input of a twilight Sun elevation by the user) will be implemented in future versions of this plugin."));
	html += QString("<tr><td>%1</td><td>%2</td></tr>").arg(q_("Full Moon:")).arg(q_("When the Moon is selected, the program can compute the exact closest dates of the Moon's opposition to the Sun."));
	html += "</table>";
	html += "</body></html>";

	ui->aboutTextBrowser->setHtml(html);
}


void ObservabilityDialog::restoreDefaults(void)
{
	qDebug() << "Observability::restoreDefaults";
	GETSTELMODULE(Observability)->restoreDefaults();
	GETSTELMODULE(Observability)->readSettingsFromConfig();
	updateGuiFromSettings();
}

void ObservabilityDialog::updateGuiFromSettings(void)
{
	ui->Today->setChecked(GETSTELMODULE(Observability)->getShowFlags(1));
	ui->AcroCos->setChecked(GETSTELMODULE(Observability)->getShowFlags(2));
	ui->Goods->setChecked(GETSTELMODULE(Observability)->getShowFlags(3));
	ui->Opposition->setChecked(GETSTELMODULE(Observability)->getShowFlags(4));
	ui->FullMoon->setChecked(GETSTELMODULE(Observability)->getShowFlags(5));
//	ui->Crescent->setChecked(GETSTELMODULE(Observability)->getShowFlags(6));
//	ui->SuperMoon->setChecked(GETSTELMODULE(Observability)->getShowFlags(7));

	Vec3f currFont = GETSTELMODULE(Observability)->getFontColor();
	int Rv = (int)(100.*currFont[0]);
	int Gv = (int)(100.*currFont[1]);
	int Bv = (int)(100.*currFont[2]);
	ui->Red->setValue(Rv);
	ui->Green->setValue(Gv);
	ui->Blue->setValue(Bv);
	ui->fontSize->setValue(GETSTELMODULE(Observability)->getFontSize());
	int SAlti = GETSTELMODULE(Observability)->getSunAltitude();
	ui->SunAltitude->setValue(SAlti);
	ui->AltiText->setText(QString("%1 -%2 %3").arg(q_("Sun altitude at twilight:")).arg(SAlti).arg(q_("deg.")));

	SAlti = GETSTELMODULE(Observability)->getHorizAltitude();
	ui->HorizAltitude->setValue(SAlti);
	ui->HorizText->setText(QString("%1 %2 %3").arg(q_("Horizon altitude:")).arg(SAlti).arg(q_("deg.")));

}

void ObservabilityDialog::saveSettings(void)
{
	GETSTELMODULE(Observability)->saveSettingsToConfig();
}

void ObservabilityDialog::setTodayFlag(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Observability)->setShow(1,b);
}

void ObservabilityDialog::setAcroCosFlag(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Observability)->setShow(2,b);
}

void ObservabilityDialog::setGoodDatesFlag(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Observability)->setShow(3,b);
}

void ObservabilityDialog::setOppositionFlag(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Observability)->setShow(4,b);
}

void ObservabilityDialog::setFullMoonFlag(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Observability)->setShow(5,b);
}

//void ObservabilityDialog::setCrescentMoonFlag(int checkState)
//{
//	bool b = checkState != Qt::Unchecked;
//	GETSTELMODULE(Observability)->setShow(6,b);
//}

//void ObservabilityDialog::setSuperMoonFlag(int checkState)
//{
//	bool b = checkState != Qt::Unchecked;
//	GETSTELMODULE(Observability)->setShow(7,b);
//}

void ObservabilityDialog::setRed(int Value)
{
	GETSTELMODULE(Observability)->setFontColor(0,Value);
}

void ObservabilityDialog::setGreen(int Value)
{
	GETSTELMODULE(Observability)->setFontColor(1,Value);
}

void ObservabilityDialog::setBlue(int Value)
{
	GETSTELMODULE(Observability)->setFontColor(2,Value);
}

void ObservabilityDialog::setSize(int Value)
{
	GETSTELMODULE(Observability)->setFontSize(Value);
}

void ObservabilityDialog::setAltitude(int Value)
{
	ui->AltiText->setText(QString("%1 -%2 %3").arg(q_("Sun altitude at twilight:")).arg(Value).arg(q_("deg.")));
	GETSTELMODULE(Observability)->setSunAltitude(Value);
}


void ObservabilityDialog::setHorizon(int Value)
{
	ui->HorizText->setText(QString("%1 %2 %3").arg(q_("Horizon altitude:")).arg(Value).arg(q_("deg.")));
	GETSTELMODULE(Observability)->setHorizAltitude(Value);
}


