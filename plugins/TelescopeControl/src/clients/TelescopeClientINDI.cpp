#include "TelescopeClientINDI.hpp"

#include <QDebug>

#include "StelCore.hpp"

TelescopeClientINDI::TelescopeClientINDI(const QString &name):
    TelescopeClient(name)
{
    qDebug() << "TelescopeClientINDI::TelescopeClientINDI";
    connection.setServer("localhost", 7624);
    connection.connectServer();
}

TelescopeClientINDI::~TelescopeClientINDI()
{
    qDebug() << "TelescopeClientINDI::~TelescopeClientINDI";
}

Vec3d TelescopeClientINDI::getJ2000EquatorialPos(const StelCore*) const
{
    qDebug() << "TelescopeClientINDI::getJ2000EquatorialPos";

    Vec3d positionJNow = connection.positionJNow();
    const StelCore* core = StelApp::getInstance().getCore();
    Vec3d positionJ2000 = core->equinoxEquToJ2000(positionJNow, StelCore::RefractionOff);
    return positionJ2000;
}

void TelescopeClientINDI::telescopeGoto(const Vec3d &positionJ2000, StelObjectP selectObject)
{
    qDebug() << "TelescopeClientINDI::telescopeGoto";

    const StelCore* core = StelApp::getInstance().getCore();
    Vec3d positionJNow = core->j2000ToEquinoxEqu(positionJ2000, StelCore::RefractionOff);
    connection.setPositionJNow(positionJNow);
}

bool TelescopeClientINDI::isConnected() const
{
    qDebug() << "TelescopeClientINDI::isConnected";
    return true;
}

bool TelescopeClientINDI::hasKnownPosition() const
{
    qDebug() << "TelescopeClientINDI::hasKnownPosition";
    return true;
}

