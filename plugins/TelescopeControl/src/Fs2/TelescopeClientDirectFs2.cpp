/*
 * Stellarium Telescope Control Plug-in
 *
 * Copyright (C) 2009 Bogdan Marinov (this file,
 * reusing code written by Johannes Gajdosik in 2006)
 *
 * Astro Electronic FS2 support by Michael Butti, 2026.
 *
 * Johannes Gajdosik wrote in 2006 the original telescope control feature
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include <QRegularExpression>
#include <QStringList>

#include "TelescopeClientDirectFs2.hpp"

#include "Fs2Connection.hpp"
#include "Fs2Command.hpp"
#include "common/TelescopeLogging.hpp"
#include "StelCore.hpp"

TelescopeClientDirectFs2::TelescopeClientDirectFs2(const QString &name, const QString &parameters,
                                                   TelescopeControl::Equinox eq, int slot)
	: TelescopeClient(name, slot)
	, Server(0, slot)
	, time_delay(0)
	, equinox(eq)
	, fs2(nullptr)
	, long_format_used(false)
	, answers_received(false)
	, last_ra(0)
	, queue_get_position(true)
	, next_pos_time(0)
{
	interpolatedPosition.reset();

	//Extract parameters
	//Format: "serial_port_name:time_delay"          e.g. "/dev/ttyS0:500000"
	static const QRegularExpression serialParamRx("^([^:]*):(\\d+)$");
	QRegularExpressionMatch serialMatch = serialParamRx.match(parameters);
	QString serialDeviceName;
	if (serialMatch.hasMatch())
	{
		// This RegExp only matches valid integers
		serialDeviceName = serialMatch.captured(1).trimmed();
		time_delay       = serialMatch.captured(2).toInt();
	}
	else
	{
		qCWarning(Telescopes) << getLogPrefix()
		                      << "ERROR creating TelescopeClientDirectFs2: invalid parameters.";
		return;
	}

	qCDebug(Telescopes) << getLogPrefix()
	                    << "TelescopeClientDirectFs2 parameters: port, time_delay:" << serialDeviceName
	                    << time_delay;

	//Validation: Time delay
	if (time_delay <= 0 || time_delay > 10000000)
	{
		qCWarning(Telescopes) << getLogPrefix()
		                      << "ERROR creating TelescopeClientDirectFs2: time_delay not valid "
		                         "(should be less than 10000000)";
		return;
	}

	//end_of_timeout = -0x8000000000000000LL;

	//Try to establish a connection to the telescope
	#ifdef Q_OS_WIN
	if(serialDeviceName.right(serialDeviceName.size() - 3).toInt() > 9)
		serialDeviceName = "\\\\.\\" + serialDeviceName;//"\\.\COMxx", not sure if it will work
	#endif //Q_OS_WIN

	Fs2SerialConnection *connection = new Fs2SerialConnection(*this, qPrintable(serialDeviceName));
	if (connection->isClosed())
	{
		qCWarning(Telescopes) << getLogPrefix()
		                      << "ERROR creating TelescopeClientDirectFs2: cannot open serial device"
		                      << serialDeviceName;
		delete connection;
		return;
	}
	fs2 = connection;
	// Server owns the connection and deletes it with its socket list in Server::SocketList::clear().
	addConnection(connection);

	long_format_used = false; // unknown
	last_ra = 0;
	queue_get_position = true;
	next_pos_time = -0x8000000000000000LL;
	answers_received = false;
}

//! queues a GOTO command
void TelescopeClientDirectFs2::telescopeGoto(const Vec3d &j2000Pos, StelObjectP selectObject)
{
	Q_UNUSED(selectObject)

	if (!isConnected())
		return;

	Vec3d position = j2000Pos;
	if (equinox == TelescopeControl::EquinoxJNow)
	{
		const StelCore* core = StelApp::getInstance().getCore();
		position = core->j2000ToEquinoxEqu(j2000Pos, StelCore::RefractionOff);
	}

	const double ra_signed = atan2(position[1], position[0]);
	//Workaround for the discrepancy in precision between Windows/Linux/PPC Macs and Intel Macs:
	const double ra = (ra_signed >= 0) ? ra_signed : (ra_signed + 2.0 * M_PI);
	const double dec = atan2(position[2], std::sqrt(position[0]*position[0]+position[1]*position[1]));
	unsigned int ra_int = static_cast<unsigned int>(std::floor(0.5 + ra*(static_cast<unsigned int>(0x80000000)/M_PI)));
	int dec_int = static_cast<int>(std::floor(0.5 + dec*(static_cast<unsigned int>(0x80000000)/M_PI)));

	gotoReceived(ra_int, dec_int);
}

void TelescopeClientDirectFs2::telescopeSync(const Vec3d &j2000Pos, StelObjectP selectObject)
{
	Q_UNUSED(selectObject)

	if (!isConnected())
		return;

	Vec3d position = j2000Pos;
	if (equinox == TelescopeControl::EquinoxJNow)
	{
		const StelCore* core = StelApp::getInstance().getCore();
		position = core->j2000ToEquinoxEqu(j2000Pos, StelCore::RefractionOff);
	}

	const double ra_signed = atan2(position[1], position[0]);
	//Workaround for the discrepancy in precision between Windows/Linux/PPC Macs and Intel Macs:
	const double ra = (ra_signed >= 0) ? ra_signed : (ra_signed + 2.0 * M_PI);
	const double dec = atan2(position[2], std::sqrt(position[0]*position[0]+position[1]*position[1]));
	unsigned int ra_int = static_cast<unsigned int>(std::floor(0.5 + ra*(static_cast<unsigned int>(0x80000000)/M_PI)));
	int dec_int = static_cast<int>(std::floor(0.5 + dec*(static_cast<unsigned int>(0x80000000)/M_PI)));

	syncReceived(ra_int, dec_int);
}

void TelescopeClientDirectFs2::telescopeAbortSlew()
{
	if (!isConnected())
		return;
	fs2->sendAbort();

	// sendAbort() discards every queued command, including any position query
	// that was in flight. That would leave queue_get_position stuck false and
	// stop the position polling entirely. Re-arm it, but with a short delay so
	// that answers to the discarded queries can arrive and be ignored before
	// the next query is sent.
	queue_get_position = true;
	next_pos_time = GetNow() + 500000; // 0.5 s
}

void TelescopeClientDirectFs2::gotoReceived(unsigned int ra_int, int dec_int)
{
	fs2->sendGoto(ra_int, dec_int);
}

void TelescopeClientDirectFs2::syncReceived(unsigned int ra_int, int dec_int)
{
	fs2->sendSync(ra_int, dec_int);
}


//! estimates where the telescope is by interpolation in the stored
//! telescope positions:
Vec3d TelescopeClientDirectFs2::getJ2000EquatorialPos(const StelCore*) const
{
	const qint64 now = getNow() - time_delay;
	return interpolatedPosition.get(now);
}

bool TelescopeClientDirectFs2::prepareCommunication()
{
	//TODO: Nothing to prepare?
	return true;
}

void TelescopeClientDirectFs2::performCommunication()
{
	step(10000);
}

void TelescopeClientDirectFs2::communicationResetReceived(void)
{
	long_format_used = false;
	queue_get_position = true;
	next_pos_time = -0x8000000000000000LL;

	qCDebug(Telescopes) << getLogPrefix() << "TelescopeClientDirectFs2::communicationResetReceived";

	// A protocol-level reset (e.g. a timeout while waiting for an answer)
	// must not tear down the serial port itself. We simply re-synchronise
	// and keep the link open.
	answers_received = false;
}

//! Called in Fs2CommandGetRa and Fs2CommandGetDec.
void TelescopeClientDirectFs2::longFormatUsedReceived(bool long_format)
{
	answers_received = true;
	if (!long_format_used && !long_format)
	{
		fs2->sendCommand(new Fs2CommandToggleFormat(*this));
	}
	long_format_used = true;
}

//! Called by Fs2CommandGetRa::readAnswerFromBuffer().
void TelescopeClientDirectFs2::raReceived(unsigned int ra_int)
{
	answers_received = true;
	last_ra = ra_int;
	qCDebug(Telescopes) << getLogPrefix() << "TelescopeClientDirectFs2::raReceived:" << ra_int;
}

//! Called by Fs2CommandGetDec::readAnswerFromBuffer().
//! Should be called after raReceived(), as it contains a call to sendPosition().
void TelescopeClientDirectFs2::decReceived(unsigned int dec_int)
{
	answers_received = true;
	qCDebug(Telescopes) << getLogPrefix() << "TelescopeClientDirectFs2::decReceived:" << dec_int;
	const int fs2_status = 0;
	sendPosition(last_ra, static_cast<int>(dec_int), fs2_status);
	queue_get_position = true;
}

void TelescopeClientDirectFs2::step(long long int timeout_micros)
{
	if (!fs2 || fs2->isClosed())
		return;
	long long int now = GetNow();
	if (queue_get_position && now >= next_pos_time)
	{
		fs2->sendCommand(new Fs2CommandGetRa(*this));
		fs2->sendCommand(new Fs2CommandGetDec(*this));
		queue_get_position = false;
		next_pos_time = now + 500000;// 500000;
	}
	Server::step(timeout_micros);
}

bool TelescopeClientDirectFs2::isConnected(void) const
{
	return (fs2 && !fs2->isClosed());//TODO
}

bool TelescopeClientDirectFs2::isInitialized(void) const
{
	return (fs2 && !fs2->isClosed());
}

//Merged from Connection::sendPosition() and TelescopeTCP::performReading()
void TelescopeClientDirectFs2::sendPosition(unsigned int ra_int, int dec_int, int status)
{
	//Server time is "now", because this class is the server
	const qint64 server_micros = static_cast<qint64>(getNow());
	const double ra  =  ra_int * (M_PI/0x80000000u);
	const double dec = dec_int * (M_PI/0x80000000u);
	const double cdec = cos(dec);
	Vec3d position(cos(ra)*cdec, sin(ra)*cdec, sin(dec));
	Vec3d j2000Position = position;
	if (equinox == TelescopeControl::EquinoxJNow)
	{
		const StelCore* core = StelApp::getInstance().getCore();
		j2000Position = core->equinoxEquToJ2000(position, StelCore::RefractionOff);
	}
	interpolatedPosition.add(j2000Position, getNow(), server_micros, status);
}
