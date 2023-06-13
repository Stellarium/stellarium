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

#include "INDIConnection.hpp"

#include <QDebug>
#include <QString>
#include <chrono>
#include <thread>
#include <limits>
#include <cmath>

#include "libindi/baseclient.h"
#include "libindi/basedevice.h"
#include "libindi/inditelescope.h"

const int INDIConnection::SLEW_STOP = INDI::Telescope::SLEW_GUIDE - 1;

INDIConnection::INDIConnection(QObject *parent) : QObject(parent)
{
}

INDIConnection::Coordinates INDIConnection::position() const
{
	std::lock_guard<std::mutex> lock(mMutex);
	return mCoordinates;
}

void INDIConnection::setPosition(INDIConnection::Coordinates coords)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope.isValid())
		return;

	if (!mTelescope.isConnected())
	{
		qDebug() << "Error: Telescope not connected";
		return;
	}

	// Make sure the TRACK member of switch ON_COORD_SET is set
	auto switchVector = mTelescope.getSwitch("ON_COORD_SET");
	if (!switchVector.isValid())
	{
		qDebug() << "Error: unable to find Telescope or ON_COORD_SET switch...";
		return;
	}
	// Note that confusingly there is a SLEW switch member as well that will move but not track.
	// TODO: Figure out if there is to be support for it
	auto track = switchVector.findWidgetByName("TRACK");
	if (track->s == ISS_OFF)
	{
		track->setState(ISS_ON);
		sendNewSwitch(switchVector);
	}

	auto property = mTelescope.getNumber("EQUATORIAL_EOD_COORD");
	if (!property.isValid())
	{
		qDebug() << "Error: unable to find Telescope or EQUATORIAL_EOD_COORD property...";
		return;
	}

	property[0].setValue(coords.RA);
	property[1].setValue(coords.DEC);
	sendNewNumber(property);
}

void INDIConnection::syncPosition(INDIConnection::Coordinates coords)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope.isValid())
		return;

	if (!mTelescope.isConnected())
	{
		qDebug() << "Error: Telescope not connected";
		return;
	}

	// Make sure the SYNC member of switch ON_COORD_SET is set
	auto switchVector = mTelescope.getSwitch("ON_COORD_SET");
	if (!switchVector.isValid())
	{
		qDebug() << "Error: unable to find Telescope or ON_COORD_SET switch...";
		return;
	}

	auto track = switchVector.findWidgetByName("TRACK");
	auto slew = switchVector.findWidgetByName("SLEW");
	auto sync = switchVector.findWidgetByName("SYNC");
	track->setState(ISS_OFF);
	slew->setState(ISS_OFF);
	sync->setState(ISS_ON);
	sendNewSwitch(switchVector);

	auto property = mTelescope.getNumber("EQUATORIAL_EOD_COORD");
	if (!property.isValid())
	{
		qDebug() << "Error: unable to find Telescope or EQUATORIAL_EOD_COORD property...";
		return;
	}

	property[0].setValue(coords.RA);
	property[1].setValue(coords.DEC);
	sendNewNumber(property);

	// And now unset SYNC switch member to revert to default state/behavior
	track->setState(ISS_ON);
	slew->setState(ISS_OFF);
	sync->setState(ISS_OFF);
	sendNewSwitch(switchVector);
}

bool INDIConnection::isDeviceConnected() const
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope.isValid())
		return false;

	return mTelescope.isConnected();
}

const QStringList INDIConnection::devices() const
{
	std::lock_guard<std::mutex> lock(mMutex);
	return mDevices;
}

void INDIConnection::unParkTelescope()
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope.isValid() || !mTelescope.isConnected())
		return;

	auto switchVector = mTelescope.getSwitch("TELESCOPE_PARK");
	if (!switchVector.isValid())
	{
		qDebug() << "Error: unable to find Telescope or TELESCOPE_PARK switch...";
		return;
	}

	auto park = switchVector.findWidgetByName("PARK");
	if (park->s == ISS_ON)
	{
		park->setState(ISS_OFF);
		sendNewSwitch(switchVector);
	}

	// The telescope will work without running command below, but I use it to avoid undefined state for parking property.
	auto unpark = switchVector.findWidgetByName("UNPARK");
	if (unpark->s == ISS_OFF)
	{
		unpark->setState(ISS_ON);
		sendNewSwitch(switchVector);
	}
}

/*
 * Unused at the moment
 * TODO: Enable method after changes in the GUI
void INDIConnection::parkTelescope()
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope.isValid() || !mTelescope.isConnected())
		return;

	auto switchVector = mTelescope.getSwitch("TELESCOPE_PARK");
	if (!switchVector.isValid())
	{
		qDebug() << "Error: unable to find Telescope or TELESCOPE_PARK switch...";
		return;
	}

	auto park = switchVector.findWidgetByName("PARK");
	if (park->s == ISS_OFF)
	{
		park->setState(ISS_ON);
		sendNewSwitch(switchVector);
	}

	auto unpark = switchVector.findWidgetByName("UNPARK");
	if (unpark->s == ISS_ON)
	{
		unpark->setState(ISS_OFF);
		sendNewSwitch(switchVector);
	}
}
*/

void INDIConnection::moveNorth(int speed)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope.isValid() || !mTelescope.isConnected())
		return;

	auto switchVector = mTelescope.getSwitch("TELESCOPE_MOTION_NS");
	if (!switchVector.isValid())
	{
		qDebug() << "Error: unable to find Telescope or TELESCOPE_MOTION_NS switch...";
		return;
	}

	auto motion = switchVector.findWidgetByName("MOTION_NORTH");

	if (speed == SLEW_STOP)
		motion->setState(ISS_OFF);
	else
	{
		setSpeed(speed);
		motion->setState(ISS_ON);
	}

	sendNewSwitch(switchVector);
}

void INDIConnection::moveEast(int speed)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope.isValid() || !mTelescope.isConnected())
		return;

	auto switchVector = mTelescope.getSwitch("TELESCOPE_MOTION_WE");
	if (!switchVector.isValid())
	{
		qDebug() << "Error: unable to find Telescope or TELESCOPE_MOTION_WE switch...";
		return;
	}

	auto motion = switchVector.findWidgetByName("MOTION_EAST");

	if (speed == SLEW_STOP)
		motion->setState(ISS_OFF);
	else
	{
		setSpeed(speed);
		motion->setState(ISS_ON);
	}

	sendNewSwitch(switchVector);
}

void INDIConnection::moveSouth(int speed)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope.isValid() || !mTelescope.isConnected())
		return;

	auto switchVector = mTelescope.getSwitch("TELESCOPE_MOTION_NS");
	if (!switchVector.isValid())
	{
		qDebug() << "Error: unable to find Telescope or TELESCOPE_MOTION_NS switch...";
		return;
	}

	auto motion = switchVector.findWidgetByName("MOTION_SOUTH");

	if (speed == SLEW_STOP)
		motion->setState(ISS_OFF);
	else
	{
		setSpeed(speed);
		motion->setState(ISS_ON);
	}

	sendNewSwitch(switchVector);
}

void INDIConnection::moveWest(int speed)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope.isValid() || !mTelescope.isConnected())
		return;

	auto switchVector = mTelescope.getSwitch("TELESCOPE_MOTION_WE");
	if (!switchVector.isValid())
	{
		qDebug() << "Error: unable to find Telescope or TELESCOPE_MOTION_WE switch...";
		return;
	}

	auto motion = switchVector.findWidgetByName("MOTION_WEST");

	if (speed == SLEW_STOP)
		motion->setState(ISS_OFF);
	else
	{
		setSpeed(speed);
		motion->setState(ISS_ON);
	}

	sendNewSwitch(switchVector);
}

void INDIConnection::setSpeed(int speed)
{
	auto slewRateSP = mTelescope.getSwitch("TELESCOPE_SLEW_RATE");

	if (!slewRateSP.isValid() || speed < 0 ||
			static_cast<std::size_t>(speed) > slewRateSP.count())
		return;

	slewRateSP.reset();
	slewRateSP[speed].setState(ISS_ON);
	sendNewSwitch(slewRateSP);
}

void INDIConnection::newDevice(INDI::BaseDevice dp)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!dp)
		return;

	QString name(dp.getDeviceName());

	qDebug() << "INDIConnection::newDevice| New Device... " << name;

	mDevices.append(name);
	mTelescope = dp;

	emit newDeviceReceived(name);
}

void INDIConnection::removeDevice(INDI::BaseDevice dp)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!dp)
		return;

	QString name(dp.getDeviceName());
	int index = mDevices.indexOf(name);
	if (index != -1)
		mDevices.removeAt(index);

	if (mTelescope.isDeviceNameMatch(dp.getDeviceName()))
		mTelescope.detach();

	emit removeDeviceReceived(name);
}

void INDIConnection::newProperty(INDI::Property property)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope.isDeviceNameMatch(
				property.getBaseDevice().getDeviceName()))
		return;

	QString name(property.getName());

	qDebug() << "INDIConnection::newProperty| " << name;

	if (name == "EQUATORIAL_EOD_COORD")
	{
		mCoordinates.RA = property.getNumber()->np[0].value;
		mCoordinates.DEC = property.getNumber()->np[1].value;
	}

	if (!mTelescope.isConnected())
	{
		connectDevice(mTelescope.getDeviceName());
		if (mTelescope.isConnected())
			qDebug() << "connected\n";
	}
}

void INDIConnection::removeProperty(INDI::Property property)
{
	Q_UNUSED(property)
}

void INDIConnection::newMessage(INDI::BaseDevice dp, int messageID)
{
	Q_UNUSED(dp)
	Q_UNUSED(messageID)
}

void INDIConnection::serverConnected()
{
	std::lock_guard<std::mutex> lock(mMutex);
	emit serverConnectedReceived();
}

void INDIConnection::serverDisconnected(int exit_code)
{
	std::lock_guard<std::mutex> lock(mMutex);
	mDevices.clear();
	emit serverDisconnectedReceived(exit_code);
}

bool INDIConnection::Coordinates::operator==(const INDIConnection::Coordinates &other) const
{
	if (std::abs(RA - other.RA) > std::numeric_limits<double>::epsilon()) return false;
	if (std::abs(DEC - other.DEC) > std::numeric_limits<double>::epsilon()) return false;
	return true;
}

bool INDIConnection::Coordinates::operator!=(const INDIConnection::Coordinates &other) const
{
	return !(*this == other);
}
