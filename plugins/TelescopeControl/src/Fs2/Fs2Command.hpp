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

#ifndef FS2COMMAND_HPP
#define FS2COMMAND_HPP

#include <QDebug>

class Server;
class TelescopeClientDirectFs2;

//! Abstract base class for Astro Electronic FS2 (and compatible) commands.
class Fs2Command
{
public:
	virtual ~Fs2Command(void) {}
	virtual bool writeCommandToBuffer(char *&buff, char *end) = 0;
	bool hasBeenWrittenToBuffer(void) const {return has_been_written_to_buffer;}
	virtual int readAnswerFromBuffer(const char *&buff, const char *end) = 0;
	virtual bool needsNoAnswer(void) const {return false;}
	virtual void print(QDebug &o) const = 0;
	virtual bool isCommandGotoSelected(void) const {return false;}
	virtual bool shortAnswerReceived(void) const {return false;}
	//returns true when reading is finished

protected:
	Fs2Command(Server &server);
	TelescopeClientDirectFs2 &server;
	bool has_been_written_to_buffer;
};

inline QDebug operator<<(QDebug o, const Fs2Command &c)
{
	c.print(o);
	return o;
}

//! Astro Electronic FS2 command: Toggle long or short format.
//! Does not require an answer from the telescope.
class Fs2CommandToggleFormat : public Fs2Command
{
public:
	Fs2CommandToggleFormat(Server &server) : Fs2Command(server) {}

private:
	bool writeCommandToBuffer(char *&buff, char *end) override;
	int readAnswerFromBuffer(const char*&, const char*) override {return 1;}
	bool needsNoAnswer(void) const override {return true;}
	void print(QDebug &o) const override;
};

//! Astro Electronic FS2 command: Stop the current slew.
//! Does not require an answer from the telescope.
class Fs2CommandStopSlew : public Fs2Command
{
public:
	Fs2CommandStopSlew(Server &server) : Fs2Command(server) {}

private:
	bool writeCommandToBuffer(char *&buff, char *end) override;
	int readAnswerFromBuffer(const char*&, const char*) override {return 1;}
	bool needsNoAnswer(void) const override {return true;}
	void print(QDebug &o) const override;
};

//! Astro Electronic FS2 command: Set right ascension. LONG FORMAT ONLY!
class Fs2CommandSetSelectedRa : public Fs2Command
{
public:
	Fs2CommandSetSelectedRa(Server &server, int ra) : Fs2Command(server), ra(ra) {}
	bool writeCommandToBuffer(char *&buff, char *end) override;
	int readAnswerFromBuffer(const char *&buff, const char *end) override;
	void print(QDebug &o) const override;

private:
	const int ra;
};

//! Astro Electronic FS2 command: Set declination. LONG FORMAT ONLY!
class Fs2CommandSetSelectedDec : public Fs2Command
{
public:
	Fs2CommandSetSelectedDec(Server &server,int dec) : Fs2Command(server), dec(dec) {}
	bool writeCommandToBuffer(char *&buff, char *end) override;
	int readAnswerFromBuffer(const char *&buff, const char *end) override;
	void print(QDebug &o) const override;

private:
	const int dec;
};

//! Astro Electronic FS2 command: Slew to the coordinates set before.
class Fs2CommandGotoSelected : public Fs2Command
{
public:
	Fs2CommandGotoSelected(Server &server) : Fs2Command(server), first_byte(256) {}
	bool writeCommandToBuffer(char *&buff, char *end) override;
	int readAnswerFromBuffer(const char *&buff, const char *end) override;
	void print(QDebug &o) const override;
	bool isCommandGotoSelected(void) const override {return true;}
	bool shortAnswerReceived(void) const override {return (first_byte != 256);}

private:
	int first_byte;
};

class Fs2CommandSyncSelected : public Fs2Command
{
public:
	Fs2CommandSyncSelected(Server &server) : Fs2Command(server), first_byte(256) {}
	bool writeCommandToBuffer(char *&buff, char *end) override;
	int readAnswerFromBuffer(const char *&buff, const char *end) override;
	void print(QDebug &o) const override;
	bool isCommandGotoSelected(void) const override {return true;}
	bool shortAnswerReceived(void) const override {return (first_byte != 256);}

private:
	int first_byte;
};

//! Astro Electronic FS2 command: Get the current right ascension.
class Fs2CommandGetRa : public Fs2Command
{
public:
	Fs2CommandGetRa(Server &server) : Fs2Command(server) {}
	bool writeCommandToBuffer(char *&buff, char *end) override;
	int readAnswerFromBuffer(const char *&buff, const char *end) override;
	void print(QDebug &o) const override;
};

//! Astro Electronic FS2 command: Get the current declination.
class Fs2CommandGetDec : public Fs2Command
{
public:
	Fs2CommandGetDec(Server &server) : Fs2Command(server) {}
	bool writeCommandToBuffer(char *&buff, char *end) override;
	int readAnswerFromBuffer(const char *&buff, const char *end) override;
	void print(QDebug &o) const override;
};

#endif // FS2COMMAND_HPP
