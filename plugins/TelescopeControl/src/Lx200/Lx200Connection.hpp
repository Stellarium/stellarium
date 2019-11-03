/*
The stellarium telescope library helps building
telescope server programs, that can communicate with stellarium
by means of the stellarium TCP telescope protocol.
It also contains smaple server classes (dummy, Meade LX200).

Author and Copyright of this file and of the stellarium telescope library:
Johannes Gajdosik, 2006

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#ifndef LX200CONNECTION_HPP
#define LX200CONNECTION_HPP

#include "common/SerialPort.hpp"

#include <list>
using namespace std;

class Lx200Command;

//! Serial port connection to a Meade LX200 or a compatible telescope.
class Lx200Connection : public SerialPort
{
public:
	Lx200Connection(Server &server, const char *serial_device);
	void sendGoto(unsigned int ra_int, int dec_int);
	void sendSync(unsigned int ra_int, int dec_int);
	void sendCommand(Lx200Command * command);
	void setTimeBetweenCommands(long long int micro_seconds)
	{
		time_between_commands = micro_seconds;
	}
	
private:
	//! Parses read buffer data received from the telescope.
	void dataReceived(const char *&p, const char *read_buff_end);
	//! Not implemented, as this is not a connection to a client.
	void sendPosition(unsigned int ra_int, int dec_int, int status) {Q_UNUSED(ra_int); Q_UNUSED(dec_int); Q_UNUSED(status);}
	void resetCommunication(void);
	void prepareSelectFds(fd_set &read_fds, fd_set &write_fds, int &fd_max);
	bool writeFrontCommandToBuffer(void);
	//! Flushes the command queue, sending commands to the write buffer.
	//! This method iterates over the queue, writing to the write buffer
	//! as many commands as possible, until it reaches a command that
	//! requires an answer.
	void flushCommandList(void);
	
private:
	list<Lx200Command*> command_list;
	long long int time_between_commands;
	long long int next_send_time;
	long long int read_timeout_endtime;
	int goto_commands_queued;
};

#endif // LX200CONNECTION_HPP
