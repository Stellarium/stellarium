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

#ifndef LX200COMMAND_HPP
#define LX200COMMAND_HPP

#include <QTextStream>
using namespace std;

class Server;
class TelescopeClientDirectLx200;

//! Abstract base class for Meade LX200 (and compatible) commands.
class Lx200Command
{
public:
	virtual ~Lx200Command(void) {}
	virtual bool writeCommandToBuffer(char *&buff, char *end) = 0;
	bool hasBeenWrittenToBuffer(void) const {return has_been_written_to_buffer;}
	virtual int readAnswerFromBuffer(const char *&buff, const char *end) = 0;
	virtual bool needsNoAnswer(void) const {return false;}
	virtual void print(QTextStream &o) const = 0;
	virtual bool isCommandGotoSelected(void) const {return false;}
	virtual bool shortAnswerReceived(void) const {return false;}
	//returns true when reading is finished
	
protected:
	Lx200Command(Server &server);
	TelescopeClientDirectLx200 &server;
	bool has_been_written_to_buffer;
};

inline QTextStream &operator<<(QTextStream &o, const Lx200Command &c)
{
	c.print(o);
	return o;
}

//! Meade LX200 command: Toggle long or short format.
//! Does not require an answer from the telescope.
class Lx200CommandToggleFormat : public Lx200Command
{
public:
	Lx200CommandToggleFormat(Server &server) : Lx200Command(server) {}
	
private:
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char*&, const char*) {return 1;}
	bool needsNoAnswer(void) const {return true;}
	void print(QTextStream &o) const;
};

//! Meade LX200 command: Stop the current slew.
//! Does not require an answer from the telescope.
class Lx200CommandStopSlew : public Lx200Command
{
public:
	Lx200CommandStopSlew(Server &server) : Lx200Command(server) {}
	
private:
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char*&, const char*) {return 1;}
	bool needsNoAnswer(void) const {return true;}
	void print(QTextStream &o) const;
};

//! Meade LX200 command: Set right ascension. LONG FORMAT ONLY!
class Lx200CommandSetSelectedRa : public Lx200Command
{
public:
	Lx200CommandSetSelectedRa(Server &server, int ra)
	                         : Lx200Command(server), ra(ra) {}
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end);
	void print(QTextStream &o) const;
	
private:
	const int ra;
};

//! Meade LX200 command: Set declination. LONG FORMAT ONLY!
class Lx200CommandSetSelectedDec : public Lx200Command
{
public:
	Lx200CommandSetSelectedDec(Server &server,int dec)
	                          : Lx200Command(server), dec(dec) {}
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end);
	void print(QTextStream &o) const;
	
private:
	const int dec;
};

//! Meade LX200 command: Slew to the coordinates set before.
class Lx200CommandGotoSelected : public Lx200Command
{
public:
	Lx200CommandGotoSelected(Server &server)
	                        : Lx200Command(server), first_byte(256) {}
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end);
	void print(QTextStream &o) const;
	bool isCommandGotoSelected(void) const {return true;}
	bool shortAnswerReceived(void) const {return (first_byte != 256);}
	
private:
	int first_byte;
};

class Lx200CommandSyncSelected : public Lx200Command
{
public:
	Lx200CommandSyncSelected(Server &server)
				: Lx200Command(server), first_byte(256) {}
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end);
	void print(QTextStream &o) const;
	bool isCommandGotoSelected(void) const {return true;}
	bool shortAnswerReceived(void) const {return (first_byte != 256);}

private:
	int first_byte;
};

//! Meade LX200 command: Get the current right ascension.
class Lx200CommandGetRa : public Lx200Command
{
public:
	Lx200CommandGetRa(Server &server) : Lx200Command(server) {}
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end);
	void print(QTextStream &o) const;
};

//! Meade LX200 command: Get the current declination.
class Lx200CommandGetDec : public Lx200Command
{
public:
	Lx200CommandGetDec(Server &server) : Lx200Command(server) {}
	bool writeCommandToBuffer(char *&buff, char *end);
	int readAnswerFromBuffer(const char *&buff, const char *end);
	void print(QTextStream &o) const;
};

#endif // LX200COMMAND_HPP
