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
 
#include "StelModuleMgr.hpp"
#include "StelApp.hpp"
#include "StelModule.hpp"

#if defined (HAVE_GMODULE) && defined (HAVE_GLIB)
 #include <glib.h>
 #include <gmodule.h>
 // the function signature for getStelmodule()
 typedef StelModule* (* getStelmoduleFunc) (void);
 static getStelmoduleFunc getStelModule = NULL;
#endif

StelModuleMgr::StelModuleMgr() : endIter(modules.end())
{
	// Initialize empty call lists for each possible actions
	callOrders["draw"]=std::vector<StelModule*>();
	callOrders["update"]=std::vector<StelModule*>();
	callOrders["handleMouseClicks"]=std::vector<StelModule*>();
	callOrders["handleMouseMoves"]=std::vector<StelModule*>();
	callOrders["handleKeys"]=std::vector<StelModule*>();
}

StelModuleMgr::~StelModuleMgr()
{
}

// Register a new StelModule to the list
void StelModuleMgr::registerModule(StelModule* m)
{
	if (modules.find(m->getModuleID()) != modules.end())
	{		
		std::cerr << "Module \"" << m->getModuleID() << "\" is already loaded." << std::endl;
		return;
	}
	modules.insert(std::pair<string, StelModule*>(m->getModuleID(), m));
}

//! Get the corresponding module or NULL if can't find it.
StelModule* StelModuleMgr::getModule(const string& moduleID)
{
	std::map<string, StelModule*>::const_iterator iter = modules.find(moduleID);
	if (iter==modules.end())
	{
		cerr << "Warning can't find module called " << moduleID << "." << endl;
		return NULL;
	}
	return iter->second;
}

StelModule* StelModuleMgr::loadExternalModule(const string& moduleID)
{
	string moduleFullPath = "modules/" + moduleID + "/" + moduleID;
#ifdef WIN32
	moduleFullPath += ".dll";
#else
	moduleFullPath += ".so";
#endif
	moduleFullPath = StelApp::getInstance().getFilePath(moduleFullPath);
#if !defined (HAVE_GMODULE) || !defined (HAVE_GLIB)
	cerr << "This version of stellarium was compiled without enabling dynamic loading of modules." << endl;
	cerr << "Module " << moduleID << " will not be loaded." << endl;
	return NULL;
#else
	if (g_module_supported()==FALSE)
	{
		cerr << "Dynamic loading of modules does not work on this plateform." << endl;
		cerr << "Module " << moduleID << " will not be loaded." << endl;
		return NULL;
	}
	// Load module
	GModule* md = g_module_open(moduleFullPath.c_str(), G_MODULE_BIND_LAZY);
	if (md==NULL)
	{
		cerr << "Couldn't find the dynamic library: " << moduleFullPath << ": " << g_module_error() << endl;
		cerr << "Module " << moduleID << " will not be loaded." << endl;
		return NULL;
	}

	if (g_module_symbol(md, "getStelModule", (gpointer *)&getStelModule)!=TRUE)
	{
		cerr << "Couldn't find the getStelModule() function in the shared library: " << moduleID << ": " << g_module_error() << endl;
		cerr << "Module " << moduleID << " will not be loaded." << endl;
		g_module_close(md);
		return NULL;
	}
	StelModule* sMod = getStelModule();
	cout << "Loaded external module " << moduleID << "." << endl;
	return sMod;
#endif
}


// Generate properly sorted calling lists for each action (e,g, draw, update)
// according to modules orders dependencies
void StelModuleMgr::generateCallingLists()
{
	std::map<string, std::vector<StelModule*> >::iterator mc;
	std::map<string, StelModule*>::iterator m;
	
	// For each actions (e.g. "draw", "update", etc..)
	for (mc=callOrders.begin();mc!=callOrders.end();++mc)
	{
		// Flush previous call orders
		mc->second.clear();
		// and init them with modules in creation order
		for (m=modules.begin();m!=modules.end();++m)
		{
			mc->second.push_back(m->second);
		}
		// Order them now according to dependencies
		for (m=modules.begin();m!=modules.end();++m)
		{
			StelModule::DependenciesOrderT::iterator tmp = m->second->dependenciesOrder.find(mc->first);
			if (tmp!=m->second->dependenciesOrder.end() && tmp->second.size()!=0)
			{
				// There is a dependency
				const string& dep = tmp->second;
				std::vector<StelModule*>& list = mc->second;
				
				// Remove the module we want to sort
				std::vector<StelModule*>::iterator thisIdx;
				for (thisIdx=list.begin(); (*thisIdx)!=m->second && thisIdx!=list.end(); ++thisIdx){;}
				assert(thisIdx!=list.end());
				list.erase(thisIdx);
				
				// And insert it after the dependent module
				std::vector<StelModule*>::iterator depIdx;
				if (dep=="first")
				{
					depIdx = list.begin();
				}
				else if (dep=="last")
				{
					depIdx = list.end();
				}
				else
				{
					for (depIdx=list.begin(); (*depIdx)->getModuleID()!=dep && depIdx!=list.end(); ++depIdx){;}
					if (depIdx==list.end())
					{
						cerr << "Error, can't find module \"" << dep << "\" on which module \"" << m->second->getModuleID() << "\" depends for the operation \""<< tmp->first << "\"." << endl;
						assert(0);
					}
					++depIdx;
				}
				list.insert(depIdx, m->second);
			}
		}
	}
}
