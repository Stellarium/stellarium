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
 
#include "TelescopeClientDirectNexStar.hpp"

#include "NexStarConnection.hpp"
#include "NexStarCommand.hpp"
#include "common/LogFile.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"

#include <QRegExp>
#include <QStringList>

TelescopeClientDirectNexStar::TelescopeClientDirectNexStar(const QString &name, const QString &parameters, Equinox eq)
	: TelescopeClient(name)
	, time_delay(0)
	, equinox(eq)
	, nexstar(Q_NULLPTR)
	, last_ra(0)
	, queue_get_position(true)
	, next_pos_time(0)
{
	interpolatedPosition.reset();
	
	//Extract parameters
	//Format: "serial_port_name:time_delay"
	QRegExp paramRx("^([^:]*):(\\d+)$");
	QString serialDeviceName;
	if (paramRx.exactMatch(parameters))
	{
		// This QRegExp only matches valid integers
		serialDeviceName = paramRx.cap(1).trimmed();
		time_delay       = paramRx.cap(2).toInt();
	}
	else
	{
		qWarning() << "ERROR creating TelescopeClientDirectNexStar: invalid parameters.";
		return;
	}
	
	qDebug() << "TelescopeClientDirectNexStar parameters: port, time_delay:" << serialDeviceName << time_delay;
	
	//Validation: Time delay
	if (time_delay <= 0 || time_delay > 10000000)
	{
		qWarning() << "ERROR creating TelescopeClientDirectNexStar: time_delay not valid (should be less than 10000000)";
		return;
	}
	
	//end_of_timeout = -0x8000000000000000LL;
	
	#ifdef Q_OS_WIN
	if(serialDeviceName.right(serialDeviceName.size() - 3).toInt() > 9)
		serialDeviceName = "\\\\.\\" + serialDeviceName; // "\\.\COMxx", not sure if it will work
	#endif //Q_OS_WIN
	
	//Try to establish a connection to the telescope
	nexstar = new NexStarConnection(*this, qPrintable(serialDeviceName));
	if (nexstar->isClosed())
	{
		qWarning() << "ERROR creating TelescopeClientDirectNexStar: cannot open serial device" << serialDeviceName;
		return;
	}
	
	//This connection will be deleted in the destructor of Server
	addConnection(nexstar);
	
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
	if (equinox == EquinoxJNow)
	{
		const StelCore* core = StelApp::getInstance().getCore();
		position = core->j2000ToEquinoxEqu(j2000Pos, StelCore::RefractionOff);
	}

	const double ra_signed = atan2(position[1], position[0]);
	//Workaround for the discrepancy in precision between Windows/Linux/PPC Macs and Intel Macs:
	const double ra = (ra_signed >= 0) ? ra_signed : (ra_signed + 2.0 * M_PI);
	const double dec = atan2(position[2], std::sqrt(position[0]*position[0]+position[1]*position[1]));
	unsigned int ra_int = static_cast<unsigned int>(floor(0.5 + ra*(static_cast<unsigned int>(0x80000000)/M_PI)));
	int dec_int = static_cast<int>(floor(0.5 + dec*(static_cast<unsigned int>(0x80000000)/M_PI)));

	gotoReceived(ra_int, dec_int);
}

void TelescopeClientDirectNexStar::telescopeSync(const Vec3d &j2000Pos, StelObjectP selectObject)
{
	Q_UNUSED(selectObject)

	if (!isConnected())
		return;

	Vec3d position = j2000Pos;
	if (equinox == EquinoxJNow)
	{
		const StelCore* core = StelApp::getInstance().getCore();
		position = core->j2000ToEquinoxEqu(j2000Pos, StelCore::RefractionOff);
	}

	const double ra_signed = atan2(position[1], position[0]);
	//Workaround for the discrepancy in precision between Windows/Linux/PPC Macs and Intel Macs:
	const double ra = (ra_signed >= 0) ? ra_signed : (ra_signed + 2.0 * M_PI);
	const double dec = atan2(position[2], std::sqrt(position[0]*position[0]+position[1]*position[1]));
	unsigned int ra_int = static_cast<unsigned int>(floor(0.5 + ra*(static_cast<unsigned int>(0x80000000)/M_PI)));
	int dec_int = static_cast<int>(floor(0.5 + dec*(static_cast<unsigned int>(0x80000000)/M_PI)));

	syncReceived(ra_int, dec_int);
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
	return (!nexstar->isClosed());//TODO
}

bool TelescopeClientDirectNexStar::isInitialized(void) const
{
	return (!nexstar->isClosed());
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
	if (equinox == EquinoxJNow)
	{
		const StelCore* core = StelApp::getInstance().getCore();
		j2000Position = core->equinoxEquToJ2000(position, StelCore::RefractionOff);
	}
	interpolatedPosition.add(j2000Position, getNow(), server_micros, status);
}
