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

#ifndef _SERVER_LX200_HPP_
#define _SERVER_LX200_HPP_

#include "Server.hpp"

class Lx200Connection;

//! Telescope server class for a Meade LX200 or a compatible telescope.
class ServerLx200 : public Server
{
public:
	ServerLx200(int port, const char *serial_device);
	void step(long long int timeout_micros);
	void communicationResetReceived(void);
	void longFormatUsedReceived(bool long_format);
	void raReceived(unsigned int ra_int);
	void decReceived(unsigned int dec_int);
	
private:
	void gotoReceived(unsigned int ra_int,int dec_int);
	
private:
	Lx200Connection *lx200;
	bool long_format_used;
	unsigned int last_ra;
	bool queue_get_position;
	bool answers_received;
	long long int next_pos_time;
};

#endif
