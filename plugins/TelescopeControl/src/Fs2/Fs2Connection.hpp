/*
The stellarium telescope library helps building
telescope server programs, that can communicate with stellarium
by means of the stellarium TCP telescope protocol.
It also contains sample server classes (dummy, Astro Electronic FS2).

Author and Copyright of this file and of the stellarium telescope library:
Johannes Gajdosik, 2006

Astro Electronic FS2 support by Michael Butti, 2026.

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

#ifndef FS2CONNECTION_HPP
#define FS2CONNECTION_HPP

#include "common/SerialPort.hpp"

#include <list>

class Fs2Command;
class Server;

//! Interface to a Astro Electronic FS2 (or compatible) telescope.
class Fs2Connection
{
public:
	virtual ~Fs2Connection() {}
	virtual void sendGoto(unsigned int ra_int, int dec_int) = 0;
	virtual void sendSync(unsigned int ra_int, int dec_int) = 0;
	virtual void sendCommand(Fs2Command *command) = 0;
	//! Delete pending commands and send abort signal.
	virtual void sendAbort() = 0;
	//! Returns true if the underlying connection is closed.
	virtual bool isClosed(void) const = 0;
};

//! Implements the Astro Electronic FS2 command protocol on top of a
//! transport (@p Transport must be a Connection; only SerialPort is used).
template <class Transport>
class Fs2ConnectionImpl : public Transport, public Fs2Connection
{
public:
	template <class... TransportArgs>
	Fs2ConnectionImpl(Server &server, TransportArgs... transport_args)
		: Transport(server, transport_args...)
	{
		initTiming();
	}

	void sendGoto(unsigned int ra_int, int dec_int) override;
	void sendSync(unsigned int ra_int, int dec_int) override;
	void sendCommand(Fs2Command *command) override;
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
	std::list<Fs2Command*> command_list;
	long long int time_between_commands;
	long long int next_send_time;
	long long int read_timeout_endtime;
	int goto_commands_queued;
};

//! Serial port connection to a Astro Electronic FS2 or a compatible telescope.
class Fs2SerialConnection : public Fs2ConnectionImpl<SerialPort>
{
public:
	Fs2SerialConnection(Server &server, const char *serial_device)
		: Fs2ConnectionImpl<SerialPort>(server, serial_device) {}
};

#endif // FS2CONNECTION_HPP
