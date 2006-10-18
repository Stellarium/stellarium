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

#include "stelmodule.h"

/**
 * Class used to manage a collection of StelModule
 * TODO : Manage drawing order, update order
 * @author Fabien Chereau <stellarium@free.fr>
 */
class StelModuleMgr{
public:
	
	StelModuleMgr();
    ~StelModuleMgr();
	
	//! Add a static (no dynamic linking) module which is already initialized
	void registerModule(StelModule* m)
	{
		if (modules.find(m->getModuleID()) != modules.end())
		{		
			std::cerr << "Module \"" << m->getModuleID() << "\" is already loaded." << std::endl;
		}
		modules.insert(std::pair<string, StelModule*>(m->getModuleID(), m));
	}
	
	//! Load dynamically a module and initialize it
	void registerModule(const string& moduleID)
	{
		// TODO use glib here
	}	
	
	StelModule& getModule(const string& moduleID)
	{
		std::map<string, StelModule*>::const_iterator iter = modules.find(moduleID);
		if (iter==modules.end())
		{
			std::cerr << "Module \"" << moduleID << "\" does not exists or is not loaded." << std::endl;
			assert(0);
		}
		return *((*iter).second);
	}
	
private:
	
	//! The main module list
	std::map<string, StelModule*> modules;
};

#endif
