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

#include "TelescopeClient.hpp"
#include "TelescopeClientDirectLx200.hpp"
#include "TelescopeClientDirectNexStar.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelCore.hpp"

#include <cmath>

#include <QDebug>
#include <QHostAddress>
#include <QHostInfo>
#include <QRegExp>
#include <QString>
#include <QTcpSocket>
#include <QTextStream>

#ifdef Q_OS_WIN32
	#include <windows.h> // GetSystemTimeAsFileTime()
#else
	#include <sys/time.h>
#endif


TelescopeClient *TelescopeClient::create(const QString &url)
{
	// example url: My_first_telescope:TCP:J2000:localhost:10000:500000
	// split to:
	// name    = My_first_telescope
	// type    = TCP
	// equinox = J2000
	// params  = localhost:10000:500000
	//
	// The params part is optional.  We will use QRegExp to validate
	// the url and extact the components.

	// note, in a reg exp, [^:] matches any chararacter except ':'
	QRegExp urlSchema("^([^:]*):([^:]*):([^:]*)(?::(.*))?$");
	QString name, type, equinox, params;
	if (urlSchema.exactMatch(url))
	{
		// trimmed removes whitespace on either end of a QString
		name = urlSchema.capturedTexts().at(1).trimmed();
		type = urlSchema.capturedTexts().at(2).trimmed();
		equinox = urlSchema.capturedTexts().at(3).trimmed();
		params = urlSchema.capturedTexts().at(4).trimmed();
	}
	else
	{
		qWarning() << "WARNING - telescope definition" << url << "not recognised";
		return NULL;
	}

	Equinox eq = EquinoxJ2000;
	if (equinox == "JNow")
		eq = EquinoxJNow;

	qDebug() << "Creating telescope" << url << "; name/type/equinox/params:" << name << type << ((eq == EquinoxJNow) ? "JNow" : "J2000") << params;

	TelescopeClient * newTelescope = 0;
	
	//if (type == "Dummy")
	if (type == "TelescopeServerDummy")
	{
		newTelescope = new TelescopeClientDummy(name, params);
	}
	else if (type == "TCP")
	{
		newTelescope = new TelescopeTCP(name, params, eq);
	}
	else if (type == "TelescopeServerLx200") //BM: One of the rare occasions of painless extension
	{
		newTelescope= new TelescopeClientDirectLx200(name, params, eq);
	}
	else if (type == "TelescopeServerNexStar")
	{
		newTelescope= new TelescopeClientDirectNexStar(name, params, eq);
	}
	else
	{
		qWarning() << "WARNING - unknown telescope type" << type << "- not creating a telescope object for url" << url;
	}
	
	if (newTelescope && !newTelescope->isInitialized())
	{
		qDebug() << "TelescopeClient::create(): Unable to create a telescope client.";
		delete newTelescope;
		newTelescope = 0;
	}
	return newTelescope;
}


TelescopeClient::TelescopeClient(const QString &name) : name(name)
{
	nameI18n = name;
}

QString TelescopeClient::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);
	if (flags&Name)
	{
		oss << "<h2>" << nameI18n << "</h2>";
	}

	oss << getPositionInfoString(core, flags);

	postProcessInfoString(str, flags);

	return str;
}

//! returns the current system time in microseconds since the Epoch
//! Prior to revision 6308, it was necessary to put put this method in an
//! #ifdef block, as duplicate function definition caused errors during static
//! linking.
qint64 getNow(void)
{
// At the moment this can't be done in a platform-independent way with Qt
// (QDateTime and QTime don't support microsecond precision)
#ifdef Q_OS_WIN32
	FILETIME file_time;
	GetSystemTimeAsFileTime(&file_time);
	return (*((__int64*)(&file_time))/10) - 86400000000LL*134774;
#else
	struct timeval tv;
	gettimeofday(&tv,0);
	return tv.tv_sec * 1000000LL + tv.tv_usec;
#endif
}

TelescopeTCP::TelescopeTCP(const QString &name, const QString &params, Equinox eq) :
		TelescopeClient(name),
		tcpSocket(new QTcpSocket()),
		equinox(eq)
{
	hangup();
	
	// Example params:
	// localhost:10000:500000
	// split into:
	// host       = localhost
	// port       = 10000 (int)
	// time_delay = 500000 (int)

	QRegExp paramRx("^([^:]*):(\\d+):(\\d+)$");
	QString host;

	if (paramRx.exactMatch(params))
	{
		// I will not use the ok param to toInt as the
		// QRegExp only matches valid integers.
		host		= paramRx.capturedTexts().at(1).trimmed();
		port		= paramRx.capturedTexts().at(2).toInt();
		time_delay	= paramRx.capturedTexts().at(3).toInt();
	}
	else
	{
		qWarning() << "WARNING - incorrect TelescopeTCP parameters";
		return;
	}

	qDebug() << "TelescopeTCP paramaters host, port, time_delay:" << host << port << time_delay;

	if (port <= 0 || port > 0xFFFF)
	{
		qWarning() << "ERROR creating TelescopeTCP - port not valid (should be less than 32767)";
		return;
	}
	
	if (time_delay <= 0 || time_delay > 10000000)
	{
		qWarning() << "ERROR creating TelescopeTCP - time_delay not valid (should be less than 10000000)";
		return;
	}
	
	//BM: TODO: This may cause some delay when there are more telescopes
	QHostInfo info = QHostInfo::fromName(host);
	if (info.error())
	{
		qWarning() << "ERROR creating TelescopeTCP: error looking up host " << host << ":" << info.errorString();
		return;
	}
	//BM: is info.addresses().isEmpty() if there's no error?
	//qDebug() << "TelescopeClient::create(): Host addresses:" << info.addresses();
	foreach(const QHostAddress& resolvedAddress, info.addresses())
	{
		//For now, Stellarium's telescope servers support only IPv4
		if(resolvedAddress.protocol() == QTcpSocket::IPv4Protocol)
		{
			address = resolvedAddress;
			break;
		}
	}
	if(address.isNull())
	{
		qWarning() << "ERROR creating TelescopeTCP: cannot find IPv4 address. Addresses found at " << host << ":" << info.addresses();
		return;
	}
	
	end_of_timeout = -0x8000000000000000LL;
	
	interpolatedPosition.reset();
	
	connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketFailed(QAbstractSocket::SocketError)));
}

void TelescopeTCP::hangup(void)
{
	if (tcpSocket->isValid())
	{
		tcpSocket->abort();// Or maybe tcpSocket->close()?
	}
	
	readBufferEnd = readBuffer;
	writeBufferEnd = writeBuffer;
	wait_for_connection_establishment = false;
	
	interpolatedPosition.reset();
}

//! queues a GOTO command with the specified position to the write buffer.
//! For the data format of the command see the
//! "Stellarium telescope control protocol" text file
void TelescopeTCP::telescopeGoto(const Vec3d &j2000Pos)
{
	if (!isConnected())
		return;

	Vec3d position = j2000Pos;
	if (equinox == EquinoxJNow)
	{
		const StelCore* core = StelApp::getInstance().getCore();
		position = core->j2000ToEquinoxEqu(j2000Pos);
	}

	if (writeBufferEnd - writeBuffer + 20 < (int)sizeof(writeBuffer))
	{
		const double ra_signed = atan2(position[1], position[0]);
		//Workaround for the discrepancy in precision between Windows/Linux/PPC Macs and Intel Macs:
		const double ra = (ra_signed >= 0) ? ra_signed : (ra_signed + 2.0 * M_PI);
		const double dec = atan2(position[2], sqrt(position[0]*position[0]+position[1]*position[1]));
		unsigned int ra_int = (unsigned int)floor(0.5 + ra*(((unsigned int)0x80000000)/M_PI));
		int dec_int = (int)floor(0.5 + dec*(((unsigned int)0x80000000)/M_PI));
		// length of packet:
		*writeBufferEnd++ = 20;
		*writeBufferEnd++ = 0;
		// type of packet:
		*writeBufferEnd++ = 0;
		*writeBufferEnd++ = 0;
		// client_micros:
		qint64 now = getNow();
		*writeBufferEnd++ = now;
		now>>=8;
		*writeBufferEnd++ = now;
		now>>=8;
		*writeBufferEnd++ = now;
		now>>=8;
		*writeBufferEnd++ = now;
		now>>=8;
		*writeBufferEnd++ = now;
		now>>=8;
		*writeBufferEnd++ = now;
		now>>=8;
		*writeBufferEnd++ = now;
		now>>=8;
		*writeBufferEnd++ = now;
		// ra:
		*writeBufferEnd++ = ra_int;
		ra_int>>=8;
		*writeBufferEnd++ = ra_int;
		ra_int>>=8;
		*writeBufferEnd++ = ra_int;
		ra_int>>=8;
		*writeBufferEnd++ = ra_int;
		// dec:
		*writeBufferEnd++ = dec_int;
		dec_int>>=8;
		*writeBufferEnd++ = dec_int;
		dec_int>>=8;
		*writeBufferEnd++ = dec_int;
		dec_int>>=8;
		*writeBufferEnd++ = dec_int;
	}
	else
	{
		qDebug() << "TelescopeTCP(" << name << ")::telescopeGoto: "<< "communication is too slow, I will ignore this command";
	}
}

void TelescopeTCP::performWriting(void)
{
	const int to_write = writeBufferEnd - writeBuffer;
	const int rc = tcpSocket->write(writeBuffer, to_write);
	if (rc < 0)
	{
		//TODO: Better error message. See the Qt documentation.
		qDebug() << "TelescopeTCP(" << name << ")::performWriting: "
			<< "write failed: " << tcpSocket->errorString();
		hangup();
	}
	else if (rc > 0)
	{
		if (rc >= to_write)
		{
			// everything written
			writeBufferEnd = writeBuffer;
		}
		else
		{
			// partly written
			memmove(writeBuffer, writeBuffer + rc, to_write - rc);
			writeBufferEnd -= rc;
		}
	}
}

//! try to read some data from the telescope server
void TelescopeTCP::performReading(void)
{
	const int to_read = readBuffer + sizeof(readBuffer) - readBufferEnd;
	const int rc = tcpSocket->read(readBufferEnd, to_read);
	if (rc < 0)
	{
		//TODO: Better error warning. See the Qt documentation.
		qDebug() << "TelescopeTCP(" << name << ")::performReading: " << "read failed: " << tcpSocket->errorString();
		hangup();
	}
	else if (rc == 0)
	{
		qDebug() << "TelescopeTCP(" << name << ")::performReading: " << "server has closed the connection";
		hangup();
	}
	else
	{
		readBufferEnd += rc;
		char *p = readBuffer;
		// parse the data in the read buffer:
		while (readBufferEnd - p >= 2)
		{
			const int size = (int)(((unsigned char)(p[0])) | (((unsigned int)(unsigned char)(p[1])) << 8));
			if (size > (int)sizeof(readBuffer) || size < 4)
			{
				qDebug() << "TelescopeTCP(" << name << ")::performReading: " << "bad packet size: " << size;
				hangup();
				return;
			}
			if (size > readBufferEnd - p)
			{
				// wait for complete packet
				break;
			}
			const int type = (int)(((unsigned char)(p[2])) | (((unsigned int)(unsigned char)(p[3])) << 8));
			// dispatch:
			switch (type)
			{
				case 0:
				{
				// We have received position information.
				// For the data format of the message see the
				// "Stellarium telescope control protocol"
					if (size < 24)
					{
						qDebug() << "TelescopeTCP(" << name << ")::performReading: " << "type 0: bad packet size: " << size;
						hangup();
						return;
					}
					const qint64 server_micros = (qint64)
						(((quint64)(unsigned char)(p[ 4])) |
						(((quint64)(unsigned char)(p[ 5])) <<  8) |
						(((quint64)(unsigned char)(p[ 6])) << 16) |
						(((quint64)(unsigned char)(p[ 7])) << 24) |
						(((quint64)(unsigned char)(p[ 8])) << 32) |
						(((quint64)(unsigned char)(p[ 9])) << 40) |
						(((quint64)(unsigned char)(p[10])) << 48) |
						(((quint64)(unsigned char)(p[11])) << 56));
					const unsigned int ra_int =
						((unsigned int)(unsigned char)(p[12])) |
						(((unsigned int)(unsigned char)(p[13])) <<  8) |
						(((unsigned int)(unsigned char)(p[14])) << 16) |
						(((unsigned int)(unsigned char)(p[15])) << 24);
					const int dec_int =
						(int)(((unsigned int)(unsigned char)(p[16])) |
						     (((unsigned int)(unsigned char)(p[17])) <<  8) |
						     (((unsigned int)(unsigned char)(p[18])) << 16) |
						     (((unsigned int)(unsigned char)(p[19])) << 24));
					const int status =
						(int)(((unsigned int)(unsigned char)(p[20])) |
						     (((unsigned int)(unsigned char)(p[21])) <<  8) |
						     (((unsigned int)(unsigned char)(p[22])) << 16) |
						     (((unsigned int)(unsigned char)(p[23])) << 24));

					const double ra  =  ra_int * (M_PI/(unsigned int)0x80000000);
					const double dec = dec_int * (M_PI/(unsigned int)0x80000000);
					const double cdec = cos(dec);
					Vec3d position(cos(ra)*cdec, sin(ra)*cdec, sin(dec));
					Vec3d j2000Position = position;
					if (equinox == EquinoxJNow)
					{
						const StelCore* core = StelApp::getInstance().getCore();
						j2000Position = core->equinoxEquToJ2000(position);
					}
					interpolatedPosition.add(j2000Position, getNow(), server_micros, status);
				}
				break;
				default:
					qDebug() << "TelescopeTCP(" << name << ")::performReading: " << "ignoring unknown packet, type: " << type;
				break;
			}
			p += size;
		}
		if (p >= readBufferEnd)
		{
			// everything handled
			readBufferEnd = readBuffer;
		}
		else
		{
			// partly handled
			memmove(readBuffer, p, readBufferEnd - p);
			readBufferEnd -= (p - readBuffer);
		}
	}
}

//! estimates where the telescope is by interpolation in the stored
//! telescope positions:
Vec3d TelescopeTCP::getJ2000EquatorialPos(const StelCore*) const
{
	const qint64 now = getNow() - time_delay;
	return interpolatedPosition.get(now);
}

//! checks if the socket is connected, tries to connect if it is not
//@return true if the socket is connected
bool TelescopeTCP::prepareCommunication()
{
	if(tcpSocket->state() == QAbstractSocket::ConnectedState)
	{
		if(wait_for_connection_establishment)
		{
			wait_for_connection_establishment = false;
			qDebug() << "TelescopeTCP(" << name << ")::prepareCommunication: Connection established";
		}
		return true;
	}
	else if(wait_for_connection_establishment)
	{
		const qint64 now = getNow();
		if (now > end_of_timeout)
		{
			end_of_timeout = now + 1000000;
			qDebug() << "TelescopeTCP(" << name << ")::prepareCommunication: Connection attempt timed out";
			hangup();
		}
	}
	else
	{
		const qint64 now = getNow();
		if (now < end_of_timeout) 
			return false; //Don't try to reconnect for some time
		end_of_timeout = now + 5000000;
		tcpSocket->connectToHost(address, port);
		wait_for_connection_establishment = true;
		qDebug() << "TelescopeTCP(" << name << ")::prepareCommunication: Attempting to connect to host" << address.toString() << "at port" << port;
	}
	return false;
}

void TelescopeTCP::performCommunication()
{
	if (tcpSocket->state() == QAbstractSocket::ConnectedState)
	{
		performWriting();
		
		if (tcpSocket->bytesAvailable() > 0)
		{
			//If performReading() is called when there are no bytes to read,
			//it closes the connection
			performReading();
		}
	}
}

//TODO: More informative error messages?
void TelescopeTCP::socketFailed(QAbstractSocket::SocketError)
{
	qDebug() << "TelescopeTCP(" << name << "): TCP socket error:\n" << tcpSocket->errorString();
}
