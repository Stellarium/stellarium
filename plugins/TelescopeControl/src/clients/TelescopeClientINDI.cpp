#include "TelescopeClientINDI.hpp"

#include <QDebug>

#include "StelCore.hpp"
#include "StelUtils.hpp"

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
    INDIConnection::Coordinates positionJNow = connection.position();

    double longitudeRad = positionJNow.RA * M_PI / 12.0;
    double latitudeRad = positionJNow.DEC * M_PI / 180.0;

    Vec3d posJNow;
    StelUtils::spheToRect(longitudeRad, latitudeRad, posJNow);
    const StelCore* core = StelApp::getInstance().getCore();
    Vec3d posJ2000 = core->equinoxEquToJ2000(posJNow, StelCore::RefractionOff);
    return posJ2000;
}

void TelescopeClientINDI::telescopeGoto(const Vec3d &positionJ2000, StelObjectP selectObject)
{
    qDebug() << "TelescopeClientINDI::telescopeGoto ";

    Vec3d pos;
    const StelCore* core = StelApp::getInstance().getCore();
    Vec3d positionJNow = core->j2000ToEquinoxEqu(positionJ2000, StelCore::RefractionOff);

    StelUtils::rectToSphe(&pos[0], &pos[1], positionJNow);



    qWarning() << "rad " << pos[0] << ", " << pos[1];
    qWarning() << "Hms" << StelUtils::radToHmsStrAdapt(pos[0]) << ", " << StelUtils::radToDmsStrAdapt(pos[1]);
    qWarning() << "DcmDeg" << StelUtils::radToDecDegStr(pos[0]) << ", " << StelUtils::radToDecDegStr(pos[1]);

    double latitude = pos[0] * 12.0 / M_PI;
    double longitude = pos[1] * 180.0 / M_PI;

    pos[0] = latitude;
    pos[1] = longitude;

    qWarning() << "lat, long " << latitude << ", " << longitude;



    //Vec3d positionJNow(latitudine, longitude, 0.0);
    //const StelCore* core = StelApp::getInstance().getCore();
    //Vec3d positionJNow = core->j2000ToEquinoxEqu(positionJ2000, StelCore::RefractionOff);
    //connection.setPositionJNow(pos);
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

