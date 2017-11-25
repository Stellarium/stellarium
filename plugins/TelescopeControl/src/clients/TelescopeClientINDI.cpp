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

