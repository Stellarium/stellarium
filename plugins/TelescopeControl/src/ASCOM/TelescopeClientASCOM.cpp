/*
 * Copyright (C) 2019 Gion Kunz <gion.kunz@gmail.com>
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

#include "TelescopeClientASCOM.hpp"

#include <QDebug>
#include <QMessageBox>
#include <QRegularExpression>
#include <cmath>
#include <limits>

#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelMainView.hpp"
#include "StelTranslator.hpp"

TelescopeClientASCOM::TelescopeClientASCOM(const QString& name, const QString& params, TelescopeControl::Equinox eq)
	: TelescopeClient(name)
	, mEquinox(eq)
	, mInterpolatedPosition(1, 0, 0)
	, mCurrentTargetPosition(1, 0, 0)
{
	static const QRegularExpression paramRx("^([^:]*):([^:]*)$");
	QRegularExpressionMatch paramMatch=paramRx.match(params);
	if (paramMatch.hasMatch())
	{
		mAscomDeviceId = paramMatch.captured(1).trimmed();
		mAscomUseDeviceEqCoordType = paramMatch.captured(2).trimmed() == "true";
	}

	qDebug() << "TelescopeClientASCOM::TelescopeClientASCOM with telescope name " << name << " and ascomDeviceId " << mAscomDeviceId;

	mAscomDevice = new ASCOMDevice(this, mAscomDeviceId);
	mAscomDevice->connect();
	mDoesRefraction = mAscomDevice->doesRefraction();
	mCoordinateType = mAscomDevice->getEquatorialCoordinateType();
}

TelescopeClientASCOM::~TelescopeClientASCOM()
{
	mAscomDevice->disconnect();
	delete mAscomDevice;
}

void TelescopeClientASCOM::performCommunication()
{
	qint64 now = getNow();

	if (now - mLastUpdateTime > 1000000)
	{
		ASCOMDevice::ASCOMCoordinates currentCoords = mAscomDevice->getCoordinates();

		double longitudeRad = currentCoords.RA * M_PI / 12.0;
		double latitudeRad = currentCoords.DEC * M_PI / 180.0;

		Vec3d position;
		StelUtils::spheToRect(longitudeRad, latitudeRad, position);

		// If telescope sent us JNow
		if (useJNow(mCoordinateType, mAscomUseDeviceEqCoordType, mEquinox))
		{
			const StelCore* core = StelApp::getInstance().getCore();
			position = core->equinoxEquToJ2000(position,
			  // If telescope does refraction correction, we don't correct for it in Stellarium
			  mDoesRefraction ? StelCore::RefractionOff : StelCore::RefractionOn);
		}

		if (!(qFuzzyCompare(mLastCoord.RA, currentCoords.RA) && qFuzzyCompare(mLastCoord.DEC, currentCoords.DEC)))
		{
			mCurrentTargetPosition = position;
			mLastCoord.RA = currentCoords.RA;
			mLastCoord.DEC = currentCoords.DEC;
		}

		mLastUpdateTime = now;
	}

	mInterpolatedPosition = mInterpolatedPosition * 10 + mCurrentTargetPosition;
	const double lq = mInterpolatedPosition.normSquared();
	if (lq > 0.0)
		mInterpolatedPosition *= (1.0 / std::sqrt(lq));
	else
		mInterpolatedPosition = mCurrentTargetPosition;
}

bool TelescopeClientASCOM::prepareCommunication()
{
	return true;
}

Vec3d TelescopeClientASCOM::getJ2000EquatorialPos(const StelCore*) const
{
	return mInterpolatedPosition;
}

ASCOMDevice::ASCOMCoordinates TelescopeClientASCOM::j2000PosToAscomCoord(const Vec3d& j2000Pos) 
{
	Vec3d position = j2000Pos;
	// If telescope wants JNow
	if (useJNow(mCoordinateType, mAscomUseDeviceEqCoordType, mEquinox))
	{
		const StelCore* core = StelApp::getInstance().getCore();
		position = core->j2000ToEquinoxEqu(j2000Pos,
		  // If telescope does refraction correction, we don't correct for it in Stellarium
		  mDoesRefraction ? StelCore::RefractionOff : StelCore::RefractionOn);
	}

	const double ra_signed = atan2(position[1], position[0]);
	// Workaround for the discrepancy in precision between Windows/Linux/PPC Macs and Intel Macs:
	const double ra = (ra_signed >= 0) ? ra_signed : (ra_signed + 2.0 * M_PI);
	const double dec = atan2(position[2], std::sqrt(position[0] * position[0] + position[1] * position[1]));

	ASCOMDevice::ASCOMCoordinates coord;
	coord.RA = ra * 12.0 / M_PI;
	coord.DEC = dec * 180.0 / M_PI;

	return coord;
}

void TelescopeClientASCOM::telescopeGoto(const Vec3d& j2000Pos, StelObjectP selectObject)
{
	Q_UNUSED(selectObject)

	if (!isConnected()) return;

	if (mAscomDevice->isParked())
	{
		QMessageBox::warning(&StelMainView::getInstance(), "Stellarium",
		  q_("Can't slew a telescope which is parked. Unpark before performing any goto command."));
		return;
	}

	ASCOMDevice::ASCOMCoordinates coords = j2000PosToAscomCoord(j2000Pos);
	mAscomDevice->slewToCoordinates(coords);
}

void TelescopeClientASCOM::telescopeSync(const Vec3d& j2000Pos, StelObjectP selectObject) 
{
	Q_UNUSED(selectObject)
	if (!isConnected()) return;

	ASCOMDevice::ASCOMCoordinates coords = j2000PosToAscomCoord(j2000Pos);
	mAscomDevice->syncToCoordinates(coords);
}

void TelescopeClientASCOM::telescopeAbortSlew() 
{
	if (!isConnected()) return;

	mAscomDevice->abortSlew();
}

bool TelescopeClientASCOM::isConnected() const
{
	return mAscomDevice->isDeviceConnected();
}

bool TelescopeClientASCOM::hasKnownPosition() const
{
	return true;
}

bool TelescopeClientASCOM::useJNow(ASCOMDevice::ASCOMEquatorialCoordinateType coordinateType, bool ascomUseDeviceEqCoordType, TelescopeControl::Equinox equinox)
{
	if (ascomUseDeviceEqCoordType)
	{
		return coordinateType == ASCOMDevice::ASCOMEquatorialCoordinateType::Topocentric
			   // Assume Other as JNow too
			   || coordinateType == ASCOMDevice::ASCOMEquatorialCoordinateType::Other;
	}
	else
	{
		return equinox == TelescopeControl::EquinoxJNow;
	}
}
