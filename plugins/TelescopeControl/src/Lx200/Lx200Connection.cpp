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

#include "Lx200Connection.hpp"
#include "Lx200Command.hpp"
#include "TelescopeClientDirectLx200.hpp"
#include "common/LogFile.hpp"
#include "StelUtils.hpp"

#include <cmath>


template <class Transport>
void Lx200ConnectionImpl<Transport>::initTiming(void)
{
	time_between_commands = 0;
	next_send_time = GetNow();
	read_timeout_endtime = 0x7FFFFFFFFFFFFFFFLL;
	goto_commands_queued = 0;
}

//! Resets the connection.
//! Removes all commands in the queue without executing them and
//! cleans both the read and the write buffers.
template <class Transport>
void Lx200ConnectionImpl<Transport>::resetCommunication(void)
{
	while (!command_list.empty())
	{
		delete command_list.front();
		command_list.pop_front();
	}

	this->read_buff_end = this->read_buff;
	this->write_buff_end = this->write_buff;
	#ifdef DEBUG4
	*log_file << Now() << "Lx200Connection::resetCommunication" << StelUtils::getEndLineChar();
	#endif
	// wait 10 seconds before sending the next command in order to read
	// and ignore data coming from the telescope:
	next_send_time = GetNow() + 10000000;
	read_timeout_endtime = 0x7FFFFFFFFFFFFFFFLL;
	goto_commands_queued = 0;
	static_cast<TelescopeClientDirectLx200&>(this->server).communicationResetReceived();
}

//! Commands the telescope to slew to the given right ascension and declination.
//! Converts the coordinates and queues the necessary sequence of Lx200Command
//! objects to perform the slew.
template <class Transport>
void Lx200ConnectionImpl<Transport>::sendGoto(unsigned int ra_int, int dec_int)
{
	if (goto_commands_queued <= 1)
	{
		int dec = static_cast<int>(std::floor(0.5 + dec_int * (360*3600/4294967296.0)));
		if (dec < -90*3600)
		{
			dec = -180*3600 - dec;
			ra_int += 0x80000000;
		}
		else if (dec > 90*3600)
		{
			dec = 180*3600 - dec;
			ra_int += 0x80000000;
		}
		int ra = static_cast<int>(std::floor(0.5 + ra_int * (86400.0/4294967296.0)));
		if (ra >= 86400)
			ra -= 86400;
		sendCommand(new Lx200CommandStopSlew(this->server));
		sendCommand(new Lx200CommandSetSelectedRa(this->server, ra));
		sendCommand(new Lx200CommandSetSelectedDec(this->server, dec));
		sendCommand(new Lx200CommandGotoSelected(this->server));
		goto_commands_queued++;
	}
	else
	{
		#ifdef DEBUG4
		*log_file << Now() << "Lx200Connection::sendGoto: ignoring command" << StelUtils::getEndLineChar();
		#endif
	}
}

template <class Transport>
void Lx200ConnectionImpl<Transport>::sendSync(unsigned int ra_int, int dec_int)
{
	if (goto_commands_queued <= 1)
	{
		int dec = static_cast<int>(std::floor(0.5 + dec_int * (360*3600/4294967296.0)));
		if (dec < -90*3600)
		{
			dec = -180*3600 - dec;
			ra_int += 0x80000000;
		}
		else if (dec > 90*3600)
		{
			dec = 180*3600 - dec;
			ra_int += 0x80000000;
		}
		int ra = static_cast<int>(std::floor(0.5 + ra_int * (86400.0/4294967296.0)));
		if (ra >= 86400)
			ra -= 86400;
		sendCommand(new Lx200CommandStopSlew(this->server));
		sendCommand(new Lx200CommandSetSelectedRa(this->server, ra));
		sendCommand(new Lx200CommandSetSelectedDec(this->server, dec));
		sendCommand(new Lx200CommandSyncSelected(this->server));
		goto_commands_queued++;
	}
	else
	{
		#ifdef DEBUG4
		*log_file << Now() << "Lx200Connection::sendGoto: ignoring command" << StelUtils::getEndLineChar();
		#endif
	}
}


template <class Transport>
bool Lx200ConnectionImpl<Transport>::writeFrontCommandToBuffer(void)
{
	if(command_list.empty())
	{
		return false;
	}

	const long long int now = GetNow();
	if (now < next_send_time)
	{
#ifdef DEBUG4
		/*
		*log_file << Now() << "Lx200Connection::writeFrontCommandToBuffer("
		                   << (*command_list.front()) << "): delayed for "
				   << (next_send_time-now) << StelUtils::getEndLineChar();
		*/
#endif
		return false;
	}

	const bool rval = command_list.front()->writeCommandToBuffer(this->write_buff_end, this->write_buff+sizeof(this->write_buff));
	if (rval)
	{
		next_send_time = now;
		if (command_list.front()->needsNoAnswer())
		{
			next_send_time += time_between_commands;
			read_timeout_endtime = 0x7FFFFFFFFFFFFFFFLL;
		}
		else
		{
			if (command_list.front()->isCommandGotoSelected())
			{
				// shorter timeout for AutoStar 494 slew:
				read_timeout_endtime = now + 3000000;
			}
			else
			{
				// extra long timeout for AutoStar 494:
				read_timeout_endtime = now + 5000000;
			}
		}
		#ifdef DEBUG4
		*log_file << Now()
		          << "Lx200Connection::writeFrontCommandToBuffer("
		          << (*command_list.front())
		          << "): queued"
			  << StelUtils::getEndLineChar();
		#endif
	}

	return rval;
}

template <class Transport>
void Lx200ConnectionImpl<Transport>::dataReceived(const char *&p, const char *read_buff_end)
{
	if (this->isClosed())
	{
		*log_file << Now() << "Lx200Connection::dataReceived: strange: fd is closed" << StelUtils::getEndLineChar();
	}
	else if (command_list.empty())
	{
		// No command is waiting for an answer. The LX200 protocol is strictly
		// request/response, so any bytes arriving here are leftovers from a
		// command that was already discarded - typically the answer to a
		// position query that was dropped by sendAbort(). Ignore them instead
		// of forcing a full communication reset: the reset imposes a multi-
		// second pause before the next command is sent, which is long enough
		// for a networked LX200 server to hit its read timeout and close the
		// connection. A genuine desync is still caught later, when the answer
		// to an actual queued command fails to parse.
		#ifdef DEBUG4
		*log_file << Now()
		          << "Lx200Connection::dataReceived: ignoring unexpected data (command_list is empty)"
			  << StelUtils::getEndLineChar();
		#endif
		p = read_buff_end;
	}
	else if (command_list.front()->needsNoAnswer())
	{
		*log_file << Now() << "Lx200Connection::dataReceived: "
		                      "strange: command("
		                   << *command_list.front()
		                   << ") needs no answer"
				   << StelUtils::getEndLineChar();
		p = read_buff_end;
	}
	else
	{
		while(true)
		{
			if (!command_list.front()->hasBeenWrittenToBuffer())
			{
				*log_file << Now()
				          << "Lx200Connection::dataReceived: "
				             "strange: no answer expected"
					  << StelUtils::getEndLineChar();
				p = read_buff_end;
				break;
			}
			const int rc=command_list.front()->readAnswerFromBuffer(p, read_buff_end);
			/*
			*log_file << Now()
			          << "Lx200Connection::dataReceived: "
			          << *command_list.front()
			          << "->readAnswerFromBuffer returned "
			          << rc
				  << StelUtils::getEndLineChar();
			*/
			if (rc <= 0)
			{
				if (rc < 0)
				{
					resetCommunication();
				}
				break;
			}
			if (command_list.front()->isCommandGotoSelected())
			{
				goto_commands_queued--;
			}
			delete command_list.front();
			command_list.pop_front();
			read_timeout_endtime = 0x7FFFFFFFFFFFFFFFLL;
			if (!writeFrontCommandToBuffer())
				break;
		}
	}
}

template <class Transport>
void Lx200ConnectionImpl<Transport>::prepareSelectFds(fd_set &read_fds,
                                                      fd_set &write_fds,
                                                      int &fd_max)
{
	// if some telegram is delayed try to queue it now:
	flushCommandList();
	if (!command_list.empty() && GetNow() > read_timeout_endtime)
	{
		if (command_list.front()->shortAnswerReceived())
		{
			// the lazy telescope, probably AutoStar 494
			// has not sent the full answer
			#ifdef DEBUG4
			*log_file << Now() << "Lx200Connection::prepareSelectFds: "
			                      "dequeueing command("
			                   << *command_list.front()
			                   << ") because of timeout"
					   << StelUtils::getEndLineChar();
			#endif
			if (command_list.front()->isCommandGotoSelected())
			{
				goto_commands_queued--;
			}
			delete command_list.front();
			command_list.pop_front();
			read_timeout_endtime = 0x7FFFFFFFFFFFFFFFLL;
		}
		else
		{
			resetCommunication();
		}
	}
	Transport::prepareSelectFds(read_fds, write_fds, fd_max);
}

template <class Transport>
void Lx200ConnectionImpl<Transport>::flushCommandList(void)
{
	if (!command_list.empty())
	{
		while (!command_list.front()->hasBeenWrittenToBuffer())
		{
			if (writeFrontCommandToBuffer())
			{
				//*log_file << Now()
				//          << "Lx200Connection::flushCommandList: "
				//          << (*command_list.front())
				//          << "::writeFrontCommandToBuffer ok"
				//          << StelUtils::getEndLineChar();
				if (command_list.front()->needsNoAnswer())
				{
					delete command_list.front();
					command_list.pop_front();
					read_timeout_endtime = 0x7FFFFFFFFFFFFFFFLL;
					if (command_list.empty())
						break;
				}
				else
				{
					break;
				}
			}
			else
			{
				//*log_file << Now() << "Lx200Connection::flushCommandList: "
				//                   << (*command_list.front())
				//                   << "::writeFrontCommandToBuffer failed/delayed" << StelUtils::getEndLineChar();
				break;
			}
		}
	}
}

template <class Transport>
void Lx200ConnectionImpl<Transport>::sendCommand(Lx200Command *command)
{
	if (command)
	{
		#ifdef DEBUG4
		*log_file << Now()
		          << "Lx200Connection::sendCommand("
		          << *command
		          << ")"
			  << StelUtils::getEndLineChar();
		#endif
		command_list.push_back(command);
		flushCommandList();
		//*log_file << Now() << "Lx200Connection::sendCommand(" << *command << ") end"
		//          << StelUtils::getEndLineChar();
	}
}

template <class Transport>
void Lx200ConnectionImpl<Transport>::sendAbort()
{
#ifdef DEBUG4
	*log_file << Now()
		  << "Lx200Connection::sendAbort()"
		  << StelUtils::getEndLineChar();
#endif
	// Remove queued commands (pointers, so delete their objects!)
	while (!command_list.empty())
	{
		delete command_list.front();
		command_list.pop_front();
	}

	this->read_buff_end = this->read_buff;
	this->write_buff_end = this->write_buff;
	command_list.push_back(new Lx200CommandStopSlew(this->server));
	flushCommandList();
	//*log_file << Now() << "Lx200Connection::sendAbort() end"
	//          << StelUtils::getEndLineChar();
}

// Explicitly instantiate the two transports we support: serial and TCP/IP.
template class Lx200ConnectionImpl<SerialPort>;
template class Lx200ConnectionImpl<TcpConnection>;
