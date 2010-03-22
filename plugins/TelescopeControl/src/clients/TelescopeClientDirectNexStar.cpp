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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
#include "TelescopeClientDirectNexStar.hpp"

#include "NexStarConnection.hpp"
#include "NexStarCommand.hpp"
#include "LogFile.hpp"

#include <QRegExp>
#include <QStringList>

TelescopeClientDirectNexStar::TelescopeClientDirectNexStar
                            (const QString &name, const QString &parameters) : 
                             TelescopeClient(name),
                             end_position(positions + (sizeof(positions)/sizeof(positions[0])))
{
	resetPositions();
	
	//Extract parameters
	//Format: "serial_port_name:time_delay"
	QRegExp paramRx("^([^:]*):(\\d+)$");
	QString serialDeviceName;
	if (paramRx.exactMatch(parameters))
	{
		// This QRegExp only matches valid integers
		serialDeviceName = paramRx.capturedTexts().at(1).trimmed();
		time_delay       = paramRx.capturedTexts().at(2).toInt();
	}
	else
	{
		qWarning() << "ERROR creating TelescopeClientDirectNexStar: invalid parameters.";
		return;
	}
	
	qDebug() << "TelescopeClientDirectNexStar paramaters: port, time_delay:" << serialDeviceName << time_delay;
	
	//Validation: Time delay
	if (time_delay <= 0 || time_delay > 10000000)
	{
		qWarning() << "ERROR creating TelescopeClientDirectNexStar: time_delay not valid (should be less than 10000000)";
		return;
	}
	
	//end_of_timeout = -0x8000000000000000LL;
	
	#ifdef Q_OS_WIN32
	if(serialDeviceName.right(serialDeviceName.size() - 3).toInt() > 9)
		serialDeviceName = "\\\\.\\" + serialDeviceName + ":";//"\\.\COMxx", not sure if it will work
	else
		serialDeviceName = serialDeviceName + ":";
	#endif //Q_OS_WIN32
	
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

//! resets/initializes the array of positions kept for position interpolation.
void TelescopeClientDirectNexStar::resetPositions()
{
	for (position_pointer = positions; position_pointer < end_position; position_pointer++)
	{
		position_pointer->server_micros = 0x7FFFFFFFFFFFFFFFLL;
		position_pointer->client_micros = 0x7FFFFFFFFFFFFFFFLL;
		position_pointer->pos[0] = 0.0;
		position_pointer->pos[1] = 0.0;
		position_pointer->pos[2] = 0.0;
		position_pointer->status = 0;
	}
	position_pointer = positions;
}

//! queues a GOTO command with the specified position to the write buffer.
//! For the data format of the command see the
//! "Stellarium telescope control protocol" text file
void TelescopeClientDirectNexStar::telescopeGoto(const Vec3d &j2000Pos)
{
	if (isConnected())//TODO: See the else clause, think how to do the same thing
	{
		//if (writeBufferEnd - writeBuffer + 20 < (int)sizeof(writeBuffer))
		{
			const double ra_signed = atan2(j2000Pos[1], j2000Pos[0]);
			//Workaround for the discrepancy in precision between Windows/Linux/PPC Macs and Intel Macs:
			const double ra = (ra_signed >= 0) ? ra_signed : (ra_signed + 2.0 * M_PI);
			const double dec = atan2(j2000Pos[2], sqrt(j2000Pos[0]*j2000Pos[0]+j2000Pos[1]*j2000Pos[1]));
			unsigned int ra_int = (unsigned int)floor(0.5 + ra*(((unsigned int)0x80000000)/M_PI));
			int dec_int = (int)floor(0.5 + dec*(((unsigned int)0x80000000)/M_PI));
			
			gotoReceived(ra_int, dec_int);
		}
		/*
		else
		{
			qDebug() << "TelescopeTCP(" << name << ")::telescopeGoto: "<< "communication is too slow, I will ignore this command";
		}
		*/
	}
}

void TelescopeClientDirectNexStar::gotoReceived(unsigned int ra_int, int dec_int)
{
	nexstar->sendGoto(ra_int, dec_int);
}

//! estimates where the telescope is by interpolation in the stored
//! telescope positions:
Vec3d TelescopeClientDirectNexStar::getJ2000EquatorialPos(const StelNavigator*) const
{
	if (position_pointer->client_micros == 0x7FFFFFFFFFFFFFFFLL)
	{
		return Vec3d(0,0,0);
	}
	const qint64 now = getNow() - time_delay;
	const Position *p = position_pointer;
	do
	{
		const Position *pp = p;
		if (pp == positions) pp = end_position;
		pp--;
		if (pp->client_micros == 0x7FFFFFFFFFFFFFFFLL) break;
		if (pp->client_micros <= now && now <= p->client_micros)
		{
			if (pp->client_micros != p->client_micros)
			{
				Vec3d rval = p->pos * (now - pp->client_micros) + pp->pos * (p->client_micros - now);
				double f = rval.lengthSquared();
				if (f > 0.0)
				{
					return (1.0/sqrt(f))*rval;
				}
			}
			break;
		}
		p = pp;
	}
	while (p != position_pointer);
	return p->pos;
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
	*log_file << Now() << "TelescopeClientDirectNexStar::communicationResetReceived" << endl;
#endif
}

//! Called by NexStarCommandGetRaDec::readAnswerFromBuffer().
void TelescopeClientDirectNexStar::raReceived(unsigned int ra_int)
{
	last_ra = ra_int;
#ifndef QT_NO_DEBUG
	*log_file << Now() << "TelescopeClientDirectNexStar::raReceived: " << ra_int << endl;
#endif
}

//! Called by NexStarCommandGetRaDec::readAnswerFromBuffer().
//! Should be called after raReceived(), as it contains a call to sendPosition().
void TelescopeClientDirectNexStar::decReceived(unsigned int dec_int)
{
#ifndef QT_NO_DEBUG
	*log_file << Now() << "TelescopeClientDirectNexStar::decReceived: " << dec_int << endl;
#endif
	const int nexstar_status = 0;
	sendPosition(last_ra, dec_int, nexstar_status);
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
	const qint64 server_micros = (qint64) GetNow();
	
	// remember the time and received position so that later we
	// will know where the telescope is pointing to:
	position_pointer++;
	if (position_pointer >= end_position)
		position_pointer = positions;
	position_pointer->server_micros = server_micros;
	position_pointer->client_micros = getNow();
	const double ra  =  ra_int * (M_PI/(unsigned int)0x80000000);
	const double dec = dec_int * (M_PI/(unsigned int)0x80000000);
	const double cdec = cos(dec);
	position_pointer->pos[0] = cos(ra)*cdec;
	position_pointer->pos[1] = sin(ra)*cdec;
	position_pointer->pos[2] = sin(dec);
	position_pointer->status = status;
}
