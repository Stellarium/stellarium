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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _STELPLUGININTERFACE_HPP_
#define _STELPLUGININTERFACE_HPP_

#include <QtPlugin>
#include <QImage>

//! @struct StelPluginInfo
//! Contains information about a Stellarium plugin.
struct StelPluginInfo
{
	StelPluginInfo() : startByDefault(false) {;}
	//! The plugin ID. It MUST match the lib file name (case sensitive), e.g. "HelloStelModule", or "VirGO".
	QString id;
	//! The displayed name, e.g. "Artificial Satellites".
	QString displayedName;
	//! The comma separated list of authors, e.g. "Fabien Chereau, Matthew Gates".
	QString authors;
	//! The contact email or URL.
	QString	contact;
	//! The HTML description of the plugin.
	QString description;
	//! Logo or preview image to display in the information dialog or an invalid image if not applicable.
	//! The image size should be x by x pixels.
	QImage image;
	//! Whether the plugin should be started by default (if nothing specified in config.ini)
	bool startByDefault;
};

//! @class StelPluginInterface
//! Define the interface to implement when creating a plugin.
//! The interface is used by the <a href="http://doc.trolltech.com/4.5/qpluginloader.html">QPluginLoader</a> to load Stellarium plugins dynamically.
//! @sa @ref plugins for documentation on how to develop external plugins.
class StelPluginInterface
{
public:
	virtual ~StelPluginInterface() {}

	//! Get the instance of StelModule to include in the list of standard StelModule
	virtual class StelModule* getStelModule() const = 0;
	
	//! Get information about the plugin.
	virtual StelPluginInfo getPluginInfo() const = 0;
};

Q_DECLARE_INTERFACE(StelPluginInterface, "stellarium.StelPluginInterface/2.0");

#endif // _STELPLUGININTERFACE_HPP_
