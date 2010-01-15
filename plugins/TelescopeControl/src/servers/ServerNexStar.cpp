/*
The stellarium telescope library helps building
telescope server programs, that can communicate with stellarium
by means of the stellarium TCP telescope protocol.
It also contains smaple server classes (dummy, Meade LX200).

Author and Copyright of this file and of the stellarium telescope library:
Johannes Gajdosik, 2006, modified for NexStar telescopes by Michael Heinz.

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

#include "ServerNexStar.hpp"
#include "NexStarConnection.hpp"
#include "NexStarCommand.hpp"
#include "LogFile.hpp"

#include <stdlib.h> // exit

ServerNexStar::ServerNexStar(int port, const char *serial_device) : Server(port), nexstar(0)
{
	nexstar = new NexStarConnection(*this,serial_device);
	if (nexstar->isClosed())
	{
		exit(125);
	}
	// nexstar will be deleted in the destructor of Server
	addConnection(nexstar);
	last_ra = 0;
	queue_get_position = true;
	next_pos_time = -0x8000000000000000LL;
}

void ServerNexStar::gotoReceived(unsigned int ra_int, int dec_int)
{
	nexstar->sendGoto(ra_int, dec_int);
}

void ServerNexStar::communicationResetReceived(void)
{
	queue_get_position = true;
	next_pos_time = -0x8000000000000000LL;
}

void ServerNexStar::raReceived(unsigned int ra_int)
{
	last_ra = ra_int;
#ifdef DEBUG3
	*log_file << Now() << "ServerNexStar::raReceived: " << ra_int << endl;
#endif
}

void ServerNexStar::decReceived(unsigned int dec_int)
{
#ifdef DEBUG3
	*log_file << Now() << "ServerNexStar::decReceived: " << dec_int << endl;
#endif
	const int nexstar_status = 0;
	sendPosition(last_ra, dec_int, nexstar_status);
	queue_get_position = true;
}

void ServerNexStar::step(long long int timeout_micros)
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
