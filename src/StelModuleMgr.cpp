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

#include <QDebug>
#include <QPluginLoader>

#include "InitParser.hpp"
#include "StelModuleMgr.hpp"
#include "StelApp.hpp"
#include "StelModule.hpp"
#include "StelFileMgr.hpp"
#include "StelPluginInterface.hpp"


StelModuleMgr::StelModuleMgr()
{
	// Initialize empty call lists for each possible actions
	callOrders[StelModule::ACTION_DRAW]=QList<StelModule*>();
	callOrders[StelModule::ACTION_UPDATE]=QList<StelModule*>();
	callOrders[StelModule::ACTION_HANDLEMOUSECLICKS]=QList<StelModule*>();
	callOrders[StelModule::ACTION_HANDLEMOUSEMOVES]=QList<StelModule*>();
	callOrders[StelModule::ACTION_HANDLEKEYS]=QList<StelModule*>();
}

StelModuleMgr::~StelModuleMgr()
{
}

/*************************************************************************
 Register a new StelModule to the list
*************************************************************************/
void StelModuleMgr::registerModule(StelModule* m)
{
	if (modules.find(m->objectName()) != modules.end())
	{		
		qWarning() << "Module \"" << m->objectName() << "\" is already loaded.";
		return;
	}
	modules.insert(m->objectName(), m);
	m->setParent(this);
}

/*************************************************************************
 Get the corresponding module or NULL if can't find it.
*************************************************************************/
StelModule* StelModuleMgr::getModule(const QString& moduleID)
{
	QMap<QString, StelModule*>::const_iterator iter = modules.find(moduleID);
	if (iter==modules.end())
	{
		qWarning() << "Warning can't find module called " << moduleID << ".";
		return NULL;
	}
	return iter.value();
}


/*************************************************************************
 Load an external plugin
*************************************************************************/
StelModule* StelModuleMgr::loadExternalPlugin(const QString& moduleID)
{
	QString moduleFullPath = "modules/" + moduleID + "/lib" + moduleID;
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
	catch (exception& e)
	{
		cerr << "ERROR while locating module path: " << e.what() << endl;
	}

	QPluginLoader loader(moduleFullPath);
	if (!loader.load())
	{
		qWarning() << "Couldn't load the dynamic library: " << moduleFullPath << ": " << loader.errorString();
		qWarning() << "Module " << moduleID << " will not be loaded.";
		return NULL;
	}
	
	QObject* obj = loader.instance();
	if (!obj)
	{
		qWarning() << "Couldn't open the dynamic library: " << moduleFullPath << ": " << loader.errorString();
		qWarning() << "Module " << moduleID << " will not be open.";
		return NULL;
	}
	
	StelPluginInterface* plugInt = qobject_cast<StelPluginInterface *>(obj);
	StelModule* sMod = plugInt->getStelModule();
	cout << "Loaded external module " << moduleID.toStdString() << "." << endl;
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
	QMap<StelModule::StelModuleActionName, QList<StelModule*> >::iterator mc;
	// For each actions (e.g. "draw", "update", etc..)
	for(mc=callOrders.begin();mc!=callOrders.end();++mc)
	{
		// Flush previous call orders
		mc.value().clear();
		// and init them with modules in creation order
		foreach (StelModule* m, getAllModules())
		{
			mc.value().push_back(m);
		}
		qSort(mc.value().begin(), mc.value().end(), StelModuleOrderComparator(mc.key()));
	}
}

/*************************************************************************
 Return the list of all the external module found in the modules/ directories
*************************************************************************/
QList<StelModuleMgr::ExternalStelModuleDescriptor> StelModuleMgr::getExternalModuleList()
{
	QList<StelModuleMgr::ExternalStelModuleDescriptor> result;
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
			pd.load(fileMan.findFile("modules/" + *dir + "/module.ini"));
			mDesc.key = *dir;
			mDesc.name = pd.get_str("module", "name").c_str();
			mDesc.author = pd.get_str("module", "author").c_str();
			mDesc.contact = pd.get_str("module", "contact").c_str();
			mDesc.description = pd.get_str("module", "description").c_str();
			mDesc.loadAtStartup = pd.get_boolean("module", "load_at_startup");
			result.push_back(mDesc);
		}
		catch (exception& e)
		{
			qWarning() << "WARNING: unable to successfully read module.ini file from module " << *dir;
		}
	}

	return result;
}
