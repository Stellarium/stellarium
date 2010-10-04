/*
 * Stellarium
 * Copyright (C) 2010 Bogdan Marinov
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
#include "MinorPlanet.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelNavigator.hpp"
#include "StelTexture.hpp"
#include "StelTextureMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"

#include <QRegExp>
#include <QDebug>

MinorPlanet::MinorPlanet(const QString& englishName,
						 int flagLighting,
						 double radius,
						 double oblateness,
						 Vec3f color,
						 float albedo,
						 const QString& atexMapName,
						 posFuncType coordFunc,
						 void* auserDataPtr,
						 OsulatingFunctType *osculatingFunc,
						 bool acloseOrbit,
						 bool hidden,
						 bool hasAtmosphere)
						: Planet (englishName,
								  flagLighting,
								  radius,
								  oblateness,
								  color,
								  albedo,
								  atexMapName,
								  coordFunc,
								  auserDataPtr,
								  osculatingFunc,
								  acloseOrbit,
								  hidden,
								  false) //No atmosphere
{
	texMapName = atexMapName;
	lastOrbitJD =0;
	deltaJD = JD_SECOND;
	orbitCached = 0;
	closeOrbit = acloseOrbit;

	eclipticPos=Vec3d(0.,0.,0.);
	rotLocalToParent = Mat4d::identity();
	texMap = StelApp::getInstance().getTextureManager().createTextureThread("textures/"+texMapName, StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));

	minorPlanetNumber = 0;

	//TODO: Fix the name
	// - Detect numeric prefix and set number if any
	// - detect provisional designation
	// - create the HTML name
	//Try to detect number
	//TODO: Move this to the minor planet parse code in the plug-in?	
	/*
	QString name = englishName;
	QRegExp bracketedNumberPrefixPattern("^\\((\\d+)\\)\\s");
	QRegExp freeNumberPrefixPattern("^(\\d+)\\s[A-Za-z]{3,}");
	if (bracketedNumberPrefixPattern.indexIn(name) == 0)
	{
		QString numberString = bracketedNumberPrefixPattern.cap(1);
		bool ok = false;
		number = numberString.toInt(&ok);
		if (!ok)
			number = 0;

		//TODO: Handle a name consisting only of a number
		name.remove(0, numberString.length() + 3);
		htmlName = QString("(%1) ").arg(number);
	}
	else if (freeNumberPrefixPattern.indexIn(name) == 0)
	{
		QString numberString =freeNumberPrefixPattern.cap(1);
		bool ok = false;
		number = numberString.toInt(&ok);
		if (!ok)
			number = 0;

		//TODO: Handle a name consisting only of a number
		name.remove(0, numberString.length() + 3);
		htmlName = QString("(%1) ").arg(number);
	}*/

	//Try to detect provisional designation
	QRegExp provisionalDesignationPattern("^(\\d{4}\\s[A-Z]{2})(\\d*)$");
	if (provisionalDesignationPattern.indexIn(englishName) == 0)
	{
		QString main = provisionalDesignationPattern.cap(1);
		QString suffix = provisionalDesignationPattern.cap(2);
		if (!suffix.isEmpty())
		{
			htmlName = (QString("%1<sub>%2</sub>").arg(main).arg(suffix));
		}
	}
	else
	{
		qDebug() << "Not a provisional designation?";
	}

	nameI18 = englishName;

	flagLabels = true;
}

MinorPlanet::~MinorPlanet()
{
	//Do nothing for the moment
}

void MinorPlanet::setNumber(int number)
{
	if (minorPlanetNumber)
		return;

	minorPlanetNumber = number;
}

QString MinorPlanet::getInfoString(const StelCore *core, const InfoStringGroup &flags) const
{
	//Mostly copied from Planet::getInfoString():
	const StelNavigator* nav = core->getNavigator();

	QString str;
	QTextStream oss(&str);

	if (flags&Name)
	{
		oss << "<h2>";
		if (minorPlanetNumber)
			oss << QString("(%1) ").arg(minorPlanetNumber);
		if (!htmlName.isEmpty())
			oss << htmlName;
		else
			oss << q_(englishName);  // UI translation can differ from sky translation
		oss.setRealNumberNotation(QTextStream::FixedNotation);
		oss.setRealNumberPrecision(1);
		if (sphereScale != 1.f)
			oss << QString::fromUtf8(" (\xC3\x97") << sphereScale << ")";
		oss << "</h2>";
	}

	if (flags&Magnitude)
		oss << q_("Magnitude: <b>%1</b>").arg(getVMagnitude(nav), 0, 'f', 2) << "<br>";

	if (flags&AbsoluteMagnitude)
		oss << q_("Absolute Magnitude: %1").arg(getVMagnitude(nav)-5.*(std::log10(getJ2000EquatorialPos(nav).length()*AU/PARSEC)-1.), 0, 'f', 2) << "<br>";

	oss << getPositionInfoString(core, flags);

	if (flags&Distance)
	{
		// xgettext:no-c-format
		oss << q_("Distance: %1AU").arg(getJ2000EquatorialPos(nav).length(), 0, 'f', 8) << "<br>";
	}

	if (flags&Size)
		oss << q_("Apparent diameter: %1").arg(StelUtils::radToDmsStr(2.*getAngularSize(core)*M_PI/180., true));

	postProcessInfoString(str, flags);

	return str;
}

float MinorPlanet::getVMagnitude(const StelNavigator *nav) const
{
	//TODO
	return 1.;
}

