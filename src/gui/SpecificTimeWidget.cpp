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

#include "SpecificTimeWidget.hpp"
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

SpecificTimeWidget::SpecificTimeWidget(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui_specificTimeWidget)
{
}

void SpecificTimeWidget::setup()
{
	ui->setupUi(this);
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &SpecificTimeWidget::retranslate);

	core = StelApp::getInstance().getCore();
	specMgr = GETSTELMODULE(SpecificTimeMgr);
	localeMgr = &StelApp::getInstance().getLocaleMgr();

	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(setSeasonLabels()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(setTodayTimes()));
	connect(core, SIGNAL(dateChangedByYear(const int)), this, SLOT(setSeasonTimes()));	
	connect(core, SIGNAL(dateChanged()), this, SLOT(setTodayTimes()));
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

	populateData();

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
}

void SpecificTimeWidget::retranslate()
{
	ui->retranslateUi(this);
	setSeasonTimes();
	setTodayTimes();
}

void SpecificTimeWidget::populateData()
{
	// Set season labels
	setSeasonLabels();
	// Compute equinoxes/solstices and fill the time
	setSeasonTimes();
	// Compute today events
	setTodayTimes();
}

void SpecificTimeWidget::setSeasonLabels()
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

void SpecificTimeWidget::setSeasonTimes()
{
	const double JD = core->getJD() + core->getUTCOffset(core->getJD()) / 24;
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
	ui->labelMarchEquinoxLT->setText(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(marchEquinox), localeMgr->getPrintableTimeLocal(marchEquinox)));
	ui->labelMarchEquinoxDuration->setText(QString("%1 %2").arg(QString::number(juneSolstice-marchEquinox, 'f', daysDepth), days));
	// Summer/Winter
	ui->labelJuneSolsticeJD->setText(QString::number(juneSolstice, 'f', jdDepth));
	ui->labelJuneSolsticeLT->setText(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(juneSolstice), localeMgr->getPrintableTimeLocal(juneSolstice)));
	ui->labelJuneSolsticeDuration->setText(QString("%1 %2").arg(QString::number(septemberEquinox-juneSolstice, 'f', daysDepth), days));
	// Fall/Spring
	ui->labelSeptemberEquinoxJD->setText(QString::number(septemberEquinox, 'f', jdDepth));
	ui->labelSeptemberEquinoxLT->setText(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(septemberEquinox), localeMgr->getPrintableTimeLocal(septemberEquinox)));
	ui->labelSeptemberEquinoxDuration->setText(QString("%1 %2").arg(QString::number(decemberSolstice-septemberEquinox, 'f', daysDepth), days));
	// Winter/Summer
	ui->labelDecemberSolsticeJD->setText(QString::number(decemberSolstice, 'f', jdDepth));
	ui->labelDecemberSolsticeLT->setText(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(decemberSolstice), localeMgr->getPrintableTimeLocal(decemberSolstice)));
	const double duration = (marchEquinox-jdFirstDay) + (jdLastDay-decemberSolstice);
	ui->labelDecemberSolsticeDuration->setText(QString("%1 %2").arg(QString::number(duration, 'f', daysDepth), days));
}

void SpecificTimeWidget::setTodayTimes()
{
	const double utcShift = core->getUTCOffset(core->getJD()) / 24.; // Fix DST shift...
	PlanetP sun = GETSTELMODULE(SolarSystem)->getSun();
	double duration, duration1, duration2;
	QString moonrise, moonset, sunrise, sunset, civilTwilightBegin, civilTwilightEnd, nauticalTwilightBegin,
		nauticalTwilightEnd, astronomicalTwilightBegin, astronomicalTwilightEnd, dayDuration, nightDuration,
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

	// Sun
	Vec4d day = sun->getRTSTime(core, 0.);
	if (day[3]==0.)
	{
		sunrise = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(day[0]+utcShift), true);
		sunset = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(day[2]+utcShift), true);
		duration = qAbs(day[2]-day[0])*24.;
	}
	else
	{
		sunrise = sunset = dash;
		// polar day/night
		duration = (day[3]>99.) ? 24. : 0.;
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
		astronomicalTwilightBegin = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(astronomicalTwilight[0]+utcShift), true);
		astronomicalTwilightEnd = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(astronomicalTwilight[2]+utcShift), true);
		duration1 = (nauticalTwilight[2]-astronomicalTwilight[2])*24.;
		if (duration1 > 0.)
			duration1 -= 24.;
		duration2 = (astronomicalTwilight[0]-nauticalTwilight[0])*24.;
		if (duration2 > 0.)
			duration2 -= 24.;
		duration = qAbs(duration1) + qAbs(duration2);
		nightDuration = StelUtils::hoursToHmsStr(24.0 - qAbs(astronomicalTwilight[2]-astronomicalTwilight[0])*24., true);
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
	}
	astronomicalTwilightDuration = StelUtils::hoursToHmsStr(duration, true);

	if (nauticalTwilight[3]==0.)
	{
		nauticalTwilightBegin = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(nauticalTwilight[0]+utcShift), true);
		nauticalTwilightEnd = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(nauticalTwilight[2]+utcShift), true);
		duration1 = (civilTwilight[2]-nauticalTwilight[2])*24.;
		if (duration1 > 0.)
			duration1 -= 24.;
		duration2 = (nauticalTwilight[0]-civilTwilight[0])*24.;
		if (duration2 > 0.)
			duration2 -= 24.;
		duration = qAbs(duration1) + qAbs(duration2);
	}
	else
	{
		nauticalTwilightBegin = nauticalTwilightEnd = dash;
		duration = 0.;
	}
	nauticalTwilightDuration = StelUtils::hoursToHmsStr(duration, true);

	if (civilTwilight[3]==0.)
	{
		civilTwilightBegin = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(civilTwilight[0]+utcShift), true);
		civilTwilightEnd = StelUtils::hoursToHmsStr(StelUtils::getHoursFromJulianDay(civilTwilight[2]+utcShift), true);
		duration1 = (day[2]-civilTwilight[2])*24.;
		if (duration1 > 0.)
			duration1 -= 24.;
		duration2 = (civilTwilight[0]-day[0])*24.;
		if (duration2 > 0.)
			duration2 -= 24.;
		duration = qAbs(duration1) + qAbs(duration2);
	}
	else
	{
		civilTwilightBegin = civilTwilightEnd = dash;
		duration = 0.;
	}
	civilTwilightDuration = StelUtils::hoursToHmsStr(duration, true);

	// fill the data
	ui->labelToday->setText(QString("%1 (%2)").arg(q_("Today"), localeMgr->getPrintableDateLocal(core->getJD())));

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
}
