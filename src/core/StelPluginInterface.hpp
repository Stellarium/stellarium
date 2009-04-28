/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _STELPLUGININTERFACE_HPP_
#define _STELPLUGININTERFACE_HPP_

#include <QtPlugin>

//! @class StelPluginInterface
//! Define the interface to implement when creating a plugin.
//! The interface is used by the <a href="http://doc.trolltech.com/4.4/qpluginloader.html">QPluginLoader</a> to load Stellarium plugins dynamically.
//! @sa @ref plugins for documentation on how to develop external plugins.
class StelPluginInterface
{
public:
	virtual ~StelPluginInterface() {}

	//! Get the instance of StelModule to include in the list of standard StelModule
	virtual class StelModule* getStelModule() const = 0;

	//! Get the name of the plugin. It should match the lib file name, e.g. HelloStelModule, or VirGO
	virtual QString getPluginId() = 0;
};

Q_DECLARE_INTERFACE(StelPluginInterface,"stellarium.StelPluginInterface/1.0");

#endif // _STELPLUGININTERFACE_HPP_
