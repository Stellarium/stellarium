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
#include "StelCore.hpp"
#include "StelObserver.hpp"
#include "StelPainter.hpp"
#include "StelTextureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"

#include "AstroCalcDialog.hpp"

#include <QSettings>

EphemerisMgr::EphemerisMgr()
	: ephemerisMarkersDisplayed(true)
	, ephemerisDatesDisplayed(false)
	, ephemerisMagnitudesDisplayed(false)
	, ephemerisHorizontalCoordinates(false)
	, ephemerisLineDisplayed(false)
	, ephemerisLineThickness(1)
	, ephemerisSkipDataDisplayed(false)
	, ephemerisSkipMarkersDisplayed(false)
	, ephemerisDataStep(1)
	, ephemerisDataLimit(1)
	, ephemerisSmartDatesDisplayed(true)
	, ephemerisScaleMarkersDisplayed(false)
	, ephemerisGenericMarkerColor(Vec3f(1.0f, 1.0f, 0.0f))
	, ephemerisSecondaryMarkerColor(Vec3f(0.7f, 0.7f, 1.0f))
	, ephemerisSelectedMarkerColor(Vec3f(1.0f, 0.7f, 0.0f))
	, ephemerisMercuryMarkerColor(Vec3f(1.0f, 1.0f, 0.0f))
	, ephemerisVenusMarkerColor(Vec3f(1.0f, 1.0f, 1.0f))
	, ephemerisMarsMarkerColor(Vec3f(1.0f, 0.0f, 0.0f))
	, ephemerisJupiterMarkerColor(Vec3f(0.3f, 1.0f, 1.0f))
	, ephemerisSaturnMarkerColor(Vec3f(0.0f, 1.0f, 0.0f))
{
	setObjectName("EphemerisMgr");
}

EphemerisMgr::~EphemerisMgr()
{
	texEphemerisMarker.clear();
	texEphemerisCometMarker.clear();
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
	conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	// Ephemeris stuff
	setFlagEphemerisMarkers(conf->value("astrocalc/flag_ephemeris_markers", true).toBool());
	setFlagEphemerisDates(conf->value("astrocalc/flag_ephemeris_dates", false).toBool());
	setFlagEphemerisMagnitudes(conf->value("astrocalc/flag_ephemeris_magnitudes", false).toBool());
	setFlagEphemerisHorizontalCoordinates(conf->value("astrocalc/flag_ephemeris_horizontal", false).toBool());
	setFlagEphemerisLine(conf->value("astrocalc/flag_ephemeris_line", false).toBool());
	setEphemerisLineThickness(conf->value("astrocalc/ephemeris_line_thickness", 1).toInt());
	setFlagEphemerisSkipData(conf->value("astrocalc/flag_ephemeris_skip_data", false).toBool());
	setFlagEphemerisSkipMarkers(conf->value("astrocalc/flag_ephemeris_skip_markers", false).toBool());
	setEphemerisDataStep(conf->value("astrocalc/ephemeris_data_step", 1).toInt());
	setFlagEphemerisSmartDates(conf->value("astrocalc/flag_ephemeris_smart_dates", true).toBool());
	setFlagEphemerisScaleMarkers(conf->value("astrocalc/flag_ephemeris_scale_markers", false).toBool());
	setEphemerisGenericMarkerColor( Vec3f(conf->value("color/ephemeris_generic_marker_color", "1.0,1.0,0.0").toString()));
	setEphemerisSecondaryMarkerColor( Vec3f(conf->value("color/ephemeris_secondary_marker_color", "0.7,0.7,1.0").toString()));
	setEphemerisSelectedMarkerColor(Vec3f(conf->value("color/ephemeris_selected_marker_color", "1.0,0.7,0.0").toString()));
	setEphemerisMercuryMarkerColor( Vec3f(conf->value("color/ephemeris_mercury_marker_color", "1.0,1.0,0.0").toString()));
	setEphemerisVenusMarkerColor(   Vec3f(conf->value("color/ephemeris_venus_marker_color", "1.0,1.0,1.0").toString()));
	setEphemerisMarsMarkerColor(    Vec3f(conf->value("color/ephemeris_mars_marker_color", "1.0,0.0,0.0").toString()));
	setEphemerisJupiterMarkerColor( Vec3f(conf->value("color/ephemeris_jupiter_marker_color", "0.3,1.0,1.0").toString()));
	setEphemerisSaturnMarkerColor(  Vec3f(conf->value("color/ephemeris_saturn_marker_color", "0.0,1.0,0.0").toString()));

	texEphemerisMarker = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/disk.png");
	texEphemerisCometMarker = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/cometIcon.png");

	// Fill ephemeris dates
	connect(this, SIGNAL(requestEphemerisVisualization()), this, SLOT(fillEphemerisDates()));
	connect(this, SIGNAL(ephemerisDataStepChanged(int)), this, SLOT(fillEphemerisDates()));
	connect(this, SIGNAL(ephemerisSkipDataChanged(bool)), this, SLOT(fillEphemerisDates()));
	connect(this, SIGNAL(ephemerisSkipMarkersChanged(bool)), this, SLOT(fillEphemerisDates()));
	connect(this, SIGNAL(ephemerisSmartDatesChanged(bool)), this, SLOT(fillEphemerisDates()));
}

void EphemerisMgr::update(double deltaTime)
{
	//
}

void EphemerisMgr::draw(StelCore* core)
{
	// AstroCalcDialog
	if (getFlagEphemerisMarkers())
		drawEphemerisMarkers(core);

	if (getFlagEphemerisLine())
		drawEphemerisLine(core);
}

Vec3f EphemerisMgr::getEphemerisMarkerColor(int index) const
{
	// Sync index with AstroCalcDialog::generateEphemeris(). If required, switch to using a QMap.
	const QList<Vec3f> colors={
		ephemerisGenericMarkerColor,
		ephemerisSecondaryMarkerColor,
		ephemerisMercuryMarkerColor,
		ephemerisVenusMarkerColor,
		ephemerisMarsMarkerColor,
		ephemerisJupiterMarkerColor,
		ephemerisSaturnMarkerColor};
	return colors.value(index, ephemerisGenericMarkerColor);
}

void EphemerisMgr::drawEphemerisMarkers(const StelCore *core)
{
	const int fsize = AstroCalcDialog::EphemerisList.count();
	if (fsize==0) return;

	StelProjectorP prj;
	if (getFlagEphemerisHorizontalCoordinates())
		prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	else
		prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter sPainter(prj);

	float size, shift, baseSize = 4.f;
	const bool showDates = getFlagEphemerisDates();
	const bool showMagnitudes = getFlagEphemerisMagnitudes();
	const bool showSkippedData = getFlagEphemerisSkipData();
	const bool skipMarkers = getFlagEphemerisSkipMarkers();
	const int dataStep = getEphemerisDataStep();
	const int sizeCoeff = getEphemerisLineThickness() - 1;
	QString info = "";
	Vec3d win;
	Vec3f markerColor;

	if (getFlagEphemerisLine() && getFlagEphemerisScaleMarkers())
		baseSize = 3.f; // The line lies through center of marker

	for (int i =0; i < fsize; i++)
	{
		// Check visibility of pointer
		if (!(sPainter.getProjector()->projectCheck(AstroCalcDialog::EphemerisList[i].coord, win)))
			continue;

		float solarAngle=0.f; // Angle to possibly rotate the texture. Degrees.
		QString debugStr; // Used temporarily for development
		const bool isComet=AstroCalcDialog::EphemerisList[i].isComet;
		if (i == AstroCalcDialog::DisplayedPositionIndex)
		{
			markerColor = getEphemerisSelectedMarkerColor();
			size = 6.f;
		}
		else
		{
			markerColor = getEphemerisMarkerColor(AstroCalcDialog::EphemerisList[i].colorIndex);
			size = baseSize;
		}
		if (isComet) size += 16.f;
		size += sizeCoeff; //
		sPainter.setColor(markerColor);
		sPainter.setBlending(true, GL_ONE, GL_ONE);
		if (isComet)
			texEphemerisCometMarker->bind();
		else
			texEphemerisMarker->bind();
		if (skipMarkers)
		{
			if ((showDates || showMagnitudes) && showSkippedData && ((i + 1)%dataStep)!=1 && dataStep!=1)
				continue;
		}
		Vec3d win;
		if (prj->project(AstroCalcDialog::EphemerisList[i].coord, win))
		{
			if (isComet)
			{
				// compute solarAngle in screen space.
				Vec3d sunWin;
				prj->project(AstroCalcDialog::EphemerisList[i].sunCoord, sunWin);
				// TODO: In some projections, we may need to test result and flip/mirror the angle, or deal with wrap-around effects.
				// E.g., in cylindrical mode, the comet icon will flip as soon as the corresponding sun position wraps around the screen edge.
				solarAngle=M_180_PIf*static_cast<float>(atan2(-(win[1]-sunWin[1]), win[0]-sunWin[0]));
				// This will show projected positions and angles usable in labels.
				debugStr = QString("Sun: %1/%2 Obj: %3/%4 -->%5").arg(QString::number(sunWin[0]), QString::number(sunWin[1]), QString::number(win[0]), QString::number(win[1]), QString::number(solarAngle));
			}
			//sPainter.drawSprite2dMode(static_cast<float>(win[0]), static_cast<float>(win[1]), size, 180.f+AstroCalcDialog::EphemerisList[i].solarAngle*M_180_PIf);
			sPainter.drawSprite2dMode(static_cast<float>(win[0]), static_cast<float>(win[1]), size, 270.f-solarAngle);
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
			sPainter.drawText(AstroCalcDialog::EphemerisList[i].coord, info, 0, shift, shift, false);
		}
	}
}

void EphemerisMgr::drawEphemerisLine(const StelCore *core)
{
	const int size = AstroCalcDialog::EphemerisList.count();
	if (size==0) return;

	// The array of data is not empty - good news!
	StelProjectorP prj;
	if (getFlagEphemerisHorizontalCoordinates())
		prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	else
		prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter sPainter(prj);

	const float oldLineThickness=sPainter.getLineWidth();
	const float lineThickness = getEphemerisLineThickness();
	if (!fuzzyEquals(lineThickness, oldLineThickness))
		sPainter.setLineWidth(lineThickness);

	Vec3f color;
	QVector<Vec3d> vertexArray;
	QVector<Vec4f> colorArray;
	const int limit = getEphemerisDataLimit();
	const int nsize = static_cast<int>(size/limit);
	vertexArray.resize(nsize);
	colorArray.resize(nsize);
	for (int j=0; j<limit; j++)
	{
		for (int i =0; i < nsize; i++)
		{
			color = getEphemerisMarkerColor(AstroCalcDialog::EphemerisList[i + j*nsize].colorIndex);
			colorArray[i]=Vec4f(color, 1.0f);
			vertexArray[i]=AstroCalcDialog::EphemerisList[i + j*nsize].coord;
		}
		sPainter.drawPath(vertexArray, colorArray);
	}

	if (!fuzzyEquals(lineThickness, oldLineThickness))
		sPainter.setLineWidth(oldLineThickness); // restore line thickness
}

void EphemerisMgr::fillEphemerisDates()
{
	const int fsize = AstroCalcDialog::EphemerisList.count();
	if (fsize==0) return;

	StelLocaleMgr* localeMgr = &StelApp::getInstance().getLocaleMgr();
	const bool showSmartDates = getFlagEphemerisSmartDates();
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
	const bool showSkippedData = getFlagEphemerisSkipData();
	const int dataStep = getEphemerisDataStep();

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

void EphemerisMgr::setFlagEphemerisMarkers(bool b)
{
	if (b!=ephemerisMarkersDisplayed)
	{
		ephemerisMarkersDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_markers", b); // Immediate saving of state
		emit ephemerisMarkersChanged(b);
	}
}

bool EphemerisMgr::getFlagEphemerisMarkers() const
{
	return ephemerisMarkersDisplayed;
}

void EphemerisMgr::setFlagEphemerisLine(bool b)
{
	if (b!=ephemerisLineDisplayed)
	{
		ephemerisLineDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_line", b); // Immediate saving of state
		emit ephemerisLineChanged(b);
	}
}

bool EphemerisMgr::getFlagEphemerisLine() const
{
	return ephemerisLineDisplayed;
}

void EphemerisMgr::setFlagEphemerisHorizontalCoordinates(bool b)
{
	if (b!=ephemerisHorizontalCoordinates)
	{
		ephemerisHorizontalCoordinates=b;
		conf->setValue("astrocalc/flag_ephemeris_horizontal", b); // Immediate saving of state
		emit ephemerisHorizontalCoordinatesChanged(b);
	}
}

bool EphemerisMgr::getFlagEphemerisHorizontalCoordinates() const
{
	return ephemerisHorizontalCoordinates;
}

void EphemerisMgr::setFlagEphemerisDates(bool b)
{
	if (b!=ephemerisDatesDisplayed)
	{
		ephemerisDatesDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_dates", b); // Immediate saving of state
		emit ephemerisDatesChanged(b);
	}
}

bool EphemerisMgr::getFlagEphemerisDates() const
{
	return ephemerisDatesDisplayed;
}

void EphemerisMgr::setFlagEphemerisMagnitudes(bool b)
{
	if (b!=ephemerisMagnitudesDisplayed)
	{
		ephemerisMagnitudesDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_magnitudes", b); // Immediate saving of state
		emit ephemerisMagnitudesChanged(b);
	}
}

bool EphemerisMgr::getFlagEphemerisMagnitudes() const
{
	return ephemerisMagnitudesDisplayed;
}

void EphemerisMgr::setFlagEphemerisSkipData(bool b)
{
	if (b!=ephemerisSkipDataDisplayed)
	{
		ephemerisSkipDataDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_skip_data", b); // Immediate saving of state
		emit ephemerisSkipDataChanged(b);
	}
}

bool EphemerisMgr::getFlagEphemerisSkipData() const
{
	return ephemerisSkipDataDisplayed;
}

void EphemerisMgr::setFlagEphemerisSkipMarkers(bool b)
{
	if (b!=ephemerisSkipMarkersDisplayed)
	{
		ephemerisSkipMarkersDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_skip_markers", b); // Immediate saving of state
		emit ephemerisSkipMarkersChanged(b);
	}
}

bool EphemerisMgr::getFlagEphemerisSkipMarkers() const
{
	return ephemerisSkipMarkersDisplayed;
}

void EphemerisMgr::setFlagEphemerisSmartDates(bool b)
{
	if (b!=ephemerisSmartDatesDisplayed)
	{
		ephemerisSmartDatesDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_smart_dates", b); // Immediate saving of state
		emit ephemerisSmartDatesChanged(b);
	}
}

bool EphemerisMgr::getFlagEphemerisSmartDates() const
{
	return ephemerisSmartDatesDisplayed;
}

void EphemerisMgr::setFlagEphemerisScaleMarkers(bool b)
{
	if (b!=ephemerisScaleMarkersDisplayed)
	{
		ephemerisScaleMarkersDisplayed=b;
		conf->setValue("astrocalc/flag_ephemeris_scale_markers", b); // Immediate saving of state
		emit ephemerisScaleMarkersChanged(b);
	}
}

bool EphemerisMgr::getFlagEphemerisScaleMarkers() const
{
	return ephemerisScaleMarkersDisplayed;
}

void EphemerisMgr::setEphemerisDataStep(int step)
{
	ephemerisDataStep = step;
	// automatic saving of the setting
	conf->setValue("astrocalc/ephemeris_data_step", step);
	emit ephemerisDataStepChanged(step);
}

int EphemerisMgr::getEphemerisDataStep() const
{
	return ephemerisDataStep;
}

void EphemerisMgr::setEphemerisDataLimit(int limit)
{
	ephemerisDataLimit = limit;
	emit ephemerisDataLimitChanged(limit);
}

int EphemerisMgr::getEphemerisDataLimit() const
{
	return ephemerisDataLimit;
}

void EphemerisMgr::setEphemerisLineThickness(int v)
{
	ephemerisLineThickness = v;
	// automatic saving of the setting
	conf->setValue("astrocalc/ephemeris_line_thickness", v);
	emit ephemerisLineThicknessChanged(v);
}

int EphemerisMgr::getEphemerisLineThickness() const
{
	return ephemerisLineThickness;
}

void EphemerisMgr::setEphemerisGenericMarkerColor(const Vec3f& color)
{
	if (color!=ephemerisGenericMarkerColor)
	{
		ephemerisGenericMarkerColor = color;
		emit ephemerisGenericMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getEphemerisGenericMarkerColor() const
{
	return ephemerisGenericMarkerColor;
}

void EphemerisMgr::setEphemerisSecondaryMarkerColor(const Vec3f& color)
{
	if (color!=ephemerisSecondaryMarkerColor)
	{
		ephemerisSecondaryMarkerColor = color;
		emit ephemerisSecondaryMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getEphemerisSecondaryMarkerColor() const
{
	return ephemerisSecondaryMarkerColor;
}

void EphemerisMgr::setEphemerisSelectedMarkerColor(const Vec3f& color)
{
	if (color!=ephemerisSelectedMarkerColor)
	{
		ephemerisSelectedMarkerColor = color;
		emit ephemerisSelectedMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getEphemerisSelectedMarkerColor() const
{
	return ephemerisSelectedMarkerColor;
}

void EphemerisMgr::setEphemerisMercuryMarkerColor(const Vec3f& color)
{
	if (color!=ephemerisMercuryMarkerColor)
	{
		ephemerisMercuryMarkerColor = color;
		emit ephemerisMercuryMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getEphemerisMercuryMarkerColor() const
{
	return ephemerisMercuryMarkerColor;
}

void EphemerisMgr::setEphemerisVenusMarkerColor(const Vec3f& color)
{
	if (color!=ephemerisVenusMarkerColor)
	{
		ephemerisVenusMarkerColor = color;
		emit ephemerisVenusMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getEphemerisVenusMarkerColor() const
{
	return ephemerisVenusMarkerColor;
}

void EphemerisMgr::setEphemerisMarsMarkerColor(const Vec3f& color)
{
	if (color!=ephemerisMarsMarkerColor)
	{
		ephemerisMarsMarkerColor = color;
		emit ephemerisMarsMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getEphemerisMarsMarkerColor() const
{
	return ephemerisMarsMarkerColor;
}

void EphemerisMgr::setEphemerisJupiterMarkerColor(const Vec3f& color)
{
	if (color!=ephemerisJupiterMarkerColor)
	{
		ephemerisJupiterMarkerColor = color;
		emit ephemerisJupiterMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getEphemerisJupiterMarkerColor() const
{
	return ephemerisJupiterMarkerColor;
}

void EphemerisMgr::setEphemerisSaturnMarkerColor(const Vec3f& color)
{
	if (color!=ephemerisSaturnMarkerColor)
	{
		ephemerisSaturnMarkerColor = color;
		emit ephemerisSaturnMarkerColorChanged(color);
	}
}

Vec3f EphemerisMgr::getEphemerisSaturnMarkerColor() const
{
	return ephemerisSaturnMarkerColor;
}
