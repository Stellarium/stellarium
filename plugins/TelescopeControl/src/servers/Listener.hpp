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

#ifndef _LISTENER_HPP_
#define _LISTENER_HPP_

#include "Socket.hpp"

//! Listens for connection attempts and adds new connections to the server.
//! Every Server object contains an instance of Listener, initialized with
//! a reference to that Server object and the TCP/IP port used by the server.
//! Once started, the Listener object listens at the port for connection
//! attempts. If a TCP/IP connection is established, the listener adds it
//! to the list of connections maintained by the server.
class Listener : public Socket
{
public:
	//! @param port TCP/IP port number
	Listener(Server &server, int port) : Socket(server, INVALID_SOCKET),
	                                     port(port) {}
	
private:
	//! Checks if the connection is closed.
	//! @returns always false, because if Listener can't listen,
	//! it causes the program to exit with code 127.
	bool isClosed(void) const {return false;}
	//! Prepares TCP/IP communication and tries to start listening.
	//! If it can't listen, it causes the program to exit with code 127.
	void prepareSelectFds(fd_set &read_fds, fd_set &write_fds, int &fd_max);
	//! Performs TCP/IP communication and handles new connections.
	//! If a new connection is established, creates a new Connection object
	//! and passes it to the parent Server with Server::addConnection().
	void handleSelectFds(const fd_set &read_fds, const fd_set &write_fds);
	
private:
	const int port;
};

#endif
