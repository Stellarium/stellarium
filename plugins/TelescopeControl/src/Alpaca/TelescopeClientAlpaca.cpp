/*
 * Copyright (C) 2019 Gion Kunz <gion.kunz@gmail.com> (ASCOM)
 * Copyright (C) 2025 Georg Zotti (Alpaca port)
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

#include "TelescopeClientAlpaca.hpp"

#include <QDebug>
#include <QMessageBox>
#include <QRegularExpression>
#include <cmath>
//#include <limits>

#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelMainView.hpp"
#include "StelTranslator.hpp"

TelescopeClientAlpaca::TelescopeClientAlpaca(const QString& name, const QString& params, TelescopeControl::Equinox eq)
	: TelescopeClient(name)
	, mEquinox(eq)
	, mInterpolatedPosition(1, 0, 0)
	, mCurrentTargetPosition(1, 0, 0)
{
	qNAM=StelApp::getInstance().getNetworkAccessManager();
	static const QRegularExpression paramRx("^([^:]*):([^:]*)$");
	QRegularExpressionMatch paramMatch=paramRx.match(params);
	if (paramMatch.hasMatch())
	{
		mAlpacaDeviceId = paramMatch.captured(1).trimmed();
		mAlpacaUseDeviceEqCoordType = paramMatch.captured(2).trimmed() == "true";
	}

	qDebug() << "TelescopeClientAlpaca::TelescopeClientAlpaca with telescope name " << name << " and AlpacaDeviceId " << mAlpacaDeviceId;

	mAlpacaDevice = new AlpacaDevice(this, mAlpacaDeviceId);
	mAlpacaDevice->connect();
	mDoesRefraction = mAlpacaDevice->doesRefraction();
	mCoordinateType = mAlpacaDevice->getEquatorialCoordinateType();
}

TelescopeClientAlpaca::~TelescopeClientAlpaca()
{
	mAlpacaDevice->disconnect();
	delete mAlpacaDevice;
}

void TelescopeClientAlpaca::performCommunication()
{
	qint64 now = getNow();

	if (now - mLastUpdateTime > 1000000)
	{
		AlpacaDevice::AlpacaCoordinates currentCoords = mAlpacaDevice->getCoordinates();

		double longitudeRad = currentCoords.RA * M_PI / 12.0;
		double latitudeRad = currentCoords.DEC * M_PI / 180.0;

		Vec3d position;
		StelUtils::spheToRect(longitudeRad, latitudeRad, position);

		// If telescope sent us JNow
		if (useJNow(mCoordinateType, mAlpacaUseDeviceEqCoordType, mEquinox))
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

bool TelescopeClientAlpaca::prepareCommunication()
{
	return true;
}

Vec3d TelescopeClientAlpaca::getJ2000EquatorialPos(const StelCore*) const
{
	return mInterpolatedPosition;
}

AlpacaDevice::AlpacaCoordinates TelescopeClientAlpaca::j2000PosToAlpacaCoord(const Vec3d& j2000Pos) 
{
	Vec3d position = j2000Pos;
	// If telescope wants JNow
	if (useJNow(mCoordinateType, mAlpacaUseDeviceEqCoordType, mEquinox))
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

	AlpacaDevice::AlpacaCoordinates coord;
	coord.RA = ra * 12.0 / M_PI;
	coord.DEC = dec * 180.0 / M_PI;

	return coord;
}

void TelescopeClientAlpaca::telescopeGoto(const Vec3d& j2000Pos, StelObjectP selectObject)
{
	Q_UNUSED(selectObject)

	if (!isConnected()) return;

	if (mAlpacaDevice->isParked())
	{
		QMessageBox::warning(&StelMainView::getInstance(), "Stellarium",
		  q_("Can't slew a telescope which is parked. Unpark before performing any goto command."));
		return;
	}

	AlpacaDevice::AlpacaCoordinates coords = j2000PosToAlpacaCoord(j2000Pos);
	mAlpacaDevice->slewToCoordinates(coords);
}

void TelescopeClientAlpaca::telescopeSync(const Vec3d& j2000Pos, StelObjectP selectObject) 
{
	Q_UNUSED(selectObject)
	if (!isConnected()) return;

	AlpacaDevice::AlpacaCoordinates coords = j2000PosToAlpacaCoord(j2000Pos);
	mAlpacaDevice->syncToCoordinates(coords);
}

void TelescopeClientAlpaca::telescopeAbortSlew() 
{
	if (!isConnected()) return;

	mAlpacaDevice->abortSlew();
}

bool TelescopeClientAlpaca::isConnected() const
{
	return mAlpacaDevice->isDeviceConnected();
}

bool TelescopeClientAlpaca::hasKnownPosition() const
{
	return true;
}

bool TelescopeClientAlpaca::useJNow(AlpacaDevice::AlpacaEquatorialCoordinateType coordinateType, bool AlpacaUseDeviceEqCoordType, TelescopeControl::Equinox equinox)
{
	if (AlpacaUseDeviceEqCoordType)
	{
		return coordinateType == AlpacaDevice::AlpacaEquatorialCoordinateType::Topocentric
			   // Assume Other as JNow too
			   || coordinateType == AlpacaDevice::AlpacaEquatorialCoordinateType::Other;
	}
	else
	{
		return equinox == TelescopeControl::EquinoxJNow;
	}
}
