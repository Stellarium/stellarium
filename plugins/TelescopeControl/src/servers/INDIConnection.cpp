#include "INDIConnection.hpp"

#include <QDebug>
#include <QString>

#include "libindi/basedevice.h"

INDIConnection::INDIConnection()
{
}

Vec3d INDIConnection::positionJNow() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mPosition;
}

void INDIConnection::setPositionJNow(Vec3d position)
{
    std::lock_guard<std::mutex> lock(mMutex);

    INumberVectorProperty *coord = nullptr;
    coord = mTelescope->getNumber("EQUATORIAL_EOD_COORD");
    if (!coord)
    {
        IDLog("Error: unable to find Telescopeor EQUATORIAL_EOD_COORD property...\n");
        return;
    }

    coord->np[0].value = position[0];
    coord->np[1].value = position[1];
    sendNewNumber(coord);
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
        qDebug() << nvp->np[0].value << nvp->np[1].value;
        mPosition[0] = nvp->np[0].value;
        mPosition[1] = nvp->np[1].value;
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
