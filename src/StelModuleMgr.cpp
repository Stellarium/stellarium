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
 
#include <iostream>
#include <config.h>

#include "InitParser.hpp"
#include "StelModuleMgr.hpp"
#include "StelApp.hpp"
#include "StelModule.hpp"
#include "StelFileMgr.hpp"
#include "StelPluginInterface.hpp"


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

/*************************************************************************
 Register a new StelModule to the list
*************************************************************************/
void StelModuleMgr::registerModule(StelModule* m)
{
	if (modules.find(m->getModuleID()) != modules.end())
	{		
		std::cerr << "Module \"" << m->getModuleID() << "\" is already loaded." << std::endl;
		return;
	}
	modules.insert(std::pair<string, StelModule*>(m->getModuleID(), m));
}

/*************************************************************************
 Get the corresponding module or NULL if can't find it.
*************************************************************************/
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

#include <QPluginLoader>

/*************************************************************************
 Load an external plugin
*************************************************************************/
StelModule* StelModuleMgr::loadExternalPlugin(const string& moduleID)
{
	string moduleFullPath = "modules/" + moduleID + "/lib" + moduleID;
#ifdef WIN32
	moduleFullPath += ".dll";
#else
	moduleFullPath += ".so";
#endif
	try
	{
		moduleFullPath = StelApp::getInstance().getFileMgr().findFile(moduleFullPath, StelFileMgr::FILE);
	}
	catch(exception& e)
	{
		cerr << "ERROR while locating module path: " << e.what() << endl;
	}

	QPluginLoader loader(moduleFullPath.c_str());
	if (!loader.load())
	{
		cerr << "Couldn't load the dynamic library: " << moduleFullPath << ": " << loader.errorString().toStdString() << endl;
		cerr << "Module " << moduleID << " will not be loaded." << endl;
		return NULL;
	}
	
	QObject* obj = loader.instance();
	if (!obj)
	{
		cerr << "Couldn't open the dynamic library: " << moduleFullPath << ": " << loader.errorString().toStdString() << endl;
		cerr << "Module " << moduleID << " will not be loaded." << endl;
		return NULL;
	}
	
	StelPluginInterface* plugInt = qobject_cast<StelPluginInterface *>(obj);
	StelModule* sMod = plugInt->getStelModule();
	cout << "Loaded external module " << moduleID << "." << endl;
	return sMod;
}

/*************************************************************************
 Generate properly sorted calling lists for each action (e,g, draw, update)
 according to modules orders dependencies
*************************************************************************/
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
		
		// Order N times to ensure that all dependencies are respected 
		// Very unoptimized method but we don't care since it's not time critical
		for (unsigned int n=0;n<modules.size();++n)
		{
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
		
//		cerr << "---------------" << endl;
//		cerr << mc->first << endl;
//		for (std::vector<StelModule*>::iterator ii=mc->second.begin();ii!=mc->second.end();++ii)
//			cerr << (*ii)->getModuleID() << endl;
	}
}

/*************************************************************************
 Return the list of all the external module found in the modules/ directories
*************************************************************************/
std::vector<StelModuleMgr::ExternalStelModuleDescriptor> StelModuleMgr::getExternalModuleList()
{
	std::vector<StelModuleMgr::ExternalStelModuleDescriptor> result;
	set<string> moduleDirs;
	
	StelFileMgr& fileMan(StelApp::getInstance().getFileMgr());
	
	try
	{
		moduleDirs = fileMan.listContents("modules",StelFileMgr::DIRECTORY);
	}
	catch(exception& e)
	{
		cerr << "ERROR while trying list list modules:" << e.what() << endl;	
	}
	
	for (set<string>::iterator dir=moduleDirs.begin(); dir!=moduleDirs.end(); dir++)
	{
		try
		{
			StelModuleMgr::ExternalStelModuleDescriptor mDesc;
			InitParser pd;
			pd.load(fileMan.findFile("modules/" + *dir + "/module.ini"));
			mDesc.key = *dir;
			mDesc.name = pd.get_str("module", "name");
			mDesc.author = pd.get_str("module", "author");
			mDesc.contact = pd.get_str("module", "contact");
			mDesc.description = pd.get_str("module", "description");
			mDesc.loadAtStartup = pd.get_boolean("module", "load_at_startup");
			result.push_back(mDesc);
		}
		catch (exception& e)
		{
			cerr << "WARNING: unable to successfully read module.ini file from module " << *dir << endl;
		}
	}

	return result;
}
