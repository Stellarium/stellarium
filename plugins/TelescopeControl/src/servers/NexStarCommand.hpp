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

#ifndef _NEXSTAR_COMMAND_HPP_
#define _NEXSTAR_COMMAND_HPP_

#include <iostream>
using namespace std;

class Server;
class ServerNexStar;

//! Abstract base class for Celestron NexStar (and compatible) commands.
class NexStarCommand
{
public:
	virtual ~NexStarCommand(void) {}
	virtual bool writeCommandToBuffer(char *&buff, char *end) = 0;
	bool hasBeenWrittenToBuffer(void) const { return has_been_written_to_buffer; }
	virtual int readAnswerFromBuffer(const char *&buff, const char *end) const = 0;
	virtual bool needsNoAnswer(void) const { return false; }
	virtual void print(ostream &o) const = 0;
	// returns true when reading is finished
	
protected:
	NexStarCommand(Server &server);
	ServerNexStar &server;
	bool has_been_written_to_buffer;
};

inline ostream &operator<<(ostream &o,const NexStarCommand &c)
{
	c.print(o);
	return o;
}

//! Celestron NexStar command: Slew to a given position.
class NexStarCommandGotoPosition : public NexStarCommand
{
public:
	NexStarCommandGotoPosition(Server &server, unsigned int ra_int, int dec_int);
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end) const;
	void print(ostream &o) const;
	
private:
	int ra, dec;
};

//! Celestron NexStar command: Get the current position.
class NexStarCommandGetRaDec : public NexStarCommand
{
public:
	NexStarCommandGetRaDec(Server &server) : NexStarCommand(server) {}
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end) const;
	void print(ostream &o) const;
};

#endif
