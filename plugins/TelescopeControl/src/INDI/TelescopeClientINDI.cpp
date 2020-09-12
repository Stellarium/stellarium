/*
 * Copyright (C) 2017 Alessandro Siniscalchi <asiniscalchi@gmail.com>
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

#include "TelescopeClientINDI.hpp"

#include <QDebug>
#include <cmath>
#include <limits>

#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "indibase/inditelescope.h"
#include "INDIControlWidget.hpp"

TelescopeClientINDI::TelescopeClientINDI(const QString &name, const QString &params):
	TelescopeClient(name)
{
	qDebug() << "TelescopeClientINDI::TelescopeClientINDI";

	QRegExp paramRx("^([^:]*):(\\d+):([^:]*)$");
	QString host;
	int port = 0;
	if (paramRx.exactMatch(params))
	{
		host = paramRx.cap(1).trimmed();
		port = paramRx.cap(2).toInt();
		mDevice = paramRx.cap(3).trimmed();
	}

	mConnection.setServer(host.toStdString().c_str(), port);
	mConnection.watchDevice(mDevice.toStdString().c_str());
	mConnection.connectServer();
}

TelescopeClientINDI::~TelescopeClientINDI()
{
	mConnection.disconnectServer();
}

Vec3d TelescopeClientINDI::getJ2000EquatorialPos(const StelCore*) const
{
	INDIConnection::Coordinates positionJNow = mConnection.position();

	double longitudeRad = positionJNow.RA * M_PI / 12.0;
	double latitudeRad = positionJNow.DEC * M_PI / 180.0;

	Vec3d posJNow;
	StelUtils::spheToRect(longitudeRad, latitudeRad, posJNow);
	const StelCore* core = StelApp::getInstance().getCore();
	Vec3d posJ2000 = core->equinoxEquToJ2000(posJNow, StelCore::RefractionOff);
	return posJ2000;
}

void TelescopeClientINDI::telescopeGoto(const Vec3d &positionJ2000, StelObjectP selectObject)
{
	Q_UNUSED(selectObject)
	const StelCore* core = StelApp::getInstance().getCore();
	Vec3d posRectJNow = core->j2000ToEquinoxEqu(positionJ2000, StelCore::RefractionOff);
	Vec3d posJ2000;
	StelUtils::rectToSphe(&posJ2000[0], &posJ2000[1], posRectJNow);

	if (posJ2000[0] < 0.0)
		posJ2000[0] += 2*M_PI;

	double longitudeRad = posJ2000[0] * 12.0 / M_PI;
	double latitudeRad = posJ2000[1] * 180.0 / M_PI;


	INDIConnection::Coordinates positionJNow = mConnection.position();
	positionJNow.RA = longitudeRad;
	positionJNow.DEC = latitudeRad;

	// unpark telescope before slewing!
	// TODO: Add commands and buttons for park/unpark telescope for all telescopes
	mConnection.unParkTelescope();
	mConnection.setPosition(positionJNow);
}

void TelescopeClientINDI::telescopeSync(const Vec3d &positionJ2000, StelObjectP selectObject)
{
	Q_UNUSED(selectObject)
	const StelCore* core = StelApp::getInstance().getCore();
	Vec3d posRectJNow = core->j2000ToEquinoxEqu(positionJ2000, StelCore::RefractionOff);
	Vec3d posJ2000;
	StelUtils::rectToSphe(&posJ2000[0], &posJ2000[1], posRectJNow);

	if (posJ2000[0] < 0.0)
		posJ2000[0] += 2*M_PI;

	double longitudeRad = posJ2000[0] * 12.0 / M_PI;
	double latitudeRad = posJ2000[1] * 180.0 / M_PI;


	INDIConnection::Coordinates positionJNow = mConnection.position();
	positionJNow.RA = longitudeRad;
	positionJNow.DEC = latitudeRad;

	// unpark telescope before slewing!
	// TODO: Add commands and buttons for park/unpark telescope for all telescopes
	mConnection.unParkTelescope();
	mConnection.syncPosition(positionJNow);
}

bool TelescopeClientINDI::isConnected() const
{
	return mConnection.isDeviceConnected();
}

bool TelescopeClientINDI::hasKnownPosition() const
{
	return mConnection.isDeviceConnected();
}

int TelescopeClientINDI::toINDISpeed(double speed) const
{
	speed = std::abs(speed);

	if (speed < std::numeric_limits<double>::epsilon())
		return INDIConnection::SLEW_STOP;
	else if (speed <= 0.25)
		return INDI::Telescope::SLEW_GUIDE;
	else if (speed <= 0.5)
		return INDI::Telescope::SLEW_CENTERING;
	else if (speed <= 0.75)
		return INDI::Telescope::SLEW_FIND;
	else
		return INDI::Telescope::SLEW_MAX;
}

void TelescopeClientINDI::move(double angle, double speed)
{
	if (angle < 0.0 || angle >= 360.0)
	{
		qWarning() << "TelescopeClientINDI::move angle " << angle << " out of range [0,360)";
		return;
	}

	if (speed < 0.0 || speed > 1.0)
	{
		qWarning() << "TelescopeClientINDI::move speed " << speed << "out of range [0,1]";
		return;
	}

	double rad = angle * M_PI / 180.0;
	double vEst = speed * std::sin(rad);
	double vNorth = speed * std::cos(rad);

	int indiSpeedE = toINDISpeed(vEst);
	int indiSpeedN = toINDISpeed(vNorth);

	if (angle < 90)
	{
		mConnection.moveNorth(indiSpeedN);
		mConnection.moveEast(indiSpeedE);
		mConnection.moveSouth(INDIConnection::SLEW_STOP);
		mConnection.moveWest(INDIConnection::SLEW_STOP);
	}
	else if (angle < 180)
	{
		mConnection.moveNorth(INDIConnection::SLEW_STOP);
		mConnection.moveEast(indiSpeedE);
		mConnection.moveSouth(indiSpeedN);
		mConnection.moveWest(INDIConnection::SLEW_STOP);
	}
	else if (angle < 270)
	{
		mConnection.moveNorth(INDIConnection::SLEW_STOP);
		mConnection.moveEast(INDIConnection::SLEW_STOP);
		mConnection.moveSouth(indiSpeedN);
		mConnection.moveWest(indiSpeedE);
	}
	else
	{
		mConnection.moveNorth(indiSpeedN);
		mConnection.moveEast(INDIConnection::SLEW_STOP);
		mConnection.moveSouth(INDIConnection::SLEW_STOP);
		mConnection.moveWest(indiSpeedE);
	}
}


QWidget *TelescopeClientINDI::createControlWidget(QSharedPointer<TelescopeClient> telescope, QWidget *parent) const
{
	return new INDIControlWidget(telescope, parent);
}
