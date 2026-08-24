/*
 * Stellarium external dynamic plug-in test
 * Copyright (C) 2026 Stellarium Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef DYNAMICPLUGINPROBE_HPP
#define DYNAMICPLUGINPROBE_HPP

#include "StelModule.hpp"
#include "StelPluginInterface.hpp"

#include <QObject>

class DynamicPluginProbe : public StelModule
{
public:
	DynamicPluginProbe();
	void init() override;
};

class DynamicPluginProbeInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)

public:
	StelModule* getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
};

#endif // DYNAMICPLUGINPROBE_HPP
