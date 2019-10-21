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
#include "StelModule.hpp"
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

void Vts::onReadyRead()
{
	QByteArray line = socket.readLine();
	qDebug() << "got line:" << line;
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
