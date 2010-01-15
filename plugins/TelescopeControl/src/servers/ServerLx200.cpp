/*
The stellarium telescope library helps building
telescope server programs, that can communicate with stellarium
by means of the stellarium TCP telescope protocol.
It also contains smaple server classes (dummy, Meade LX200).

Author and Copyright of this file and of the stellarium telescope library:
Johannes Gajdosik, 2006

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "ServerLx200.hpp"
#include "Lx200Connection.hpp"
#include "Lx200Command.hpp"
#include "LogFile.hpp"

#include <stdlib.h> // exit

ServerLx200::ServerLx200(int port, const char *serial_device) : Server(port),
                                                                lx200(0)
{
	/*
	Bogdan Marinov: This instantiation and the if-clause below
	was the reason for the infamous bug that caused this telescope server to
	exit immediately on Windows. Socket::isClosed() checks only the file
	descriptor, but a file descriptor is not used in SerialPort on Windows.
	This has been fixed temporarily by overriding isClosed() in
	SerialPort::isClosed().
	*/
	
	lx200 = new Lx200Connection(*this, serial_device);
	if (lx200->isClosed())
	{
		exit(125);
	}
	
	// lx200 will be deleted in the destructor of Server
	addConnection(lx200);
	long_format_used = false; // unknown
	last_ra = 0;
	queue_get_position = true;
	next_pos_time = -0x8000000000000000LL;
	answers_received = false;
}

void ServerLx200::gotoReceived(unsigned int ra_int, int dec_int)
{
	lx200->sendGoto(ra_int, dec_int);
}

void ServerLx200::communicationResetReceived(void)
{
	long_format_used = false;
	queue_get_position = true;
	next_pos_time = -0x8000000000000000LL;
	
#ifdef DEBUG3
	*log_file << Now() << "ServerLx200::communicationResetReceived" << endl;
#endif

	if (answers_received)
	{
		closeAcceptedConnections();
		answers_received = false;
	}
}

//! Called in Lx200CommandGetRa and Lx200CommandGetDec.
void ServerLx200::longFormatUsedReceived(bool long_format)
{
	answers_received = true;
	if (!long_format_used && !long_format)
	{
		lx200->sendCommand(new Lx200CommandToggleFormat(*this));
	}
	long_format_used = true;
}

//! Called by Lx200CommandGetRa::readAnswerFromBuffer().
void ServerLx200::raReceived(unsigned int ra_int)
{
	answers_received = true;
	last_ra = ra_int;
#ifdef DEBUG3
	*log_file << Now() << "ServerLx200::raReceived: " << ra_int << endl;
#endif
}

//! Called by Lx200CommandGetDec::readAnswerFromBuffer().
//! Should be called after raReceived(), as it contains a call to sendPosition().
void ServerLx200::decReceived(unsigned int dec_int)
{
	answers_received = true;
#ifdef DEBUG3
	*log_file << Now() << "ServerLx200::decReceived: " << dec_int << endl;
#endif
	const int lx200_status = 0;
	sendPosition(last_ra, dec_int, lx200_status);
	queue_get_position = true;
}

void ServerLx200::step(long long int timeout_micros)
{
	long long int now = GetNow();
	if (queue_get_position && now >= next_pos_time)
	{
		lx200->sendCommand(new Lx200CommandGetRa(*this));
		lx200->sendCommand(new Lx200CommandGetDec(*this));
		queue_get_position = false;
		next_pos_time = now + 500000;// 500000;
	}
	Server::step(timeout_micros);
}
