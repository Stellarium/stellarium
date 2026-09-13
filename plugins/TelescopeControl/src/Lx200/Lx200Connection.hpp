/*
The stellarium telescope library helps building
telescope server programs, that can communicate with stellarium
by means of the stellarium TCP telescope protocol.
It also contains sample server classes (dummy, Meade LX200).

Author and Copyright of this file and of the stellarium telescope library:
Johannes Gajdosik, 2006

TCP transport support added by Rumen Bogdanovski, 2026.

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
#include "common/TcpConnection.hpp"

#include <list>

class Lx200Command;
class Server;

//! Transport-independent interface to a Meade LX200 (or compatible) telescope.
//!
//! The Meade LX200 ASCII protocol can be spoken either over a serial port or
//! over a TCP/IP connection. TelescopeClientDirectLx200 talks to the telescope
//! through this interface without needing to know which transport is in use.
class Lx200Connection
{
public:
	virtual ~Lx200Connection() {}
	virtual void sendGoto(unsigned int ra_int, int dec_int) = 0;
	virtual void sendSync(unsigned int ra_int, int dec_int) = 0;
	virtual void sendCommand(Lx200Command *command) = 0;
	//! Delete pending commands and send abort signal.
	virtual void sendAbort() = 0;
	//! Returns true if the underlying connection is closed.
	virtual bool isClosed(void) const = 0;
};

//! Implements the Meade LX200 command protocol on top of an arbitrary
//! transport (@p Transport must be a Connection: either SerialPort or
//! TcpConnection). All the protocol logic lives here and is shared between the
//! serial and the network variants; only the transport differs.
template <class Transport>
class Lx200ConnectionImpl : public Transport, public Lx200Connection
{
public:
	template <class... TransportArgs>
	Lx200ConnectionImpl(Server &server, TransportArgs... transport_args)
		: Transport(server, transport_args...)
	{
		initTiming();
	}

	void sendGoto(unsigned int ra_int, int dec_int) override;
	void sendSync(unsigned int ra_int, int dec_int) override;
	void sendCommand(Lx200Command *command) override;
	void sendAbort() override;
	bool isClosed(void) const override {return Transport::isClosed();}
	void setTimeBetweenCommands(long long int micro_seconds)
	{
		time_between_commands = micro_seconds;
	}

private:
	void initTiming(void);
	//! Parses read buffer data received from the telescope.
	void dataReceived(const char *&p, const char *read_buff_end) override;
	//! Not implemented, as this is not a connection to a client.
	void sendPosition(unsigned int ra_int, int dec_int, int status) override {Q_UNUSED(ra_int) Q_UNUSED(dec_int) Q_UNUSED(status)}
	void resetCommunication(void);
	void prepareSelectFds(fd_set &read_fds, fd_set &write_fds, int &fd_max) override;
	bool writeFrontCommandToBuffer(void);
	//! Flushes the command queue, sending commands to the write buffer.
	//! This method iterates over the queue, writing to the write buffer
	//! as many commands as possible, until it reaches a command that
	//! requires an answer.
	void flushCommandList(void);

private:
	std::list<Lx200Command*> command_list;
	long long int time_between_commands;
	long long int next_send_time;
	long long int read_timeout_endtime;
	int goto_commands_queued;
};

//! Serial port connection to a Meade LX200 or a compatible telescope.
class Lx200SerialConnection : public Lx200ConnectionImpl<SerialPort>
{
public:
	Lx200SerialConnection(Server &server, const char *serial_device)
		: Lx200ConnectionImpl<SerialPort>(server, serial_device) {}
};

//! TCP/IP connection to a Meade LX200 or a compatible telescope.
class Lx200TcpConnection : public Lx200ConnectionImpl<TcpConnection>
{
public:
	Lx200TcpConnection(Server &server, const char *host, int port)
		: Lx200ConnectionImpl<TcpConnection>(server, host, port) {}
};

#endif // LX200CONNECTION_HPP
