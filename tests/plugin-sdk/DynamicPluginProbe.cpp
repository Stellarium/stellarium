/*
 * Stellarium external dynamic plug-in test
 * Copyright (C) 2026 Stellarium Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "DynamicPluginProbe.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QTimer>

DynamicPluginProbe::DynamicPluginProbe()
{
	setObjectName(QStringLiteral("DynamicPluginProbe"));
}

void DynamicPluginProbe::init()
{
	const QString moduleVersion = getModuleVersion();
	qInfo().noquote() << "STEL_DYNAMIC_PLUGIN_PROBE_INITIALIZED"
	                 << moduleVersion;

	const QString sentinelPath =
		qEnvironmentVariable("STELLARIUM_PLUGIN_PROBE_SENTINEL");
	if (sentinelPath.isEmpty())
		return;

	QFile sentinel(sentinelPath);
	if (sentinel.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream stream(&sentinel);
		stream << "DynamicPluginProbe " << moduleVersion << '\n';
	}
	else
	{
		qWarning().noquote() << "DynamicPluginProbe could not write sentinel:"
		                     << sentinel.errorString();
	}

	// Startup may enter nested event loops before the top-level loop starts.
	// Keep requesting shutdown until the top-level loop handles it.
	QTimer* quitTimer = new QTimer(this);
	quitTimer->setInterval(2000);
	QObject::connect(quitTimer, &QTimer::timeout,
	                 QCoreApplication::instance(), []()
	{
		QCoreApplication::quit();
	});
	quitTimer->start();
}

StelModule* DynamicPluginProbeInterface::getStelModule() const
{
	return new DynamicPluginProbe();
}

StelPluginInfo DynamicPluginProbeInterface::getPluginInfo() const
{
	StelPluginInfo info;
	info.id = QStringLiteral("DynamicPluginProbe");
	info.displayedName = QStringLiteral("Dynamic plug-in SDK probe");
	info.authors = QStringLiteral("Stellarium Developers");
	info.contact = QStringLiteral("https://stellarium.org/");
	info.description =
		QStringLiteral("Minimal standalone Windows dynamic plug-in test.");
	info.version = QStringLiteral("1.0.0");
	info.license = QStringLiteral("GPL-2.0-or-later");
	info.startByDefault = true;
	return info;
}
