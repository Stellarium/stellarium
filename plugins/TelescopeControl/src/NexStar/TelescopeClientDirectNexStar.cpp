/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2009 Bogdan Marinov (this file,
 * reusing code written by Johannes Gajdosik in 2006)
 * 
 * Johannes Gajdosik wrote in 2006 the original telescope control feature
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
 * 
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
 
#include <QRegularExpression>
#include <QStringList>

#include "TelescopeClientDirectNexStar.hpp"

#include "NexStarConnection.hpp"
#include "NexStarCommand.hpp"
#include "common/LogFile.hpp"
#include "StelCore.hpp"
#ifndef QT_NO_DEBUG
#include "StelUtils.hpp"
#endif

TelescopeClientDirectNexStar::TelescopeClientDirectNexStar(const QString &name, const QString &parameters, TelescopeControl::Equinox eq)
	: TelescopeClient(name)
	, time_delay(0)
	, equinox(eq)
	, nexstar(nullptr)
	, last_ra(0)
	, queue_get_position(true)
	, next_pos_time(0)
{
	interpolatedPosition.reset();
	
	//Extract parameters. Accept the same serial and TCP formats as LX200:
	// Serial:  "serial_port_name:time_delay"
	// Network: "host_name:tcp_port:time_delay"
	static const QRegularExpression tcpParamRx("^([^:]+):(\\d+):(\\d+)$");
	static const QRegularExpression serialParamRx("^([^:]*):(\\d+)$");
	QRegularExpressionMatch tcpMatch = tcpParamRx.match(parameters);
	QString serialDeviceName;
	QString hostName;
	int tcpPort = 0;
	const bool useNetwork = tcpMatch.hasMatch();
	if (useNetwork)
	{
		hostName   = tcpMatch.captured(1).trimmed();
		tcpPort    = tcpMatch.captured(2).toInt();
		time_delay = tcpMatch.captured(3).toInt();
		if (hostName.isEmpty() || tcpPort <= 0 || tcpPort > 65535)
		{
			qCWarning(Telescopes) << "ERROR creating TelescopeClientDirectNexStar: invalid host or port.";
			return;
		}
	}
	else
	{
		QRegularExpressionMatch serialMatch = serialParamRx.match(parameters);
		if (serialMatch.hasMatch())
		{
			serialDeviceName = serialMatch.captured(1).trimmed();
			time_delay       = serialMatch.captured(2).toInt();
		}
		else
		{
			qCWarning(Telescopes) << "ERROR creating TelescopeClientDirectNexStar: invalid parameters.";
			return;
		}
	}
	
	if (useNetwork)
		qCDebug(Telescopes) << "TelescopeClientDirectNexStar parameters: host, port, time_delay:"
		                    << hostName << tcpPort << time_delay;
	else
		qCDebug(Telescopes) << "TelescopeClientDirectNexStar parameters: port, time_delay:"
		                    << serialDeviceName << time_delay;
	
	//Validation: Time delay
	if (time_delay <= 0 || time_delay > 10000000)
	{
		qCWarning(Telescopes) << "ERROR creating TelescopeClientDirectNexStar: time_delay not valid (should be less than 10000000)";
		return;
	}
	
	//end_of_timeout = -0x8000000000000000LL;
	
	//Try to establish a connection to the telescope
	if (useNetwork)
	{
		NexStarTcpConnection *connection = new NexStarTcpConnection(*this, qPrintable(hostName), tcpPort);
		if (connection->isClosed())
		{
			qCWarning(Telescopes) << "ERROR creating TelescopeClientDirectNexStar: cannot connect to"
			                      << hostName << "port" << tcpPort;
			delete connection;
			return;
		}
		nexstar = connection;
		// Server owns the connection and deletes it with its socket list in Server::SocketList::clear().
		addConnection(connection);
	}
	else
	{
		#ifdef Q_OS_WIN
		if(serialDeviceName.right(serialDeviceName.size() - 3).toInt() > 9)
			serialDeviceName = "\\\\.\\" + serialDeviceName; // "\\.\COMxx", not sure if it will work
		#endif //Q_OS_WIN

		NexStarSerialConnection *connection = new NexStarSerialConnection(*this, qPrintable(serialDeviceName));
		if (connection->isClosed())
		{
			qCWarning(Telescopes) << "ERROR creating TelescopeClientDirectNexStar: cannot open serial device"
			                      << serialDeviceName;
			delete connection;
			return;
		}
		nexstar = connection;
		// Server owns the connection and deletes it with its socket list in Server::SocketList::clear().
		addConnection(connection);
	}
	
	last_ra = 0;
	queue_get_position = true;
	next_pos_time = -0x8000000000000000LL;
}

//! queues a GOTO command
void TelescopeClientDirectNexStar::telescopeGoto(const Vec3d &j2000Pos, StelObjectP selectObject)
{
	Q_UNUSED(selectObject)

	if (!isConnected())
		return;

	Vec3d position = j2000Pos;
	if (equinox == TelescopeControl::EquinoxJNow)
	{
		const StelCore* core = StelApp::getInstance().getCore();
		position = core->j2000ToEquinoxEqu(j2000Pos, StelCore::RefractionOff);
	}

	const double ra_signed = atan2(position[1], position[0]);
	//Workaround for the discrepancy in precision between Windows/Linux/PPC Macs and Intel Macs:
	const double ra = (ra_signed >= 0) ? ra_signed : (ra_signed + 2.0 * M_PI);
	const double dec = atan2(position[2], std::sqrt(position[0]*position[0]+position[1]*position[1]));
	unsigned int ra_int = static_cast<unsigned int>(std::floor(0.5 + ra*(static_cast<unsigned int>(0x80000000)/M_PI)));
	int dec_int = static_cast<int>(std::floor(0.5 + dec*(static_cast<unsigned int>(0x80000000)/M_PI)));

	gotoReceived(ra_int, dec_int);
}

void TelescopeClientDirectNexStar::telescopeSync(const Vec3d &j2000Pos, StelObjectP selectObject)
{
	Q_UNUSED(selectObject)

	if (!isConnected())
		return;

	Vec3d position = j2000Pos;
	if (equinox == TelescopeControl::EquinoxJNow)
	{
		const StelCore* core = StelApp::getInstance().getCore();
		position = core->j2000ToEquinoxEqu(j2000Pos, StelCore::RefractionOff);
	}

	const double ra_signed = atan2(position[1], position[0]);
	//Workaround for the discrepancy in precision between Windows/Linux/PPC Macs and Intel Macs:
	const double ra = (ra_signed >= 0) ? ra_signed : (ra_signed + 2.0 * M_PI);
	const double dec = atan2(position[2], std::sqrt(position[0]*position[0]+position[1]*position[1]));
	unsigned int ra_int = static_cast<unsigned int>(std::floor(0.5 + ra*(static_cast<unsigned int>(0x80000000)/M_PI)));
	int dec_int = static_cast<int>(std::floor(0.5 + dec*(static_cast<unsigned int>(0x80000000)/M_PI)));

	syncReceived(ra_int, dec_int);
}

void TelescopeClientDirectNexStar::telescopeAbortSlew()
{
	if (!isConnected())
		return;
	nexstar->sendAbort();

	// sendAbort() discards every queued command, including any position query
	// that was in flight. That would leave queue_get_position stuck false and
	// stop the position polling entirely. Re-arm it, but with a short delay so
	// that answers to the discarded queries can arrive and be ignored before
	// the next query is sent.
	queue_get_position = true;
	next_pos_time = GetNow() + 500000;
}

void TelescopeClientDirectNexStar::gotoReceived(unsigned int ra_int, int dec_int)
{
	nexstar->sendGoto(ra_int, dec_int);
}

void TelescopeClientDirectNexStar::syncReceived(unsigned int ra_int, int dec_int)
{
	nexstar->sendSync(ra_int, dec_int);
}

//! estimates where the telescope is by interpolation in the stored
//! telescope positions:
Vec3d TelescopeClientDirectNexStar::getJ2000EquatorialPos(const StelCore*) const
{
	const qint64 now = getNow() - time_delay;
	return interpolatedPosition.get(now);
}

bool TelescopeClientDirectNexStar::prepareCommunication()
{
	//TODO: Nothing to prepare?
	return true;
}

void TelescopeClientDirectNexStar::performCommunication()
{
	step(10000);
}

void TelescopeClientDirectNexStar::communicationResetReceived(void)
{
	queue_get_position = true;
	next_pos_time = -0x8000000000000000LL;
	
#ifndef QT_NO_DEBUG
	*log_file << Now() << "TelescopeClientDirectNexStar::communicationResetReceived" << StelUtils::getEndLineChar();
#endif
}

//! Called by NexStarCommandGetRaDec::readAnswerFromBuffer().
void TelescopeClientDirectNexStar::raReceived(unsigned int ra_int)
{
	last_ra = ra_int;
#ifndef QT_NO_DEBUG
	*log_file << Now() << "TelescopeClientDirectNexStar::raReceived: " << ra_int << StelUtils::getEndLineChar();
#endif
}

//! Called by NexStarCommandGetRaDec::readAnswerFromBuffer().
//! Should be called after raReceived(), as it contains a call to sendPosition().
void TelescopeClientDirectNexStar::decReceived(unsigned int dec_int)
{
#ifndef QT_NO_DEBUG
	*log_file << Now() << "TelescopeClientDirectNexStar::decReceived: " << dec_int << StelUtils::getEndLineChar();
#endif
	const int nexstar_status = 0;
	sendPosition(last_ra, static_cast<int>(dec_int), nexstar_status);
	queue_get_position = true;
}

void TelescopeClientDirectNexStar::step(long long int timeout_micros)
{
	if (!isConnected())
		return;
	long long int now = GetNow();
	if (queue_get_position && now >= next_pos_time)
	{
		nexstar->sendCommand(new NexStarCommandGetRaDec(*this));
		queue_get_position = false;
		next_pos_time = now + 500000;
	}
	Server::step(timeout_micros);
}

bool TelescopeClientDirectNexStar::isConnected(void) const
{
	return (nexstar && !nexstar->isClosed());
}

bool TelescopeClientDirectNexStar::isInitialized(void) const
{
	return (nexstar && !nexstar->isClosed());
}

//Merged from Connection::sendPosition() and TelescopeTCP::performReading()
void TelescopeClientDirectNexStar::sendPosition(unsigned int ra_int, int dec_int, int status)
{
	//Server time is "now", because this class is the server
	const qint64 server_micros = static_cast<qint64>(GetNow());
	const double ra  =  ra_int * (M_PI/0x80000000u);
	const double dec = dec_int * (M_PI/0x80000000u);
	const double cdec = cos(dec);
	Vec3d position(cos(ra)*cdec, sin(ra)*cdec, sin(dec));
	Vec3d j2000Position = position;
	if (equinox == TelescopeControl::EquinoxJNow)
	{
		const StelCore* core = StelApp::getInstance().getCore();
		j2000Position = core->equinoxEquToJ2000(position, StelCore::RefractionOff);
	}
	interpolatedPosition.add(j2000Position, getNow(), server_micros, status);
}
