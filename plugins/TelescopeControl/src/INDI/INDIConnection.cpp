#include "INDIConnection.hpp"

#include <QDebug>
#include <QString>
#include <chrono>
#include <thread>
#include <limits>
#include <cmath>

#include "indibase/baseclient.h"
#include "indibase/basedevice.h"

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
}

void INDIConnection::newNumber(INumberVectorProperty *nvp)
{
	// @TODO filter for mTelescope

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
