/*
 * Stellarium external dynamic plug-in test
 * Copyright (C) 2026 Stellarium Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "StelModule.hpp"
#include "StelPluginInterface.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QPluginLoader>

#include <memory>

int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);
	if (argc != 2)
	{
		qCritical() << "Usage: StellariumPluginProbeLoader <plugin DLL>";
		return 2;
	}

	const QString pluginPath =
		QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
	QPluginLoader loader(pluginPath);
	if (!loader.load())
	{
		qCritical().noquote() << "QPluginLoader failed:" << loader.errorString();
		return 3;
	}

	StelPluginInterface* plugin =
		qobject_cast<StelPluginInterface*>(loader.instance());
	if (plugin == nullptr)
	{
		qCritical() << "The DLL does not implement StelPluginInterface";
		return 4;
	}

	const StelPluginInfo info = plugin->getPluginInfo();
	if (info.id != QStringLiteral("DynamicPluginProbe") ||
		info.version != QStringLiteral("1.0.0") ||
		!info.startByDefault)
	{
		qCritical() << "Unexpected plug-in metadata";
		return 5;
	}

	std::unique_ptr<StelModule> module(plugin->getStelModule());
	if (!module || module->objectName() != info.id)
	{
		qCritical() << "The plug-in returned an invalid module";
		return 6;
	}
	if (module->getModuleVersion() !=
		QStringLiteral(STELLARIUM_EXPECTED_VERSION))
	{
		qCritical() << "Unexpected Stellarium module version:"
		            << module->getModuleVersion();
		return 7;
	}

	module->init();
	qInfo().noquote() << "STEL_PLUGIN_SDK_PROBE_OK" << info.id
	                 << module->getModuleVersion();
	return 0;
}
