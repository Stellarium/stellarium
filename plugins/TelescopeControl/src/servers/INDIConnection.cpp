#include "INDIConnection.hpp"

#include <QDebug>
#include <QString>

#include "libindi/basedevice.h"

INDIConnection::INDIConnection()
{
}

double INDIConnection::declination() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mDeclination;
}

double INDIConnection::rightAscension() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mRightAscension;
}

void INDIConnection::newDevice(INDI::BaseDevice *dp)
{
    IDLog("INDIConnection::newDevice| %s Device...\n", dp->getDeviceName());
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
        mDeclination = nvp->np[0].value;
        mRightAscension = nvp->np[1].value;
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
