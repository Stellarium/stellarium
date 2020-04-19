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
#include <cmath>
#include <limits>

#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"

bool useJNow(ASCOMDevice::ASCOMEquatorialCoordinateType coordinateType, bool mAscomUseDeviceEqCoordType, Equinox mEquinox)
{
	if (mAscomUseDeviceEqCoordType)
	{
		return coordinateType == ASCOMDevice::ASCOMEquatorialCoordinateType::Topocentric
			   // Assume Other as JNow too
			   || coordinateType == ASCOMDevice::ASCOMEquatorialCoordinateType::Other;
	}
	else
	{
		return mEquinox == EquinoxJNow;
	}
}

bool areSimilar(double a, double b)
{
	return std::abs(a - b) < std::numeric_limits<double>::epsilon();
}

TelescopeClientASCOM::TelescopeClientASCOM(const QString& name, const QString& params, Equinox eq)
	: TelescopeClient(name)
	, mEquinox(eq)
{
	QRegExp paramRx("^([^:]*):([^:]*)$");
	if (paramRx.exactMatch(params))
	{
		mAscomDeviceId = paramRx.cap(1).trimmed();
		mAscomUseDeviceEqCoordType = paramRx.cap(2).trimmed() == "true";
	}

	qDebug() << "TelescopeClientASCOM::TelescopeClientASCOM with telescope name " << name << " and ascomDeviceId " << mAscomDeviceId;

	mAscomDevice = new ASCOMDevice(this, mAscomDeviceId);
	mAscomDevice->connect();
	mDoesRefraction = mAscomDevice->doesRefraction();
	mCoordinateType = mAscomDevice->getEquatorialCoordinateType();

	mCurrentTargetPosition[0] = mInterpolatedPosition[0] = 1;
	mCurrentTargetPosition[1] = mInterpolatedPosition[1] = 0;
	mCurrentTargetPosition[2] = mInterpolatedPosition[2] = 0;
}

TelescopeClientASCOM::~TelescopeClientASCOM()
{
	mAscomDevice->disconnect();
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

		if (!(areSimilar(mLastCoord.RA, currentCoords.RA) && areSimilar(mLastCoord.DEC, currentCoords.DEC)))
		{
			mCurrentTargetPosition = position;
			mLastCoord.RA = currentCoords.RA;
			mLastCoord.DEC = currentCoords.DEC;
		}

		mLastUpdateTime = now;
	}

	mInterpolatedPosition = mInterpolatedPosition * 10 + mCurrentTargetPosition;
	const double lq = mInterpolatedPosition.lengthSquared();
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
		  // If telescope dones refraction correction, we don't correct for it in Stellarium
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
	Q_UNUSED(selectObject);

	if (!isConnected()) return;

	if (mAscomDevice->isParked())
	{
		QMessageBox::warning(Q_NULLPTR, "Stellarium",
		  q_("Can't slew a telescope which is parked. Unpark before performing any goto command."));
		return;
	}

	ASCOMDevice::ASCOMCoordinates coords = j2000PosToAscomCoord(j2000Pos);
	mAscomDevice->slewToCoordinates(coords);
}

void TelescopeClientASCOM::telescopeSync(const Vec3d& j2000Pos, StelObjectP selectObject) 
{
	Q_UNUSED(selectObject);
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

void TelescopeClientASCOM::move(double angle, double speed)
{
	Q_UNUSED(angle);
	Q_UNUSED(speed);
}
