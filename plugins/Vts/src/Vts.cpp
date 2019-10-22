/*
 * Copyright (C) 2019 Guillaume Chereau
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

#include "Vts.hpp"

#include "CLIProcessor.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModule.hpp"
#include "StelScriptMgr.hpp"
#include "StelTranslator.hpp"

#include <QApplication>
#include <QDebug>
#include <QTcpSocket>

class Vts : public StelModule
{
public:
	virtual void init();
	virtual void update(double) {;}

private:
	QTcpSocket socket;
	int port;

	// Private methods.
	void processMessage(const QByteArray& msg);
	void processPropCmd(const QString& prop, const QString& value);

private slots:
	void onConnected();
	void onDisconnected();
	void onReadyRead();
};

void Vts::init()
{
	port = CLIProcessor::argsGetOptionWithArg(qApp->arguments(), "", "--serverport", 8888).toInt();
	qDebug() << "Start Vts on port" << port;

	connect(&socket, &QTcpSocket::connected, this, &Vts::onConnected);
	connect(&socket, &QTcpSocket::disconnected, this, &Vts::onDisconnected);
	connect(&socket, &QTcpSocket::readyRead, this, &Vts::onReadyRead);
	socket.connectToHost("localhost", port);
}

void Vts::onConnected()
{
	socket.write("INIT Stellarium CONSTRAINT 1.0\n");
}

void Vts::onDisconnected()
{
}

void Vts::processPropCmd(const QString& prop, const QString& value)
{
	QString script = QString("%1 = %2").arg(prop, value);
	StelApp::getInstance().getScriptMgr().runPreprocessedScript(script, "Vts");
}

void Vts::processMessage(const QByteArray& msg)
{
	QTextStream stream(msg);
	QString cmd;
	stream >> cmd;

	StelCore* core = StelApp::getInstance().getCore();

	if (cmd == "CMD")
	{
		if (msg == "CMD TIME PAUSE\n")
		{
			core->setTimeRate(0);
			return;
		}
		stream >> cmd;
		if (cmd == "PROP")
		{
			QString prop, value;
			stream >> prop >> value;
			processPropCmd(prop, value);
			return;
		}
	}

	if (cmd == "TIME")
	{
		const double JD1950 = 2433282.5;
		double time, ratio;
		stream >> time >> ratio;

		core->setJD(time + JD1950);
		core->setTimeRate(StelCore::JD_SECOND * ratio);
		return;
	}
	qWarning() << "Unknown CMD" << msg;
}

void Vts::onReadyRead()
{
	while (socket.canReadLine())
	{
		QByteArray message = socket.readLine();
		processMessage(message);
	}
}

StelModule* VtsStelPluginInterface::getStelModule() const
{
	return new Vts();
}

StelPluginInfo VtsStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	// Q_INIT_RESOURCE(Vts);

	StelPluginInfo info;
	info.id = "Vts";
	info.displayedName = N_("Vts");
	info.authors = "Guillaume Chereau";
	info.contact = "<guillaume@noctua-software.com>";
	info.description = N_("Allow to use Stellarium as a plugin in CNES VTS.");
	info.version = VTS_PLUGIN_VERSION;
	info.license = VTS_PLUGIN_LICENSE;
	return info;
}
