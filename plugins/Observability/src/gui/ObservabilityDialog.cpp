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
#include "StelCore.hpp"
#include "ui_ObservabilityDialog.h"
#include "ObservabilityDialog.hpp"
#include "Observability.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelStyle.hpp"
#include "StelGui.hpp"
#include "StelMainView.hpp"
#include "StelFileMgr.hpp"
#include "StelTranslator.hpp"

ObservabilityDialog::ObservabilityDialog() : StelDialog("Observability")
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
		updateControls(); // Also re-translate the dynamic slider labels
	}
}

// Initialize the dialog widgets and connect the signals/slots
void ObservabilityDialog::createDialogContent()
{
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);
	connect(&StelApp::getInstance(),
	        SIGNAL(languageChanged()), this, SLOT(retranslate()));

	Observability* plugin = GETSTELMODULE(Observability);

	// Kinetic scrolling
	kineticScrollingList << ui->aboutTextBrowser;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

	// Settings:
	
	// clicked() is called only when the user makes an input,
	// so we avoid an endless loop when setting the value in updateControls().
	connect(ui->todayCheckBox, SIGNAL(clicked(bool)),
	        plugin, SLOT(enableTodayField(bool)));
	connect(ui->acroCosCheckBox, SIGNAL(clicked(bool)),
	        plugin, SLOT(enableAcroCosField(bool)));
	connect(ui->oppositionCheckBox, SIGNAL(clicked(bool)),
	        plugin, SLOT(enableOppositionField(bool)));
	connect(ui->goodNightsCheckBox, SIGNAL(clicked(bool)),
	        plugin, SLOT(enableGoodNightsField(bool)));
	connect(ui->fullMoonCheckBox, SIGNAL(clicked(bool)),
	        plugin, SLOT(enableFullMoonField(bool)));

	connect(ui->redSlider, SIGNAL(sliderMoved(int)),
	        this, SLOT(setColor()));
	connect(ui->greenSlider, SIGNAL(sliderMoved(int)),
	        this, SLOT(setColor()));
	connect(ui->blueSlider, SIGNAL(sliderMoved(int)),
	        this, SLOT(setColor()));
	
	// Isn't valueChanged() better? But then we'll have to block
	// signlas when settting the slider values.
	connect(ui->fontSize, SIGNAL(sliderMoved(int)),
	        plugin, SLOT(setFontSize(int)));
	connect(ui->sunAltitudeSlider, SIGNAL(sliderMoved(int)),
	        plugin, SLOT(setTwilightAltitude(int)));
	connect(ui->sunAltitudeSlider, SIGNAL(sliderMoved(int)),
	        this, SLOT(updateAltitudeLabel(int)));
	connect(ui->horizonAltitudeSlider, SIGNAL(sliderMoved(int)),
	        plugin, SLOT(setHorizonAltitude(int)));
	connect(ui->horizonAltitudeSlider, SIGNAL(sliderMoved(int)),
	        this, SLOT(updateHorizonLabel(int)));

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()),
		this, SLOT(restoreDefaults()));
	// TODO: The plug-in should emit a signal when settings are changed.
	// This works, because slots are called in the order they were connected.
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()),
	        this, SLOT(updateControls()));
	connect(ui->saveSettingsButton, SIGNAL(clicked()),
	        plugin, SLOT(saveConfiguration()));

	// About tab
	setAboutHtml();
	if(gui!=Q_NULLPTR)
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));

	updateControls();
}

void ObservabilityDialog::restoreDefaults()
{
	if (askConfirmation())
	{
		qDebug() << "[Observability] restore defaults...";
		GETSTELMODULE(Observability)->resetConfiguration();
	}
	else
		qDebug() << "[Observability] restore defaults is canceled...";
}

void ObservabilityDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";

	html += "<h2>" + q_("Observability Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + OBSERVABILITY_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + OBSERVABILITY_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Ivan Marti-Vidal &lt;i.martividal@gmail.com&gt;</td></tr></table>";

	html += "<p>" + q_("Plugin that analyzes the observability of the selected source (or the screen center, if no source is selected). The plugin can show rise, transit, and set times, as well as the best epoch of the year (i.e., largest angular separation from the Sun), the date range when the source is above the horizon at dark night, and the dates of Acronychal and Cosmical rise/set.<br>Ephemeris of the Solar-System objects and parallax effects are taken into account.<br><br> The author thanks Alexander Wolf and Georg Zotti for their advice.<br><br>Ivan Marti-Vidal (Onsala Space Observatory)") + "</p>";

	html += "<h3>" + q_("Explanation of some parameters") + "</h3><table width=\"90%\">";
	html += QString("<tr><td>%1</td><td>%2</td></tr>").arg(q_("Sun altitude at twilight:")).arg(q_("Any celestial object will be considered visible when the Sun is below this altitude. The altitude at astronomical twilight ranges usually between -12 and -18 degrees. This parameter is only used for the estimate of the range of observable epochs (see below)."));

	html += QString("<tr><td>%1</td><td>%2</td></tr>").arg(q_("Horizon altitude:")).arg(q_("Minimum observable altitude (due to mountains, buildings, or just a limited telescope mount)."));

	html += QString("<tr><td>%1</td><td>%2</td></tr>").arg(q_("Today ephemeris:")).arg(q_("Self-explanatory. The program will show the rise, set, and culmination (transit) times. The exact times for these ephemeris are given in two ways: as time spans (referred to the current time) and as clock hours (in local time)."));
	html += QString("<tr><td>%1</td><td>%2</td></tr>").arg(q_("Acronychal/Cosmical/Heliacal rise/set:")).arg(q_("The days of Cosmical rise/set of an object are estimated as the days when the object rises (or sets) together with the rise/set of the Sun. The exact dates of these ephemeris depend on the Observer's location.  On the contrary, the Acronycal rise (or set) happens when the star rises/sets with the setting/rising of the Sun (i.e., opposite to the Sun). On the one hand, it is obvious that the source is hardly observable (or not observable at all) in the dates between Cosmical set and Cosmical rise. On the other hand, the dates around the Acronychal set and rise are those when the altitude of the celestial object uses to be high when the Sun is well below the horizon (hence the object can be well observed). The date of Heliacal rise is the first day of the year when a star becomes visible. It happens when the star is close to the eastern horizon roughly before the end of the astronomical night (i.e., at the astronomical twilight). In the following nights, the star will be visibile during longer periods of time, until it reaches its Heliacal set (i.e., the last night of the year when the star is still visible). At the Heliacal set, the star sets roughly after the beginning of the astronomical night."));
	html += QString("<tr><td>%1</td><td>%2</td></tr>").arg(q_("Largest Sun separation:")).arg(q_("Happens when the angular separation between the Sun and the celestial object are maximum. In most cases, this is equivalent to say that the Equatorial longitudes of the Sun and the object differ by 180 degrees, so the Sun is in opposition to the object. When an object is at its maximum possible angular separation from the Sun (no matter if it is a planet or a star), it culminates roughly at midnight, and on the darkest possible area of the Sky at that declination. Hence, that is the 'best' night to observe a particular object."));
	html += QString("<tr><td>%1</td><td>%2</td></tr>").arg(q_("Nights with source above horizon:")).arg(q_("The program computes the range of dates when the celestial object is above the horizon at least during one moment of the night. By 'night', the program considers the time span when the Sun altitude is below that of the twilight (which can be set by the user; see above). When the objects are fixed on the sky (or are exterior planets), the range of observable epochs for the current year can have two possible forms: either a range from one date to another (e.g., 20 Jan to 15 Sep) or in two steps (from 1 Jan to a given date and from another date to 31 Dec). In the first case, the first date (20 Jan in our example) shall be close to the so-called 'Heliacal rise of a star' and the second date (15 Sep in our example) shall be close to the 'Heliacal set'. In the second case (e.g., a range in the form 1 Jan to 20 May and 21 Sep to 31 Dec), the first date (20 May in our example) would be close to the Heliacal set and the second one (21 Sep in our example) to the Heliacal rise. More exact equations to estimate the Heliacal rise/set of stars and planets (which will not depend on the mere input of a twilight Sun elevation by the user) will be implemented in future versions of this plugin."));
	html += QString("<tr><td>%1</td><td>%2</td></tr>").arg(q_("Full Moon:")).arg(q_("When the Moon is selected, the program can compute the exact closest dates of the Moon's opposition to the Sun."));
	html += "</table>";
	html += "</body></html>";

	ui->aboutTextBrowser->setHtml(html);
}

void ObservabilityDialog::updateControls()
{
	Observability* plugin = GETSTELMODULE(Observability);
	
	ui->todayCheckBox->setChecked(plugin->getShowFlags(1));
	ui->acroCosCheckBox->setChecked(plugin->getShowFlags(2));
	ui->goodNightsCheckBox->setChecked(plugin->getShowFlags(3));
	ui->oppositionCheckBox->setChecked(plugin->getShowFlags(4));
	ui->fullMoonCheckBox->setChecked(plugin->getShowFlags(5));
//	ui->Crescent->setChecked(GETSTELMODULE(Observability)->getShowFlags(6));
//	ui->SuperMoon->setChecked(GETSTELMODULE(Observability)->getShowFlags(7));

	Vec3f fontColor = plugin->getFontColor();
	int red = static_cast<int>(100.*fontColor[0]);
	int green = static_cast<int>(100.*fontColor[1]);
	int blue = static_cast<int>(100.*fontColor[2]);
	ui->redSlider->setValue(red);
	ui->greenSlider->setValue(green);
	ui->blueSlider->setValue(blue);

	ui->fontSize->setValue(plugin->getFontSize());
	int sunAltitude = plugin->getTwilightAltitude();
	ui->sunAltitudeSlider->setValue(sunAltitude);
	updateAltitudeLabel(sunAltitude);
	int horizonAltitude = plugin->getHorizonAltitude();
	ui->horizonAltitudeSlider->setValue(horizonAltitude);
	updateHorizonLabel(horizonAltitude);
}

void ObservabilityDialog::setColor()
{
	int red = ui->redSlider->value();
	int green = ui->greenSlider->value();
	int blue = ui->blueSlider->value();

	float fRed = (float)(red) / 100.;
	float fGreen = (float)(green) / 100.;
	float fBlue = (float)(blue) / 100.;
	
	Vec3f color(fRed, fGreen, fBlue);
	GETSTELMODULE(Observability)->setFontColor(color);
}

void ObservabilityDialog::updateAltitudeLabel(int altitude)
{
	// This allows translators to use their own conventions for degree signs.
	ui->sunAltitudeLabel->setText(q_("Sun altitude at twilight: %1 deg.").arg(altitude));
}

void ObservabilityDialog::updateHorizonLabel(int horizon)
{
	// This allows translators to use their own conventions for degree signs.
	ui->horizonAltitudeLabel->setText(q_("Horizon altitude: %1 deg.").arg(horizon));
}


