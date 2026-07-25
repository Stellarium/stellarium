/*
The stellarium telescope library helps building
telescope server programs, that can communicate with stellarium
by means of the stellarium TCP telescope protocol.
It also contains sample server classes (dummy, Meade LX200).

Author and Copyright of this file:
Rumen Bogdanovski, 2026

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

#ifndef TCPCONNECTION_HPP
#define TCPCONNECTION_HPP

#include "Connection.hpp"

//! Outgoing TCP/IP client connection to a device that speaks an ASCII
//! telescope protocol (such as a Meade LX200 exposed over a network bridge).
//!
//! Unlike Connection, whose constructor wraps an already-accepted socket,
//! TcpConnection actively resolves @p host and connects to @p port. Once
//! connected it reuses all of Connection's cross-platform, select()-based
//! reading and writing. The socket is switched to non-blocking mode, but the
//! initial connection attempt is performed synchronously (with a timeout) so
//! that isClosed() reflects the real connection state right after construction,
//! mirroring the behaviour of SerialPort.
class TcpConnection : public Connection
{
public:
	//! @param host host name or IPv4/IPv6 address of the telescope.
	//! @param port TCP port the telescope is listening on.
	TcpConnection(Server &server, const char *host, int port);

private:
	//! An ASCII protocol runs on top of this connection, so allow the
	//! DEBUG5 logging in Connection to dump the traffic as text.
	bool isAsciiConnection(void) const override {return true;}
};

#endif // TCPCONNECTION_HPP
