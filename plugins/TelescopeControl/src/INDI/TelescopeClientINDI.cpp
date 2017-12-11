#include "TelescopeClientINDI.hpp"

#include <QDebug>

#include "StelCore.hpp"
#include "StelUtils.hpp"

TelescopeClientINDI::TelescopeClientINDI(const QString &name, const QString &params):
    TelescopeClient(name)
{
	qDebug() << "TelescopeClientINDI::TelescopeClientINDI";

	QRegExp paramRx("^([^:]*):(\\d+):([^:]*)$");
	QString host;
	int port = 0;
	if (paramRx.exactMatch(params))
	{
		host = paramRx.capturedTexts().at(1).trimmed();
		port = paramRx.capturedTexts().at(2).toInt();
		mDevice = paramRx.capturedTexts().at(3).trimmed();
	}

	mConnection.setServer(host.toStdString().c_str(), port);
	mConnection.watchDevice(mDevice.toStdString().c_str());
	mConnection.connectServer();
}

TelescopeClientINDI::~TelescopeClientINDI()
{
	mConnection.disconnectServer();
}

Vec3d TelescopeClientINDI::getJ2000EquatorialPos(const StelCore*) const
{
	INDIConnection::Coordinates positionJNow = mConnection.position();

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
	const StelCore* core = StelApp::getInstance().getCore();
	Vec3d posRectJNow = core->j2000ToEquinoxEqu(positionJ2000, StelCore::RefractionOff);
	Vec3d posJ2000;
	StelUtils::rectToSphe(&posJ2000[0], &posJ2000[1], posRectJNow);

	if (posJ2000[0] < 0.0)
		posJ2000[0] += 2*M_PI;

	double longitudeRad = posJ2000[0] * 12.0 / M_PI;
	double latitudeRad = posJ2000[1] * 180.0 / M_PI;


	INDIConnection::Coordinates positionJNow = mConnection.position();
	positionJNow.RA = longitudeRad;
	positionJNow.DEC = latitudeRad;

	mConnection.setPosition(positionJNow);
}

bool TelescopeClientINDI::isConnected() const
{
	return mConnection.isDeviceConnected();
}

bool TelescopeClientINDI::hasKnownPosition() const
{
	return mConnection.isDeviceConnected();
}

