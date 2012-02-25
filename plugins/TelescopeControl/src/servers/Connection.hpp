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

#ifndef _CONNECTION_HPP_
#define _CONNECTION_HPP_

#include "Socket.hpp"

//! TCP/IP connection to a client.
class Connection : public Socket
{
public:
	Connection(Server &server, SOCKET fd);
	long long int getServerMinusClientTime(void) const
	{
		return server_minus_client_time;
	}
	
protected:
	//! Receives data from a TCP/IP connection and stores it in the read buffer.
	void performReading(void);
	//! Sends the contents of the write buffer over a TCP/IP connection.
	void performWriting(void);
	void prepareSelectFds(fd_set &read_fds, fd_set &write_fds, int &fd_max);
	
private:
	//! Returns true, as by default Connection implements a TCP/IP connection.
	virtual bool isTcpConnection(void) const {return true;}
	//! Returns false, as by default Connection implements a TCP/IP connection.
	virtual bool isAsciiConnection(void) const {return false;}
	void handleSelectFds(const fd_set &read_fds, const fd_set &write_fds);
	//! Parses the read buffer and handles any messages contained within it.
	//! If the data contains a Stellarium telescope control command,
	//! dataReceived() calls the appropriate method of Server.
	//! For example, "MessageGoto" (type 0) causes a call to Server::gotoReceived().
	virtual void dataReceived(const char *&p, const char *read_buff_end);
	//! Composes a "MessageCurrentPosition" in the write buffer.
	//! This is a Stellarium telescope control protocol message containing
	//! the current right ascension, declination and status of the telescope mount.
	void sendPosition(unsigned int ra_int, int dec_int, int status);
	
protected:
	char read_buff[120];
	char *read_buff_end;
	char write_buff[120];
	char *write_buff_end;
	
private:
	long long int server_minus_client_time;
};

#endif //_CONNECTION_HPP_
