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

	INumberVectorProperty *property = nullptr;
	property = mTelescope->getNumber("EQUATORIAL_EOD_COORD");
	if (!property)
	{
		qDebug() << "Error: unable to find Telescopeor EQUATORIAL_EOD_COORD property...";
		return;
	}

	property->np[0].value = coords.RA;
	property->np[1].value = coords.DEC;
	sendNewNumber(property);
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

void INDIConnection::moveNorth(int speed)
{
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mTelescope || !mTelescope->isConnected())
		return;

	ISwitchVectorProperty *switchVector = mTelescope->getSwitch("TELESCOPE_MOTION_NS");
	if (!switchVector)
	{
		qDebug() << "Error: unable to find Telescopeor TELESCOPE_MOTION_NS switch...";
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
		qDebug() << "Error: unable to find Telescopeor TELESCOPE_MOTION_WE switch...";
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
		qDebug() << "Error: unable to find Telescopeor TELESCOPE_MOTION_NS switch...";
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
		qDebug() << "Error: unable to find Telescopeor TELESCOPE_MOTION_WE switch...";
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

	qDebug() << "INDIConnection::newDevice| %s Device... " << name;

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
		mTelescope = nullptr;

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
}

void INDIConnection::newBLOB(IBLOB *bp)
{
}

void INDIConnection::newSwitch(ISwitchVectorProperty *svp)
{
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
}

void INDIConnection::newLight(ILightVectorProperty *lvp)
{
}

void INDIConnection::newMessage(INDI::BaseDevice *dp, int messageID)
{
}

void INDIConnection::serverConnected()
{
	emit serverConnectedReceived();
}

void INDIConnection::serverDisconnected(int exit_code)
{
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
