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
	callOrders[StelModule::ACTION_DRAW]=std::vector<StelModule*>();
	callOrders[StelModule::ACTION_UPDATE]=std::vector<StelModule*>();
	callOrders[StelModule::ACTION_HANDLEMOUSECLICKS]=std::vector<StelModule*>();
	callOrders[StelModule::ACTION_HANDLEMOUSEMOVES]=std::vector<StelModule*>();
	callOrders[StelModule::ACTION_HANDLEKEYS]=std::vector<StelModule*>();
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
#ifdef MACOSX
	moduleFullPath += ".dylib";
#else
	moduleFullPath += ".so";
#endif
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
		cerr << "Module " << moduleID << " will not be open." << endl;
		return NULL;
	}
	
	StelPluginInterface* plugInt = qobject_cast<StelPluginInterface *>(obj);
	StelModule* sMod = plugInt->getStelModule();
	cout << "Loaded external module " << moduleID << "." << endl;
	return sMod;
}

struct StelModuleOrderComparator
{
	StelModuleOrderComparator(StelModule::StelModuleActionName aaction) : action(aaction) {;}
	bool operator()(StelModule* x, StelModule* y) {return x->getCallOrder(action)<y->getCallOrder(action);}
private:
	StelModule::StelModuleActionName action;
};

/*************************************************************************
 Generate properly sorted calling lists for each action (e,g, draw, update)
 according to modules orders dependencies
*************************************************************************/
void StelModuleMgr::generateCallingLists()
{
	std::map<StelModule::StelModuleActionName, std::vector<StelModule*> >::iterator mc;
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
		std::sort(mc->second.begin(), mc->second.end(), StelModuleOrderComparator(mc->first));
	}
}

/*************************************************************************
 Return the list of all the external module found in the modules/ directories
*************************************************************************/
std::vector<StelModuleMgr::ExternalStelModuleDescriptor> StelModuleMgr::getExternalModuleList()
{
	std::vector<StelModuleMgr::ExternalStelModuleDescriptor> result;
	QSet<QString> moduleDirs;
	
	StelFileMgr& fileMan(StelApp::getInstance().getFileMgr());
	
	try
	{
		moduleDirs = fileMan.listContents("modules",StelFileMgr::DIRECTORY);
	}
	catch(exception& e)
	{
		cerr << "ERROR while trying list list modules:" << e.what() << endl;	
	}
	
	for (QSet<QString>::iterator dir=moduleDirs.begin(); dir!=moduleDirs.end(); dir++)
	{
		try
		{
			StelModuleMgr::ExternalStelModuleDescriptor mDesc;
			InitParser pd;
			pd.load(fileMan.qfindFile("modules/" + *dir + "/module.ini"));
			mDesc.key = (*dir).toStdString();
			mDesc.name = pd.get_str("module", "name");
			mDesc.author = pd.get_str("module", "author");
			mDesc.contact = pd.get_str("module", "contact");
			mDesc.description = pd.get_str("module", "description");
			mDesc.loadAtStartup = pd.get_boolean("module", "load_at_startup");
			result.push_back(mDesc);
		}
		catch (exception& e)
		{
			cerr << "WARNING: unable to successfully read module.ini file from module " << (*dir).toStdString() << endl;
		}
	}

	return result;
}
