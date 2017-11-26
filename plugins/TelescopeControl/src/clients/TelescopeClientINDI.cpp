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
    //qWarning() << "TelescopeClientINDI::getJ2000EquatorialPos";

    Vec3d positionJNow = connection.positionJNow();

    const StelCore* core = StelApp::getInstance().getCore();
    Vec3d positionJ2000 = core->equinoxEquToJ2000(positionJNow, StelCore::RefractionOff);

    //StelUtils::

    double longitudeDec = positionJ2000[0];
    double latitudeDec = positionJ2000[1];

    //qWarning() << "positioin (long, lat) [dec] : " << longitudeDec << ", " << latitudeDec;


    double longitudeRad = longitudeDec * M_PI / 12.0;
    double latitudeRad = latitudeDec * M_PI / 180.0;

    //qWarning() << "positioin (long, lat) : " << longitudeRad << ", " << latitudeRad;

    Vec3d pos;
    //pos[0] = longitudeRad;
    //pos[1] = latitudeDec;


    StelUtils::spheToRect(longitudeRad, latitudeRad, pos);

    return pos;
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
    connection.setPositionJNow(pos);
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

