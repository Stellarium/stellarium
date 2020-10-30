/*
 * Stellarium
 * Copyright (C) 2020 Alexander Wolf
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "EphemerisMgr.hpp"
#include "StelApp.hpp"
#include "StelTranslator.hpp"
#include "StelProjector.hpp"
#include "SolarSystem.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelObserver.hpp"
#include "StelTextureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"

#include "AstroCalcDialog.hpp"

#include <QSettings>

EphemerisMgr::EphemerisMgr()
	: markersDisplayed(true)
	, datesDisplayed(false)
	, magnitudesDisplayed(false)
	, horizontalCoordinates(false)
	, lineDisplayed(false)
	, lineThickness(1)
	, skipDataDisplayed(false)
	, skipMarkersDisplayed(false)
	, dataStep(1)
	, dataLimit(1)
	, smartDatesDisplayed(true)
	, scaleMarkersDisplayed(false)
	, genericMarkerColor(Vec3f(1.0f, 1.0f, 0.0f))
	, secondaryMarkerColor(Vec3f(0.7f, 0.7f, 1.0f))
	, selectedMarkerColor(Vec3f(1.0f, 0.7f, 0.0f))
	, mercuryMarkerColor(Vec3f(1.0f, 1.0f, 0.0f))
	, venusMarkerColor(Vec3f(1.0f, 1.0f, 1.0f))
	, marsMarkerColor(Vec3f(1.0f, 0.0f, 0.0f))
	, jupiterMarkerColor(Vec3f(0.3f, 1.0f, 1.0f))
	, saturnMarkerColor(Vec3f(0.0f, 1.0f, 0.0f))
{
	setObjectName("EphemerisMgr");
	conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
}

EphemerisMgr::~EphemerisMgr()
{
	texGenericMarker.clear();
	texCometMarker.clear();
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double EphemerisMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("SolarSystem")->getCallOrder(actionName)+1.;
	return 0.;
}

void EphemerisMgr::init()
{
	// Ephemeris stuff
	setFlagMarkers(conf->value("astrocalc/flag_ephemeris_markers", true).toBool());
	setFlagDates(conf->value("astrocalc/flag_ephemeris_dates", false).toBool());
	setFlagMagnitudes(conf->value("astrocalc/flag_ephemeris_magnitudes", false).toBool());
	setFlagHorizontalCoordinates(conf->value("astrocalc/flag_ephemeris_horizontal", false).toBool());
	setFlagLine(conf->value("astrocalc/flag_ephemeris_line", false).toBool());
	setLineThickness(conf->value("astrocalc/ephemeris_line_thickness", 1).toInt());
	setFlagSkipData(conf->value("astrocalc/flag_ephemeris_skip_data", false).toBool());
	setFlagSkipMarkers(conf->value("astrocalc/flag_ephemeris_skip_markers", false).toBool());
	setDataStep(conf->value("astrocalc/ephemeris_data_step", 1).toInt());
	setFlagSmartDates(conf->value("astrocalc/flag_ephemeris_smart_dates", true).toBool());
	setFlagScaleMarkers(conf->value("astrocalc/flag_ephemeris_scale_markers", false).toBool());
	setGenericMarkerColor( Vec3f(conf->value("color/ephemeris_generic_marker_color", "1.0,1.0,0.0").toString()));
	setSecondaryMarkerColor( Vec3f(conf->value("color/ephemeris_secondary_marker_color", "0.7,0.7,1.0").toString()));
	setSelectedMarkerColor(Vec3f(conf->value("color/ephemeris_selected_marker_color", "1.0,0.7,0.0").toString()));
	setMercuryMarkerColor( Vec3f(conf->value("color/ephemeris_mercury_marker_color", "1.0,1.0,0.0").toString()));
	setVenusMarkerColor(   Vec3f(conf->value("color/ephemeris_venus_marker_color", "1.0,1.0,1.0").toString()));
	setMarsMarkerColor(    Vec3f(conf->value("color/ephemeris_mars_marker_color", "1.0,0.0,0.0").toString()));
	setJupiterMarkerColor( Vec3f(conf->value("color/ephemeris_jupiter_marker_color", "0.3,1.0,1.0").toString()));
	setSaturnMarkerColor(  Vec3f(conf->value("color/ephemeris_saturn_marker_color", "0.0,1.0,0.0").toString()));

	texGenericMarker	= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/disk.png");
	texCometMarker		= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/cometIcon.png");

	// Fill ephemeris dates
	connect(this, SIGNAL(requestVisualization()), this, SLOT(fillDates()));
	connect(this, SIGNAL(dataStepChanged(int)), this, SLOT(fillDates()));
	connect(this, SIGNAL(skipDataChanged(bool)), this, SLOT(fillDates()));
	connect(this, SIGNAL(skipMarkersChanged(bool)), this, SLOT(fillDates()));
	connect(this, SIGNAL(smartDatesChanged(bool)), this, SLOT(fillDates()));
}

void EphemerisMgr::update(double deltaTime)
{
	//
}

void EphemerisMgr::draw(StelCore* core)
{
	if (AstroCalcDialog::EphemerisList.count()==0)
		return;

	StelProjectorP prj;
	if (getFlagHorizontalCoordinates())
		prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	else
		prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter sPainter(prj);

	if (getFlagMarkers())
		drawMarkers(&sPainter);

	if (getFlagLine())
		drawLine(&sPainter);
}

Vec3f EphemerisMgr::getMarkerColor(int index) const
{
	// Sync index with AstroCalcDialog::generateEphemeris(). If required, switch to using a QMap.
	const QList<Vec3f> colors={
		genericMarkerColor,
		secondaryMarkerColor,
		mercuryMarkerColor,
		venusMarkerColor,
		marsMarkerColor,
		jupiterMarkerColor,
		saturnMarkerColor};
	return colors.value(index, genericMarkerColor);
}

void EphemerisMgr::drawMarkers(StelPainter *painter)
{
	const int fsize = AstroCalcDialog::EphemerisList.count();
	float size, shift, baseSize = 4.f;
	const bool showDates = getFlagDates();
	const bool showMagnitudes = getFlagMagnitudes();
	const bool showSkippedData = getFlagSkipData();
	const bool skipMarkers = getFlagSkipMarkers();
	const int dataStep = getDataStep();
	const int sizeCoeff = getLineThickness() - 1;
	QString info = "";
	Vec3d win;
	Vec3f markerColor;

	if (getFlagLine() && getFlagScaleMarkers())
		baseSize = 3.f; // The line lies through center of marker

	for (int i =0; i < fsize; i++)
	{
		// Check visibility of pointer
		if (!(painter->getProjector()->projectCheck(AstroCalcDialog::EphemerisList[i].coord, win)))
			continue;

		float solarAngle=0.f; // Angle to possibly rotate the texture. Degrees.
		QString debugStr; // Used temporarily for development
		const bool isComet=AstroCalcDialog::EphemerisList[i].isComet;
		if (i == AstroCalcDialog::DisplayedPositionIndex)
		{
			markerColor = getSelectedMarkerColor();
			size = 6.f;
		}
		else
		{
			markerColor = getMarkerColor(AstroCalcDialog::EphemerisList[i].colorIndex);
			size = baseSize;
		}
		if (isComet) size += 16.f;
		size += sizeCoeff; //
		painter->setColor(markerColor);
		painter->setBlending(true, GL_ONE, GL_ONE);
		if (isComet)
			texCometMarker->bind();
		else
			texGenericMarker->bind();
		if (skipMarkers)
		{
			if ((showDates || showMagnitudes) && showSkippedData && ((i + 1)%dataStep)!=1 && dataStep!=1)
				continue;
		}
		Vec3d win;
		if (painter->getProjector()->project(AstroCalcDialog::EphemerisList[i].coord, win))
		{
			if (isComet)
			{
				// compute solarAngle in screen space.
				Vec3d sunWin;
				painter->getProjector()->project(AstroCalcDialog::EphemerisList[i].sunCoord, sunWin);
				// TODO: In some projections, we may need to test result and flip/mirror the angle, or deal with wrap-around effects.
				// E.g., in cylindrical mode, the comet icon will flip as soon as the corresponding sun position wraps around the screen edge.
				solarAngle=M_180_PIf*static_cast<float>(atan2(-(win[1]-sunWin[1]), win[0]-sunWin[0]));
				// This will show projected positions and angles usable in labels.
				debugStr = QString("Sun: %1/%2 Obj: %3/%4 -->%5").arg(QString::number(sunWin[0]), QString::number(sunWin[1]), QString::number(win[0]), QString::number(win[1]), QString::number(solarAngle));
			}
			painter->drawSprite2dMode(static_cast<float>(win[0]), static_cast<float>(win[1]), size, 270.f-solarAngle);
		}

		if (showDates || showMagnitudes)
		{
			if (showSkippedData && ((i + 1)%dataStep)!=1 && dataStep!=1)
				continue;

			shift = 3.f + size/1.6f;
			if (showDates && showMagnitudes)
				info = QString("%1 (%2)").arg(AstroCalcDialog::EphemerisList[i].objDateStr, QString::number(AstroCalcDialog::EphemerisList[i].magnitude, 'f', 2));
			if (showDates && !showMagnitudes)
				info = AstroCalcDialog::EphemerisList[i].objDateStr;
			if (!showDates && showMagnitudes)
				info = QString::number(AstroCalcDialog::EphemerisList[i].magnitude, 'f', 2);

			// Activate for debug labels.
			//info=debugStr;
			painter->drawText(AstroCalcDialog::EphemerisList[i].coord, info, 0, shift, shift, false);
		}
	}
}

void EphemerisMgr::drawLine(StelPainter *painter)
{
	const int size = AstroCalcDialog::EphemerisList.count();
	const float oldLineThickness=painter->getLineWidth();
	const float lineThickness = getLineThickness();
	if (!fuzzyEquals(lineThickness, oldLineThickness))
		painter->setLineWidth(lineThickness);

	Vec3f color;
	QVector<Vec3d> vertexArray;
	QVector<Vec4f> colorArray;
	const int limit = getDataLimit();
	const int nsize = static_cast<int>(size/limit);
	vertexArray.resize(nsize);
	colorArray.resize(nsize);
	for (int j=0; j<limit; j++)
	{
		for (int i =0; i < nsize; i++)
		{
			color = getMarkerColor(AstroCalcDialog::EphemerisList[i + j*nsize].colorIndex);
			colorArray[i]=Vec4f(color, 1.0f);
			vertexArray[i]=AstroCalcDialog::EphemerisList[i + j*nsize].coord;
		}
		painter->drawPath(vertexArray, colorArray);
	}

	if (!fuzzyEquals(lineThickness, oldLineThickness))
		painter->setLineWidth(oldLineThickness); // restore line thickness
}

void EphemerisMgr::fillDates()
{
	const int fsize = AstroCalcDialog::EphemerisList.count();
	if (fsize==0) return;

	StelLocaleMgr* localeMgr = &StelApp::getInstance().getLocaleMgr();
	const bool showSmartDates = getFlagSmartDates();
	double JD = AstroCalcDialog::EphemerisList.first().objDate;
	bool withTime = (fsize>1 && (AstroCalcDialog::EphemerisList[1].objDate-JD<1.0));

	int fYear, fMonth, fDay, sYear, sMonth, sDay, h, m, s;
	QString info;
	const double shift = StelApp::getInstance().getCore()->getUTCOffset(JD)*0.041666666666;
	StelUtils::getDateFromJulianDay(JD+shift, &fYear, &fMonth, &fDay);
	bool sFlag = true;
	sYear = fYear;
	sMonth = fMonth;
	sDay = fDay;
	const bool showSkippedData = getFlagSkipData();
	const int dataStep = getDataStep();

	for (int i = 0; i < fsize; i++)
	{
		JD = AstroCalcDialog::EphemerisList[i].objDate;
		StelUtils::getDateFromJulianDay(JD+shift, &fYear, &fMonth, &fDay);

		if (showSkippedData && ((i + 1)%dataStep)!=1 && dataStep!=1)
			continue;

		if (showSmartDates)
		{
			if (sFlag)
				info = QString("%1").arg(fYear);

			if (info.isEmpty() && !sFlag && fYear!=sYear)
				info = QString("%1").arg(fYear);

			if (!info.isEmpty())
				info.append(QString("/%1").arg(localeMgr->romanMonthName(fMonth)));
			else if (fMonth!=sMonth)
				info = QString("%1").arg(localeMgr->romanMonthName(fMonth));

			if (!info.isEmpty())
				info.append(QString("/%1").arg(fDay));
			else
				info = QString("%1").arg(fDay);

			if (withTime) // very short step
			{
				if (fDay==sDay && !sFlag)
					info.clear();

				StelUtils::getTimeFromJulianDay(JD+shift, &h, &m, &s);
				if (!info.isEmpty())
					info.append(QString(" %1:%2").arg(h).arg(m));
				else
					info = QString("%1:%2").arg(h).arg(m);
			}

			AstroCalcDialog::EphemerisList[i].objDateStr = info;
			info.clear();
			sYear = fYear;
			sMonth = fMonth;
			sDay = fDay;
			sFlag = false;
		}
		else
		{
			// OK, let's use standard formats for date and time (as defined for whole planetarium)
			if (withTime)
				AstroCalcDialog::EphemerisList[i].objDateStr = QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD));
			else
				AstroCalcDialog::EphemerisList[i].objDateStr = localeMgr->getPrintableDateLocal(JD);
		}
	}
}

void EphemerisMgr::setFlagMarkers(bool b)
{
	if (b!=markersDisplayed)
	{
		markersDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_markers", b); // Immediate saving of state
		emit markersDisplayedChanged(b);
	}
}

bool EphemerisMgr::getFlagMarkers() const
{
	return markersDisplayed;
}

void EphemerisMgr::setFlagLine(bool b)
{
	if (b!=lineDisplayed)
	{
		lineDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_line", b); // Immediate saving of state
		emit lineDisplayedChanged(b);
	}
}

bool EphemerisMgr::getFlagLine() const
{
	return lineDisplayed;
}

void EphemerisMgr::setFlagHorizontalCoordinates(bool b)
{
	if (b!=horizontalCoordinates)
	{
		horizontalCoordinates=b;
		conf->setValue("astrocalc/flag_ephemeris_horizontal", b); // Immediate saving of state
		emit horizontalCoordinatesChanged(b);
	}
}

bool EphemerisMgr::getFlagHorizontalCoordinates() const
{
	return horizontalCoordinates;
}

void EphemerisMgr::setFlagDates(bool b)
{
	if (b!=datesDisplayed)
	{
		datesDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_dates", b); // Immediate saving of state
		emit datesDisplayedChanged(b);
	}
}

bool EphemerisMgr::getFlagDates() const
{
	return datesDisplayed;
}

void EphemerisMgr::setFlagMagnitudes(bool b)
{
	if (b!=magnitudesDisplayed)
	{
		magnitudesDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_magnitudes", b); // Immediate saving of state
		emit magnitudesDisplayedChanged(b);
	}
}

bool EphemerisMgr::getFlagMagnitudes() const
{
	return magnitudesDisplayed;
}

void EphemerisMgr::setFlagSkipData(bool b)
{
	if (b!=skipDataDisplayed)
	{
		skipDataDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_skip_data", b); // Immediate saving of state
		emit skipDataChanged(b);
	}
}

bool EphemerisMgr::getFlagSkipData() const
{
	return skipDataDisplayed;
}

void EphemerisMgr::setFlagSkipMarkers(bool b)
{
	if (b!=skipMarkersDisplayed)
	{
		skipMarkersDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_skip_markers", b); // Immediate saving of state
		emit skipMarkersChanged(b);
	}
}

bool EphemerisMgr::getFlagSkipMarkers() const
{
	return skipMarkersDisplayed;
}

void EphemerisMgr::setFlagSmartDates(bool b)
{
	if (b!=smartDatesDisplayed)
	{
		smartDatesDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_smart_dates", b); // Immediate saving of state
		emit smartDatesChanged(b);
	}
}

bool EphemerisMgr::getFlagSmartDates() const
{
	return smartDatesDisplayed;
}

void EphemerisMgr::setFlagScaleMarkers(bool b)
{
	if (b!=scaleMarkersDisplayed)
	{
		scaleMarkersDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_scale_markers", b); // Immediate saving of state
		emit scaleMarkersChanged(b);
	}
}

bool EphemerisMgr::getFlagScaleMarkers() const
{
	return scaleMarkersDisplayed;
}

void EphemerisMgr::setDataStep(int step)
{
	dataStep = step;
	// automatic saving of the setting
	conf->setValue("astrocalc/ephemeris_data_step", step);
	emit dataStepChanged(step);
}

int EphemerisMgr::getDataStep() const
{
	return dataStep;
}

void EphemerisMgr::setDataLimit(int limit)
{
	dataLimit = limit;
	emit dataLimitChanged(limit);
}

int EphemerisMgr::getDataLimit() const
{
	return dataLimit;
}

void EphemerisMgr::setLineThickness(int v)
{
	lineThickness = v;
	// automatic saving of the setting
	conf->setValue("astrocalc/ephemeris_line_thickness", v);
	emit lineThicknessChanged(v);
}

int EphemerisMgr::getLineThickness() const
{
	return lineThickness;
}

void EphemerisMgr::setGenericMarkerColor(const Vec3f& color)
{
	if (color!=genericMarkerColor)
	{
		genericMarkerColor = color;
		emit genericMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getGenericMarkerColor() const
{
	return genericMarkerColor;
}

void EphemerisMgr::setSecondaryMarkerColor(const Vec3f& color)
{
	if (color!=secondaryMarkerColor)
	{
		secondaryMarkerColor = color;
		emit secondaryMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getSecondaryMarkerColor() const
{
	return secondaryMarkerColor;
}

void EphemerisMgr::setSelectedMarkerColor(const Vec3f& color)
{
	if (color!=selectedMarkerColor)
	{
		selectedMarkerColor = color;
		emit selectedMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getSelectedMarkerColor() const
{
	return selectedMarkerColor;
}

void EphemerisMgr::setMercuryMarkerColor(const Vec3f& color)
{
	if (color!=mercuryMarkerColor)
	{
		mercuryMarkerColor = color;
		emit mercuryMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getMercuryMarkerColor() const
{
	return mercuryMarkerColor;
}

void EphemerisMgr::setVenusMarkerColor(const Vec3f& color)
{
	if (color!=venusMarkerColor)
	{
		venusMarkerColor = color;
		emit venusMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getVenusMarkerColor() const
{
	return venusMarkerColor;
}

void EphemerisMgr::setMarsMarkerColor(const Vec3f& color)
{
	if (color!=marsMarkerColor)
	{
		marsMarkerColor = color;
		emit marsMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getMarsMarkerColor() const
{
	return marsMarkerColor;
}

void EphemerisMgr::setJupiterMarkerColor(const Vec3f& color)
{
	if (color!=jupiterMarkerColor)
	{
		jupiterMarkerColor = color;
		emit jupiterMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getJupiterMarkerColor() const
{
	return jupiterMarkerColor;
}

void EphemerisMgr::setSaturnMarkerColor(const Vec3f& color)
{
	if (color!=saturnMarkerColor)
	{
		saturnMarkerColor = color;
		emit saturnMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getSaturnMarkerColor() const
{
	return saturnMarkerColor;
}
