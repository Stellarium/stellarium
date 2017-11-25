#include "INDIConnection.hpp"

INDIConnection::INDIConnection()
{

}


void INDIConnection::newDevice(INDI::BaseDevice *dp)
{
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
