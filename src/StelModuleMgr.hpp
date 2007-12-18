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
 
#ifndef STELMODULEMGR_H
#define STELMODULEMGR_H

#include <QObject>
#include <QMap>
#include <QList>
#include "StelModule.hpp"

//! @class StelModuleMgr
//! Manage a collection of StelModules including both core and plugin modules.
//! 
class StelModuleMgr : public QObject
{
	Q_OBJECT
			
public:
	StelModuleMgr();
	~StelModuleMgr();
	
	//! Register a new StelModule to the list
	void registerModule(StelModule* m);
	
	//! Load dynamically a module
	//! @param moduleID the name of the module = name of the dynamic library file without extension 
	//! (e.g "mymodule" for mymodule.so or mymodule.dll)
	//! @return the loaded module or NULL in case of error. The returned Stelmodule still needs to be initialized 
	StelModule* loadExternalPlugin(const QString& moduleID);

	//! Get the corresponding module or NULL if can't find it.
	StelModule* getModule(const QString& moduleID);
	
	//! Get the list of all the currently registered modules
	QList<StelModule*> getAllModules() {return modules.values();}
	
	//! Generate properly sorted calling lists for each action (e,g, draw, update)
	//! according to modules orders dependencies
	void generateCallingLists();

	//! Get the list of modules in the correct order for calling the given action
	const QList<StelModule*>& getCallOrders(StelModule::StelModuleActionName action)
	{
		return callOrders[action];
	}

	//! Contains the information read from the module.ini file
	struct ExternalStelModuleDescriptor
	{
		//! The name of the directory and of the lib*.so with *=key
		QString key;
		QString name;
		QString author;
		QString contact;
		QString description;
		bool loadAtStartup;
	};
 
	//! Return the list of all the external module found in the modules directories
	static QList<ExternalStelModuleDescriptor> getExternalModuleList();

	//! Enum used when selecting objects to define whether to add to, replace, or remove from 
	//! the existing selection list.
	enum selectAction
	{
		ADD_TO_SELECTION,
		REPLACE_SELECTION,
		REMOVE_FROM_SELECTION
	};

 
private:
	//! The main module list associating name:pointer
	QMap<QString, StelModule*> modules;
	
	//! The list of all module in the correct order for each action
	QMap<StelModule::StelModuleActionName, QList<StelModule*> > callOrders;

};

#endif
