/*
 * Stellarium
 * Copyright (C) 2023 Alexander Wolf
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

#include "AstroCalcAlmanacWidget.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelUtils.hpp"
#include "SpecificTimeMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "SolarSystem.hpp"

#include <QPushButton>
#include <QToolTip>
#include <QSettings>

AstroCalcAlmanacWidget::AstroCalcAlmanacWidget(QWidget* parent)
	: QWidget(parent)
	, sunriseJD(0.)
	, sunsetJD(0.)
	, moonriseJD(0.)
	, moonsetJD(0.)
	, civilDawnJD(0.)
	, civilDuskJD(0.)
	, nauticalDawnJD(0.)
	, nauticalDuskJD(0.)
	, astronomicalDawnJD(0.)
	, astronomicalDuskJD(0.)
	, ui(new Ui_astroCalcAlmanacWidget)
{
}

void AstroCalcAlmanacWidget::setup()
{
	ui->setupUi(this);
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &AstroCalcAlmanacWidget::retranslate);

	core = StelApp::getInstance().getCore();
	specMgr = GETSTELMODULE(SpecificTimeMgr);
	localeMgr = &StelApp::getInstance().getLocaleMgr();

	populateData();

	connect(core, &StelCore::locationChanged, this, [=](const StelLocation&){ populateData(); });
	connect(core, &StelCore::currentTimeZoneChanged, this, [=](const QString&){ setSeasonTimes(); setTodayTimes(); });
	// update the data when "Now" button is pressed or date is changed
	connect(core, &StelCore::dateChanged, this, [=](){ setSeasonTimes(); setTodayTimes(); });
	connect(specMgr, SIGNAL(eventYearChanged()), this, SLOT(setSeasonTimes()));
	connect(specMgr, SIGNAL(eventYearChanged()), this, SLOT(setTodayTimes()));

	connect(ui->buttonMarchEquinoxCurrent, &QPushButton::clicked, this, [=](){specMgr->currentMarchEquinox();});
	connect(ui->buttonMarchEquinoxNext, &QPushButton::clicked, this, [=](){specMgr->nextMarchEquinox();});
	connect(ui->buttonMarchEquinoxPrevious, &QPushButton::clicked, this, [=](){specMgr->previousMarchEquinox();});

	connect(ui->buttonSeptemberEquinoxCurrent, &QPushButton::clicked, this, [=](){specMgr->currentSeptemberEquinox();});
	connect(ui->buttonSeptemberEquinoxNext, &QPushButton::clicked, this, [=](){specMgr->nextSeptemberEquinox();});
	connect(ui->buttonSeptemberEquinoxPrevious, &QPushButton::clicked, this, [=](){specMgr->previousSeptemberEquinox();});

	connect(ui->buttonJuneSolsticeCurrent, &QPushButton::clicked, this, [=](){specMgr->currentJuneSolstice();});
	connect(ui->buttonJuneSolsticeNext, &QPushButton::clicked, this, [=](){specMgr->nextJuneSolstice();});
	connect(ui->buttonJuneSolsticePrevious, &QPushButton::clicked, this, [=](){specMgr->previousJuneSolstice();});

	connect(ui->buttonDecemberSolsticeCurrent, &QPushButton::clicked, this, [=](){specMgr->currentDecemberSolstice();});
	connect(ui->buttonDecemberSolsticeNext, &QPushButton::clicked, this, [=](){specMgr->nextDecemberSolstice();});
	connect(ui->buttonDecemberSolsticePrevious, &QPushButton::clicked, this, [=](){specMgr->previousDecemberSolstice();});

	// handling special days
	connect(ui->buttonMarchEquinoxCurrent, &QPushButton::clicked, this, [=](){setTodayTimes();});
	connect(ui->buttonSeptemberEquinoxCurrent, &QPushButton::clicked, this, [=](){setTodayTimes();});
	connect(ui->buttonJuneSolsticeCurrent, &QPushButton::clicked, this, [=](){setTodayTimes();});
	connect(ui->buttonDecemberSolsticeCurrent, &QPushButton::clicked, this, [=](){setTodayTimes();});

	// handling special times
	connect(ui->buttonSunrise, &QPushButton::clicked, this, [=](){core->setJD(sunriseJD);});
	connect(ui->buttonSunset, &QPushButton::clicked, this, [=](){core->setJD(sunsetJD);});
	connect(ui->buttonMoonrise, &QPushButton::clicked, this, [=](){core->setJD(moonriseJD);});
	connect(ui->buttonMoonset, &QPushButton::clicked, this, [=](){core->setJD(moonsetJD);});
	connect(ui->buttonCivilDawn, &QPushButton::clicked, this, [=](){core->setJD(civilDawnJD);});
	connect(ui->buttonCivilDusk, &QPushButton::clicked, this, [=](){core->setJD(civilDuskJD);});
	connect(ui->buttonNauticalDawn, &QPushButton::clicked, this, [=](){core->setJD(nauticalDawnJD);});
	connect(ui->buttonNauticalDusk, &QPushButton::clicked, this, [=](){core->setJD(nauticalDuskJD);});
	connect(ui->buttonAstronomicalDawn, &QPushButton::clicked, this, [=](){core->setJD(astronomicalDawnJD);});
	connect(ui->buttonAstronomicalDusk, &QPushButton::clicked, this, [=](){core->setJD(astronomicalDuskJD);});

	QSize button = QSize(24, 24);
	ui->buttonMarchEquinoxCurrent->setFixedSize(button);
	ui->buttonMarchEquinoxNext->setFixedSize(button);
	ui->buttonMarchEquinoxPrevious->setFixedSize(button);
	ui->buttonJuneSolsticeCurrent->setFixedSize(button);
	ui->buttonJuneSolsticeNext->setFixedSize(button);
	ui->buttonJuneSolsticePrevious->setFixedSize(button);
	ui->buttonSeptemberEquinoxCurrent->setFixedSize(button);
	ui->buttonSeptemberEquinoxNext->setFixedSize(button);
	ui->buttonSeptemberEquinoxPrevious->setFixedSize(button);
	ui->buttonDecemberSolsticeCurrent->setFixedSize(button);
	ui->buttonDecemberSolsticeNext->setFixedSize(button);
	ui->buttonDecemberSolsticePrevious->setFixedSize(button);
	ui->buttonCivilDawn->setFixedSize(button);
	ui->buttonCivilDusk->setFixedSize(button);
	ui->buttonNauticalDawn->setFixedSize(button);
	ui->buttonNauticalDusk->setFixedSize(button);
	ui->buttonAstronomicalDawn->setFixedSize(button);
	ui->buttonAstronomicalDusk->setFixedSize(button);
	ui->buttonSunrise->setFixedSize(button);
	ui->buttonSunset->setFixedSize(button);
	ui->buttonMoonrise->setFixedSize(button);
	ui->buttonMoonset->setFixedSize(button);
}

void AstroCalcAlmanacWidget::retranslate()
{
	ui->retranslateUi(this);
	populateData();
}

void AstroCalcAlmanacWidget::populateData()
{
	// Set season labels
	setSeasonLabels();
	// Compute equinoxes/solstices and fill the time
	setSeasonTimes();
	// Compute today events
	setTodayTimes();
}

void AstroCalcAlmanacWidget::setSeasonLabels()
{
	const float latitide = StelApp::getInstance().getCore()->getCurrentLocation().getLatitude();
	if (latitide >= 0.f)
	{
		// Northern Hemisphere
		// TRANSLATORS: The name of season
		ui->labelMarchEquinox->setText(qc_("Spring","season"));
		// TRANSLATORS: The name of season
		ui->labelJuneSolstice->setText(qc_("Summer","season"));
		// TRANSLATORS: The name of season
		ui->labelSeptemberEquinox->setText(qc_("Fall","season"));
		// TRANSLATORS: The name of season
		ui->labelDecemberSolstice->setText(qc_("Winter","season"));
	}
	else
	{
		// Southern Hemisphere
		// TRANSLATORS: The name of season
		ui->labelMarchEquinox->setText(qc_("Fall","season"));
		// TRANSLATORS: The name of season
		ui->labelJuneSolstice->setText(qc_("Winter","season"));
		// TRANSLATORS: The name of season
		ui->labelSeptemberEquinox->setText(qc_("Spring","season"));
		// TRANSLATORS: The name of season
		ui->labelDecemberSolstice->setText(qc_("Summer","season"));
	}
}

QString AstroCalcAlmanacWidget::getFormattedDateTime(const double JD, const double utcShift)
{
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	return QString("%1 %2 %3").arg(QString::number(day), localeMgr->longGenitiveMonthName(month), StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(JD+utcShift), true));
}

void AstroCalcAlmanacWidget::setSeasonTimes()
{
	const double utcShift = core->getUTCOffset(core->getJD()) / 24.; // Fix DST shift...
	const double JD = core->getJD() + utcShift;
	int year, month, day;
	double jdFirstDay, jdLastDay;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	StelUtils::getJDFromDate(&jdFirstDay, year, 1, 1, 0, 0, 0);
	StelUtils::getJDFromDate(&jdLastDay, year, 12, 31, 24, 0, 0);
	const double marchEquinox = specMgr->getEquinox(year, SpecificTimeMgr::Equinox::March);
	const double septemberEquinox = specMgr->getEquinox(year, SpecificTimeMgr::Equinox::September);
	const double juneSolstice = specMgr->getSolstice(year, SpecificTimeMgr::Solstice::June);
	const double decemberSolstice = specMgr->getSolstice(year, SpecificTimeMgr::Solstice::December);
	QString days = qc_("days", "duration");	
	int jdDepth = 5;
	int daysDepth = 2;

	// Current year
	ui->labelCurrentYear->setText(QString::number(year));
	ui->labelYearDuration->setText(QString("(%1 %2)").arg(QString::number(jdLastDay-jdFirstDay), days));
	// Spring/Fall
	ui->labelMarchEquinoxJD->setText(QString::number(marchEquinox, 'f', jdDepth));
	ui->labelMarchEquinoxLT->setText(getFormattedDateTime(marchEquinox, utcShift));
	ui->labelMarchEquinoxDuration->setText(QString("%1 %2").arg(QString::number(juneSolstice-marchEquinox, 'f', daysDepth), days));
	// Summer/Winter
	ui->labelJuneSolsticeJD->setText(QString::number(juneSolstice, 'f', jdDepth));
	ui->labelJuneSolsticeLT->setText(getFormattedDateTime(juneSolstice, utcShift));
	ui->labelJuneSolsticeDuration->setText(QString("%1 %2").arg(QString::number(septemberEquinox-juneSolstice, 'f', daysDepth), days));
	// Fall/Spring
	ui->labelSeptemberEquinoxJD->setText(QString::number(septemberEquinox, 'f', jdDepth));
	ui->labelSeptemberEquinoxLT->setText(getFormattedDateTime(septemberEquinox, utcShift));
	ui->labelSeptemberEquinoxDuration->setText(QString("%1 %2").arg(QString::number(decemberSolstice-septemberEquinox, 'f', daysDepth), days));
	// Winter/Summer
	ui->labelDecemberSolsticeJD->setText(QString::number(decemberSolstice, 'f', jdDepth));
	ui->labelDecemberSolsticeLT->setText(getFormattedDateTime(decemberSolstice, utcShift));
	const double duration = (marchEquinox-jdFirstDay) + (jdLastDay-decemberSolstice);
	ui->labelDecemberSolsticeDuration->setText(QString("%1 %2").arg(QString::number(duration, 'f', daysDepth), days));
}

void AstroCalcAlmanacWidget::setTodayTimes()
{
	if (core->getCurrentPlanet()!=GETSTELMODULE(SolarSystem)->getEarth())
		return;

	const double JD = core->getJD();
	const double utcOffsetHrs = core->getUTCOffset(JD);
	const double utcShift = utcOffsetHrs / 24.; // Fix DST shift...
	PlanetP sun = GETSTELMODULE(SolarSystem)->getSun();
	double duration, duration1, duration2;
	bool astronomicalTwilightBtn, nauticalTwilightBtn, civilTwilightBtn, sunBtn;
	QString moonrise, moonset, sunrise, sunset, civilTwilightBegin, civilTwilightEnd, nauticalTwilightBegin,
		nauticalTwilightEnd, astronomicalTwilightBegin, astronomicalTwilightEnd, dayDuration, nightDuration,
		civilTwilightDuration, nauticalTwilightDuration, astronomicalTwilightDuration, dash = QChar(0x2014);

	// Moon
	Vec4d moon = GETSTELMODULE(SolarSystem)->getMoon()->getRTSTime(core, 0.);
	if (moon[3]==30 || moon[3]<0 || moon[3]>50) // no moonrise on current date
	{
		moonrise = dash;
		ui->buttonMoonrise->setEnabled(false);
	}
	else
	{
		moonriseJD = moon[0];
		moonrise = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(moonriseJD+utcShift), true);
		ui->buttonMoonrise->setEnabled(true);
	}

	if (moon[3]==40 || moon[3]<0 || moon[3]>50) // no moonset on current date
	{
		moonset = dash;
		ui->buttonMoonset->setEnabled(false);
	}
	else
	{
		moonsetJD = moon[2];
		moonset = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(moonsetJD+utcShift), true);
		ui->buttonMoonset->setEnabled(true);
	}

	// Sun
	Vec4d day = sun->getRTSTime(core, 0.);
	if (day[3]==0.)
	{
		sunriseJD = day[0];
		sunsetJD = day[2];
		sunrise = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(sunriseJD+utcShift), true);
		sunset = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(sunsetJD+utcShift), true);
		duration = qAbs(day[2]-day[0])*24.;
		sunBtn = true;
	}
	else
	{
		sunrise = sunset = dash;
		// polar day/night
		duration = (day[3]>99.) ? 24. : 0.;
		sunBtn = false;
	}
	dayDuration = StelUtils::hoursToHmsStr(duration, true);

	// twilights
	Vec4d civilTwilight = sun->getRTSTime(core, -6.);
	Vec4d nauticalTwilight = sun->getRTSTime(core, -12.);
	Vec4d astronomicalTwilight = sun->getRTSTime(core, -18.);
	// tooltips
	// TRANSLATORS: full phrase is "The Sun is XX° below the horizon"
	QString belowHorizon = q_("The Sun is %1 below the horizon");
	ui->labelCivilTwilight->setToolTip(belowHorizon.arg("6°"));
	ui->labelNauticalTwilight->setToolTip(belowHorizon.arg("12°"));
	ui->labelAstronomicalTwilight->setToolTip(belowHorizon.arg("18°"));
	ui->labelDayDuration->setToolTip(q_("The duration of daytime"));
	ui->labelNightDuration->setToolTip(q_("The duration of the astronomical night"));
	QString twilights = q_("The duration of morning and evening twilights");
	ui->labelCivilTwilightDuration->setToolTip(twilights);
	ui->labelNauticalTwilightDuration->setToolTip(twilights);
	ui->labelAstronomicalTwilightDuration->setToolTip(twilights);

	if (astronomicalTwilight[3]==0.)
	{
		astronomicalDawnJD = astronomicalTwilight[0];
		astronomicalDuskJD = astronomicalTwilight[2];
		astronomicalTwilightBegin = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(astronomicalDawnJD+utcShift), true);
		astronomicalTwilightEnd = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(astronomicalDuskJD+utcShift), true);
		duration1 = (nauticalTwilight[2]-astronomicalTwilight[2])*24.;
		if (duration1 > 0.)
			duration1 -= 24.;
		duration2 = (astronomicalTwilight[0]-nauticalTwilight[0])*24.;
		if (duration2 > 0.)
			duration2 -= 24.;
		duration = qAbs(duration1) + qAbs(duration2);
		nightDuration = StelUtils::hoursToHmsStr(24.0 - qAbs(astronomicalTwilight[2]-astronomicalTwilight[0])*24., true);
		astronomicalTwilightBtn = true;
	}
	else
	{
		astronomicalTwilightBegin = astronomicalTwilightEnd = dash;
		duration = 0.;
		// polar night/day
		if (day[3]<-99.)
			nightDuration = StelUtils::hoursToHmsStr(24., true);
		else
			nightDuration = StelUtils::hoursToHmsStr(duration, true);		
		astronomicalTwilightBtn = false;
	}
	astronomicalTwilightDuration = StelUtils::hoursToHmsStr(duration, true);

	if (nauticalTwilight[3]==0.)
	{
		nauticalDawnJD = nauticalTwilight[0];
		nauticalDuskJD = nauticalTwilight[2];
		nauticalTwilightBegin = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(nauticalDawnJD+utcShift), true);
		nauticalTwilightEnd = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(nauticalDuskJD+utcShift), true);
		duration1 = (civilTwilight[2]-nauticalTwilight[2])*24.;
		if (duration1 > 0.)
			duration1 -= 24.;
		duration2 = (nauticalTwilight[0]-civilTwilight[0])*24.;
		if (duration2 > 0.)
			duration2 -= 24.;
		duration = qAbs(duration1) + qAbs(duration2);
		nauticalTwilightBtn = true;
	}
	else
	{
		nauticalTwilightBegin = nauticalTwilightEnd = dash;
		duration = 0.;
		nauticalTwilightBtn = false;
	}
	nauticalTwilightDuration = StelUtils::hoursToHmsStr(duration, true);

	if (civilTwilight[3]==0.)
	{
		civilDawnJD = civilTwilight[0];
		civilDuskJD = civilTwilight[2];
		civilTwilightBegin = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(civilDawnJD+utcShift), true);
		civilTwilightEnd = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(civilDuskJD+utcShift), true);
		duration1 = (day[2]-civilTwilight[2])*24.;
		if (duration1 > 0.)
			duration1 -= 24.;
		duration2 = (civilTwilight[0]-day[0])*24.;
		if (duration2 > 0.)
			duration2 -= 24.;
		duration = qAbs(duration1) + qAbs(duration2);
		civilTwilightBtn = true;
	}
	else
	{
		civilTwilightBegin = civilTwilightEnd = dash;
		duration = 0.;
		civilTwilightBtn = false;
	}
	civilTwilightDuration = StelUtils::hoursToHmsStr(duration, true);

	// fill the data
	ui->labelToday->setText(QString("%1 (%2)").arg(q_("Today"), localeMgr->getPrintableDateLocal(JD, utcOffsetHrs)));

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
	ui->labelNightDuration->setText(nightDuration);

	ui->labelSunrise->setText(sunrise);
	ui->labelSunset->setText(sunset);
	ui->labelMoonRise->setText(moonrise);
	ui->labelMoonSet->setText(moonset);

	// buttons
	ui->buttonSunrise->setEnabled(sunBtn);
	ui->buttonSunset->setEnabled(sunBtn);
	ui->buttonAstronomicalDawn->setEnabled(astronomicalTwilightBtn);
	ui->buttonAstronomicalDusk->setEnabled(astronomicalTwilightBtn);
	ui->buttonNauticalDawn->setEnabled(nauticalTwilightBtn);
	ui->buttonNauticalDusk->setEnabled(nauticalTwilightBtn);
	ui->buttonCivilDawn->setEnabled(civilTwilightBtn);
	ui->buttonCivilDusk->setEnabled(civilTwilightBtn);
}
