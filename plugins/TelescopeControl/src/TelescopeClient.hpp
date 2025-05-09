/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2006 Johannes Gajdosik
 * Copyright (C) 2009 Bogdan Marinov
 * 
 * This module was originally written by Johannes Gajdosik in 2006
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef TELESCOPECLIENT_HPP
#define TELESCOPECLIENT_HPP

#include <QHostAddress>
#include <QHostInfo>
#include <QList>
#include <QString>
#include <QTcpSocket>
#include <QObject>

#include "StelObject.hpp"
#include "StelTranslator.hpp"
#include "TelescopeControl.hpp"
#include "common/InterpolatedPosition.hpp"

class StelCore;


//! An abstract base class that should never be used directly, only inherited.
//! This class used to be called Telescope, but it has been renamed
//! to TelescopeClient in order to resolve a compiler/linker conflict
//! with the identically named Telescope class in Stellarium's main code.
class TelescopeClient : public QObject, public StelObject
{
	Q_OBJECT
public:
	static const QString TELESCOPECLIENT_TYPE;
	//! example url: My_first_telescope:TCP:J2000:localhost:10000:500000
	//! split to:
	//! name    = My_first_telescope
	//! type    = TCP
	//! equinox = J2000
	//! params  = localhost:10000:500000
	//!
	//! The params part is optional.  We use QRegularExpression to validate the url and extract the components.
	static TelescopeClient *create(const QString &url);
	~TelescopeClient(void) override {}
	
	// Methods inherited from StelObject
	QString getEnglishName(void) const override {return name;}
	QString getNameI18n(void) const override {return nameI18n;}
	//Vec3f getInfoColor(void) const override
	//{
	//	return Vec3f(1.f, 1.f, 1.f);
	//}
	//! TelescopeClient supports the following InfoStringGroup flags:
	//! - Name
	//! - RaDecJ2000
	//! - RaDec
	//! - PlainText
	//! @param core the StelCore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString containing an HTML encoded description of the Telescope.
	QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const override;
	QString getType(void) const override {return TELESCOPECLIENT_TYPE;}
	QString getObjectType(void) const override {return N_("telescope");}
	QString getObjectTypeI18n(void) const override {return q_(getObjectType());}
	QString getID() const override {return name;}
		
	// Methods specific to telescope
	virtual void telescopeGoto(const Vec3d &j2000Pos, StelObjectP selectObject) = 0;
	//! Some telescopes can synchronize the telescope with the given position or object.
	//! The base client does nothing.
	//! Derived classes may override this command
	//! @todo: Properly document method and the arguments
	virtual void telescopeSync(const Vec3d &j2000Pos, StelObjectP selectObject){};
	//! report whether this client can respond to sync commands.
	//! Can be used for GUI tweaks
	virtual bool isTelescopeSyncSupported() const {return false;}
	//! Send command to abort slew. Not all telescopes support this, base implementation only gives a warning.
	//! After abort, the current position should be retrieved and displayed.
	virtual void telescopeAbortSlew();
	//! report whether this client can abort a running slew.
	//! Can be used for GUI tweaks
	virtual bool isAbortSlewSupported() const {return false;}

	//!
	//! \brief move
	//! \param angle [0,360). 180=Down/South, 270=Right/West, 0=Up/North, 90=Left/East
	//! \param speed [0,1]
	//! The default implementation does nothing but emit a warning.
	virtual void move(double angle, double speed);
	virtual bool isConnected(void) const = 0;
	virtual bool hasKnownPosition(void) const = 0;
	//! Store field of view for an ocular circle (unrelated to Oculars plugin)
	void addOcular(double fov) {if (fov>=0.0) ocularFovs.push_back(fov);}
	//! Retrieve list of fields of view for this telescope
	const QList<double> &getOculars(void) const {return ocularFovs;}
	
	virtual bool prepareCommunication() {return false;}
	virtual void performCommunication() {}

	//! Some telescope types may override this method to display additional user interface elements.
	virtual QWidget* createControlWidget(QSharedPointer<TelescopeClient> telescope, QWidget* parent = nullptr) const { Q_UNUSED(telescope) Q_UNUSED(parent) return nullptr; }

protected:
	TelescopeClient(const QString &name);
	QString nameI18n;
	const QString name;

	virtual QString getTelescopeInfoString(const StelCore* core, const InfoStringGroup& flags) const
	{
		Q_UNUSED(core)
		Q_UNUSED(flags)
		return QString();
	}

	//! returns the current system time in microseconds since the Epoch
	static qint64 getNow(void);
private:
	virtual bool isInitialized(void) const {return true;}
	float getSelectPriority(const StelCore* core) const override {Q_UNUSED(core) return -10.f;}
private:
	QList<double> ocularFovs; // fov of the oculars
};

//! Example Telescope class. A physical telescope does not exist.
//! This can be used as a starting point for implementing a derived
//! Telescope class.
//! This class used to be called TelescopeDummy, but it had to be renamed
//! in order to resolve a compiler/linker conflict with the identically named
//! TelescopeDummy class in Stellarium's main code.
class TelescopeClientDummy : public TelescopeClient
{
public:
	TelescopeClientDummy(const QString &name, const QString &) : TelescopeClient(name)
	{
		desired_pos[0] = XYZ[0] = 1.0;
		desired_pos[1] = XYZ[1] = 0.0;
		desired_pos[2] = XYZ[2] = 0.0;
	}
	~TelescopeClientDummy(void) override {}
	bool isConnected(void) const override
	{
		return true;
	}
	bool prepareCommunication(void) override
	{
		XYZ = XYZ * 31.0 + desired_pos;
		const double lq = XYZ.normSquared();
		if (lq > 0.0)
			XYZ *= (1.0/std::sqrt(lq));
		else
			XYZ = desired_pos;
		return true;
	}
	void telescopeGoto(const Vec3d &j2000Pos, StelObjectP selectObject) override
	{
		Q_UNUSED(selectObject)
		desired_pos = j2000Pos;
		desired_pos.normalize();
	}
	void telescopeAbortSlew() override
	{
		desired_pos=XYZ;
		qDebug() << "Telescope" << getID() << "Slew aborted";
	}
	bool isAbortSlewSupported() const override {return true;}
	bool hasKnownPosition(void) const override
	{
		return true;
	}
	Vec3d getJ2000EquatorialPos(const StelCore*) const override
	{
		return XYZ;
	}
	
private:
	Vec3d XYZ; // j2000 position
	Vec3d desired_pos;
};

//! This TelescopeClient class can control a telescope by communicating
//! to a server process ("telescope server") via 
//! the "Stellarium telescope control protocol" over TCP/IP.
//! The "Stellarium telescope control protocol" is specified in a separate
//! document along with the telescope server software.
class TelescopeTCP : public TelescopeClient
{
	Q_OBJECT
public:
	TelescopeTCP(const QString &name, const QString &params, TelescopeControl::Equinox eq = TelescopeControl::EquinoxJ2000);
	~TelescopeTCP(void) override
	{
		hangup();
	}
	bool isConnected(void) const override
	{
		//return (tcpSocket->isValid() && !wait_for_connection_establishment);
		return (tcpSocket->state() == QAbstractSocket::ConnectedState);
	}
	
private:
	Vec3d getJ2000EquatorialPos(const StelCore* core=nullptr) const override;
	bool prepareCommunication() override;
	void performCommunication() override;
	void telescopeGoto(const Vec3d &j2000Pos, StelObjectP selectObject) override;
	bool isInitialized(void) const override
	{
		return (!address.isNull());
	}
	void performReading(void);
	void performWriting(void);
	
private:
	void hangup(void);
	QHostAddress address;
	quint16 port;
	QTcpSocket * tcpSocket;
	bool wait_for_connection_establishment;
	qint64 end_of_timeout;
	char readBuffer[120];
	char *readBufferEnd;
	char writeBuffer[120];
	char *writeBufferEnd;
	int time_delay;

	InterpolatedPosition interpolatedPosition;
	bool hasKnownPosition(void) const override
	{
		return interpolatedPosition.isKnown();
	}

	TelescopeControl::Equinox equinox;
	
private slots:
	void socketConnected(void);
	void socketFailed(QAbstractSocket::SocketError socketError);
};

#endif // TELESCOPECLIENT_HPP
