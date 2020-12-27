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

#include "indibase/baseclient.h"
#include "indibase/basedevice.h"
#include "indibase/inditelescope.h"

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
	if (!mTelescope)
		return;

	if (!mTelescope->isConnected())
	{
		qDebug() << "Error: Telescope not connected";
		return;
	}

	// Make sure the TRACK member of switch ON_COORD_SET is set
	ISwitchVectorProperty *switchVector = mTelescope->getSwitch("ON_COORD_SET");
	if (!switchVector)
	{
		qDebug() << "Error: unable to find Telescope or ON_COORD_SET switch...";
		return;
	}
	// Note that confusingly there is a SLEW switch member as well that will move but not track.
	// TODO: Figure out if there is to be support for it
	ISwitch *track = IUFindSwitch(switchVector, "TRACK");
	if (track->s == ISS_OFF)
	{
		track->s = ISS_ON;
		sendNewSwitch(switchVector);
	}

	INumberVectorProperty *property = Q_NULLPTR;
	property = mTelescope->getNumber("EQUATORIAL_EOD_COORD");
	if (!property)
	{
		qDebug() << "Error: unable to find Telescope or EQUATORIAL_EOD_COORD property...";
		return;
	}

	property->np[0].value = coords.RA;
	property->np[1].value = coords.DEC;
	sendNewNumber(property);
}

void INDIConnection::syncPosition(INDIConnection::Coordinates coords)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope)
		return;

	if (!mTelescope->isConnected())
	{
		qDebug() << "Error: Telescope not connected";
		return;
	}

	// Make sure the SYNC member of switch ON_COORD_SET is set
	ISwitchVectorProperty *switchVector = mTelescope->getSwitch("ON_COORD_SET");
	if (!switchVector)
	{
		qDebug() << "Error: unable to find Telescope or ON_COORD_SET switch...";
		return;
	}

	ISwitch *track = IUFindSwitch(switchVector, "TRACK");
	ISwitch *slew = IUFindSwitch(switchVector, "SLEW");
	ISwitch *sync = IUFindSwitch(switchVector, "SYNC");
	track->s = ISS_OFF;
	slew->s = ISS_OFF;
	sync->s = ISS_ON;
	sendNewSwitch(switchVector);

	INumberVectorProperty *property = Q_NULLPTR;
	property = mTelescope->getNumber("EQUATORIAL_EOD_COORD");
	if (!property)
	{
		qDebug() << "Error: unable to find Telescope or EQUATORIAL_EOD_COORD property...";
		return;
	}

	property->np[0].value = coords.RA;
	property->np[1].value = coords.DEC;
	sendNewNumber(property);

	// And now unset SYNC switch member to revert to default state/behavior
	track->s = ISS_ON;
	slew->s = ISS_OFF;
	sync->s = ISS_OFF;
	sendNewSwitch(switchVector);
}

bool INDIConnection::isDeviceConnected() const
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope)
		return false;

	return mTelescope->isConnected();
}

const QStringList INDIConnection::devices() const
{
	std::lock_guard<std::mutex> lock(mMutex);
	return mDevices;
}

void INDIConnection::unParkTelescope()
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope || !mTelescope->isConnected())
		return;

	ISwitchVectorProperty *switchVector = mTelescope->getSwitch("TELESCOPE_PARK");
	if (!switchVector)
	{
		qDebug() << "Error: unable to find Telescope or TELESCOPE_PARK switch...";
		return;
	}

	ISwitch *park = IUFindSwitch(switchVector, "PARK");
	if (park->s == ISS_ON)
	{
		park->s = ISS_OFF;
		sendNewSwitch(switchVector);
	}

	// The telescope will work without running command below, but I use it to avoid undefined state for parking property.
	ISwitch *unpark = IUFindSwitch(switchVector, "UNPARK");
	if (unpark->s == ISS_OFF)
	{
		unpark->s = ISS_ON;
		sendNewSwitch(switchVector);
	}
}

/*
 * Unused at the moment
 * TODO: Enable method after changes in the GUI
void INDIConnection::parkTelescope()
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope || !mTelescope->isConnected())
		return;

	ISwitchVectorProperty *switchVector = mTelescope->getSwitch("TELESCOPE_PARK");
	if (!switchVector)
	{
		qDebug() << "Error: unable to find Telescope or TELESCOPE_PARK switch...";
		return;
	}

	ISwitch *park = IUFindSwitch(switchVector, "PARK");
	if (park->s == ISS_OFF)
	{
		park->s = ISS_ON;
		sendNewSwitch(switchVector);
	}

	ISwitch *unpark = IUFindSwitch(switchVector, "UNPARK");
	if (unpark->s == ISS_ON)
	{
		unpark->s = ISS_OFF;
		sendNewSwitch(switchVector);
	}
}
*/

void INDIConnection::moveNorth(int speed)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope || !mTelescope->isConnected())
		return;

	ISwitchVectorProperty *switchVector = mTelescope->getSwitch("TELESCOPE_MOTION_NS");
	if (!switchVector)
	{
		qDebug() << "Error: unable to find Telescope or TELESCOPE_MOTION_NS switch...";
		return;
	}

	ISwitch *motion = IUFindSwitch(switchVector, "MOTION_NORTH");

	if (speed == SLEW_STOP)
		motion->s = ISS_OFF;
	else
	{
		setSpeed(speed);
		motion->s = ISS_ON;
	}

	sendNewSwitch(switchVector);
}

void INDIConnection::moveEast(int speed)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope || !mTelescope->isConnected())
		return;

	ISwitchVectorProperty *switchVector = mTelescope->getSwitch("TELESCOPE_MOTION_WE");
	if (!switchVector)
	{
		qDebug() << "Error: unable to find Telescope or TELESCOPE_MOTION_WE switch...";
		return;
	}

	ISwitch *motion = IUFindSwitch(switchVector, "MOTION_EAST");

	if (speed == SLEW_STOP)
		motion->s = ISS_OFF;
	else
	{
		setSpeed(speed);
		motion->s = ISS_ON;
	}

	sendNewSwitch(switchVector);
}

void INDIConnection::moveSouth(int speed)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope || !mTelescope->isConnected())
		return;

	ISwitchVectorProperty *switchVector = mTelescope->getSwitch("TELESCOPE_MOTION_NS");
	if (!switchVector)
	{
		qDebug() << "Error: unable to find Telescope or TELESCOPE_MOTION_NS switch...";
		return;
	}

	ISwitch *motion = IUFindSwitch(switchVector, "MOTION_SOUTH");

	if (speed == SLEW_STOP)
		motion->s = ISS_OFF;
	else
	{
		setSpeed(speed);
		motion->s = ISS_ON;
	}

	sendNewSwitch(switchVector);
}

void INDIConnection::moveWest(int speed)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope || !mTelescope->isConnected())
		return;

	ISwitchVectorProperty *switchVector = mTelescope->getSwitch("TELESCOPE_MOTION_WE");
	if (!switchVector)
	{
		qDebug() << "Error: unable to find Telescope or TELESCOPE_MOTION_WE switch...";
		return;
	}

	ISwitch *motion = IUFindSwitch(switchVector, "MOTION_WEST");

	if (speed == SLEW_STOP)
		motion->s = ISS_OFF;
	else
	{
		setSpeed(speed);
		motion->s = ISS_ON;
	}

	sendNewSwitch(switchVector);
}

void INDIConnection::setSpeed(int speed)
{
	ISwitchVectorProperty *slewRateSP = mTelescope->getSwitch("TELESCOPE_SLEW_RATE");

	if (!slewRateSP || speed < 0 || speed > slewRateSP->nsp)
		return;

	IUResetSwitch(slewRateSP);
	slewRateSP->sp[speed].s = ISS_ON;
	sendNewSwitch(slewRateSP);
}

void INDIConnection::newDevice(INDI::BaseDevice *dp)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!dp)
		return;

	QString name(dp->getDeviceName());

	qDebug() << "INDIConnection::newDevice| New Device... " << name;

	mDevices.append(name);
	mTelescope = dp;

	emit newDeviceReceived(name);
}

void INDIConnection::removeDevice(INDI::BaseDevice *dp)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!dp)
		return;

	QString name(dp->getDeviceName());
	int index = mDevices.indexOf(name);
	if (index != -1)
		mDevices.removeAt(index);

	if (mTelescope == dp)
		mTelescope = Q_NULLPTR;

	emit removeDeviceReceived(name);
}

void INDIConnection::newProperty(INDI::Property *property)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (mTelescope != property->getBaseDevice())
		return;

	QString name(property->getName());

	qDebug() << "INDIConnection::newProperty| " << name;

	if (name == "EQUATORIAL_EOD_COORD")
	{
		mCoordinates.RA = property->getNumber()->np[0].value;
		mCoordinates.DEC = property->getNumber()->np[1].value;
	}

	if (!mTelescope->isConnected())
	{
		connectDevice(mTelescope->getDeviceName());
		if (mTelescope->isConnected())
			qDebug() << "connected\n";
	}
}

void INDIConnection::removeProperty(INDI::Property *property)
{
	Q_UNUSED(property)
}

void INDIConnection::newBLOB(IBLOB *bp)
{
	Q_UNUSED(bp)
}

void INDIConnection::newSwitch(ISwitchVectorProperty *svp)
{
	std::lock_guard<std::mutex> lock(mMutex);
	QString name(svp->name);
	if (name == "TELESCOPE_SLEW_RATE")
	{
		int speed = IUFindOnSwitchIndex(svp);
		emit speedChanged(speed);
	}
}

void INDIConnection::newNumber(INumberVectorProperty *nvp)
{
	std::lock_guard<std::mutex> lock(mMutex);

	QString name(nvp->name);
	if (name == "EQUATORIAL_EOD_COORD")
	{
		mCoordinates.RA = nvp->np[0].value;
		mCoordinates.DEC = nvp->np[1].value;
	}
}

void INDIConnection::newText(ITextVectorProperty *tvp)
{
	Q_UNUSED(tvp)
}

void INDIConnection::newLight(ILightVectorProperty *lvp)
{
	Q_UNUSED(lvp)
}

void INDIConnection::newMessage(INDI::BaseDevice *dp, int messageID)
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
