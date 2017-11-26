#include "INDIConnection.hpp"

#include <QDebug>
#include <QString>
#include <chrono>
#include <thread>

#include "libindi/basedevice.h"

INDIConnection::INDIConnection()
{
}

INDIConnection::Coordinates INDIConnection::position() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mCoordinatesJNow;
}

void INDIConnection::setPosition(INDIConnection::Coordinates coords)
{
    if (!mTelescope->isConnected())
    {
        IDLog("Error: Telescope not connected");
        return;
    }

    std::lock_guard<std::mutex> lock(mMutex);
    INumberVectorProperty *property = nullptr;
    property = mTelescope->getNumber("EQUATORIAL_EOD_COORD");
    if (!property)
    {
        IDLog("Error: unable to find Telescopeor EQUATORIAL_EOD_COORD property...\n");
        return;
    }

    property->np[0].value = coords.RA;
    property->np[1].value = coords.DEC;
    sendNewNumber(property);
}

bool INDIConnection::isConnected() const
{
    if (!mTelescope)
        return false;

    return mTelescope->isConnected();
}

void INDIConnection::newDevice(INDI::BaseDevice *dp)
{
    std::lock_guard<std::mutex> lock(mMutex);
    IDLog("INDIConnection::newDevice| %s Device...\n", dp->getDeviceName());
    mTelescope = dp;
}

void INDIConnection::removeDevice(INDI::BaseDevice *dp)
{
}

void INDIConnection::newProperty(INDI::Property *property)
{
    IDLog("INDIConnection::newProperty| %s\n", property->getName());

    QString name(property->getName());

    if (name == "EQUATORIAL_EOD_COORD")
    {
        mCoordinatesJNow.RA = property->getNumber()->np[0].value;
        mCoordinatesJNow.DEC = property->getNumber()->np[1].value;
    }

    if (!mTelescope->isConnected())
    {
        connectDevice(mTelescope->getDeviceName());
        if (mTelescope->isConnected())
            IDLog("connected\n");
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
    std::lock_guard<std::mutex> lock(mMutex);

    QString name(nvp->name);
    if (name == "EQUATORIAL_EOD_COORD")
    {
        mCoordinatesJNow.RA = nvp->np[0].value;
        mCoordinatesJNow.DEC = nvp->np[1].value;
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
}

void INDIConnection::serverDisconnected(int exit_code)
{
}
