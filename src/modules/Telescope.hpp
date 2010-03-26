/*
 * Author and Copyright of this file and of the stellarium telescope feature:
 * Johannes Gajdosik, 2006
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _TELESCOPE_HPP_
#define _TELESCOPE_HPP_

#include <QHostAddress>
#include <QHostInfo>
#include <QList>
#include <QString>
#include <QTcpSocket>
#include <QObject>

#include "StelObject.hpp"
#include "StelNavigator.hpp"

qint64 getNow(void);

namespace Stel {

class Telescope : public QObject, public StelObject
{
	Q_OBJECT
public:
	static Telescope *create(const QString &url);
	virtual ~Telescope(void) {}

	// Method inherited from StelObject
	QString getEnglishName(void) const {return name;}
	QString getNameI18n(void) const {return nameI18n;}
	//! Telescope supports the following InfoStringGroup flags:
	//! - Name
	//! - RaDecJ2000
	//! - RaDec
	//! - PlainText
	//! @param core the StelCore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString containing an HMTL encoded description of the Telescope.
	QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const;
	QString getType(void) const {return "Telescope";}
	virtual double getAngularSize(const StelCore* core) const {Q_ASSERT(0); return 0;}	// TODO

	// Methods specific to telescope
	virtual void telescopeGoto(const Vec3d &j2000Pos) = 0;
	virtual bool isConnected(void) const = 0;
	virtual bool hasKnownPosition(void) const = 0;
	void addOcular(double fov) {if (fov>=0.0) oculars.push_back(fov);}
	const QList<double> &getOculars(void) const {return oculars;}

	// all TCP (and all possible other style) communication shall be done in these functions:
	virtual bool prepareCommunication() {return false;}
	virtual void performCommunication() {}

protected:
	Telescope(const QString &name);
	QString nameI18n;
	const QString name;
private:
	bool isInitialized(void) const {return true;}
	float getSelectPriority(const StelNavigator *nav) const {return -10.f;}
private:
	QList<double> oculars; // fov of the oculars
};

//! This Telescope class can controll a telescope by communicating
//! to a server process ("telescope server") via 
//! the "Stellarium telescope control protocol" over TCP/IP.
//! The "Stellarium telescope control protocol" is specified in a seperate
//! document along with the telescope server software.
class TelescopeTcp : public Telescope
{
	Q_OBJECT
public:
	TelescopeTcp(const QString &name,const QString &params);
	~TelescopeTcp(void)
	{
		hangup();
	}
private:
	bool isConnected(void) const
	{
		//return (tcpSocket->isValid() && !wait_for_connection_establishment);
		return (tcpSocket->state() == QAbstractSocket::ConnectedState);
	}
	Vec3d getJ2000EquatorialPos(const StelNavigator *nav=0) const;
	bool prepareCommunication();
	void performCommunication();
	void telescopeGoto(const Vec3d &j2000Pos);
	bool isInitialized(void) const
	{
		return (!address.isNull());
	}
	void performReading(void);
	void performWriting(void);
private:
	void hangup(void);
	void resetPositions(void);
	QHostAddress address;
	unsigned int port;
	QTcpSocket * tcpSocket;
	bool wait_for_connection_establishment;
	qint64 end_of_timeout;
	char readBuffer[120];
	char *readBufferEnd;
	char writeBuffer[120];
	char *writeBufferEnd;
	int time_delay;
	struct Position
	{
		qint64 server_micros;
		qint64 client_micros;
		Vec3d pos;
		int status;
	};
	Position positions[16];
	Position *position_pointer;
	Position *const end_position;
	virtual bool hasKnownPosition(void) const
	{
		return (position_pointer->client_micros != 0x7FFFFFFFFFFFFFFFLL);
	}
private slots:
	void socketFailed(QAbstractSocket::SocketError socketError);
};

}//namespace Stel

#endif // _TELESCOPE_HPP_
