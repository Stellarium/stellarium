#include "TelescopeClientINDI.hpp"

#include <QDebug>

TelescopeClientINDI::TelescopeClientINDI(const QString &name):
    TelescopeClient(name)
{
    qDebug() << "TelescopeClientINDI::TelescopeClientINDI";
}

TelescopeClientINDI::~TelescopeClientINDI()
{
    qDebug() << "TelescopeClientINDI::~TelescopeClientINDI";
}

Vec3d TelescopeClientINDI::getJ2000EquatorialPos(const StelCore *core) const
{
    qDebug() << "TelescopeClientINDI::getJ2000EquatorialPos";
    return Vec3d();
}

void TelescopeClientINDI::telescopeGoto(const Vec3d &j2000Pos, StelObjectP selectObject)
{
    qDebug() << "TelescopeClientINDI::telescopeGoto";
}

bool TelescopeClientINDI::isConnected() const
{
    qDebug() << "TelescopeClientINDI::isConnected";
    return false;
}

bool TelescopeClientINDI::hasKnownPosition() const
{
    qDebug() << "TelescopeClientINDI::hasKnownPosition";
    return false;
}


void TelescopeClientINDI::newDevice(INDI::BaseDevice *dp)
{
}

void TelescopeClientINDI::removeDevice(INDI::BaseDevice *dp)
{
}

void TelescopeClientINDI::newProperty(INDI::Property *property)
{
}

void TelescopeClientINDI::removeProperty(INDI::Property *property)
{
}

void TelescopeClientINDI::newBLOB(IBLOB *bp)
{
}

void TelescopeClientINDI::newSwitch(ISwitchVectorProperty *svp)
{
}

void TelescopeClientINDI::newNumber(INumberVectorProperty *nvp)
{
}

void TelescopeClientINDI::newText(ITextVectorProperty *tvp)
{
}

void TelescopeClientINDI::newLight(ILightVectorProperty *lvp)
{
}

void TelescopeClientINDI::newMessage(INDI::BaseDevice *dp, int messageID)
{
}

void TelescopeClientINDI::serverConnected()
{
}

void TelescopeClientINDI::serverDisconnected(int exit_code)
{
}
