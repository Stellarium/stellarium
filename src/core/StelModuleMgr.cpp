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
 
#include <config.h>

#include <QDebug>
#include <QPluginLoader>
#include <QSettings>

#include "StelModuleMgr.hpp"
#include "StelApp.hpp"
#include "StelModule.hpp"
#include "StelFileMgr.hpp"
#include "StelPluginInterface.hpp"
#include "StelIniParser.hpp"


StelModuleMgr::StelModuleMgr()
{
	// Initialize empty call lists for each possible actions
	callOrders[StelModule::ActionDraw]=QList<StelModule*>();
	callOrders[StelModule::ActionUpdate]=QList<StelModule*>();
	callOrders[StelModule::ActionHandleMouseClicks]=QList<StelModule*>();
	callOrders[StelModule::ActionHandleMouseMoves]=QList<StelModule*>();
	callOrders[StelModule::ActionHandleKeys]=QList<StelModule*>();
	callingListsToRegenerate = false;
}

StelModuleMgr::~StelModuleMgr()
{
}

// Regenerate calling lists if necessary
void StelModuleMgr::update()
{
	if (callingListsToRegenerate)
		generateCallingLists();
	callingListsToRegenerate = false;
}

/*************************************************************************
 Register a new StelModule to the list
*************************************************************************/
void StelModuleMgr::registerModule(StelModule* m, bool fgenerateCallingLists)
{
	if (modules.find(m->objectName()) != modules.end())
	{		
		qWarning() << "Module \"" << m->objectName() << "\" is already loaded.";
		return;
	}
	modules.insert(m->objectName(), m);
	m->setParent(this);
	
	if (fgenerateCallingLists)
		generateCallingLists();
}

/*************************************************************************
 Unregister and delete a StelModule.
*************************************************************************/
void StelModuleMgr::unloadModule(const QString& moduleID, bool alsoDelete)
{
	StelModule* m = getModule(moduleID);
	if (!m)
	{
		qWarning() << "Module \"" << moduleID << "\" is not loaded.";
		return;
	}
	modules.remove(moduleID);
	m->setParent(NULL);
	callingListsToRegenerate = true;
	if (alsoDelete)
		m->deleteLater();
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
StelModule* StelModuleMgr::loadPlugin(const QString& moduleID)
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
		moduleFullPath = StelApp::getInstance().getFileMgr().findFile(moduleFullPath, StelFileMgr::File);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "ERROR while locating plugin path: " << e.what();
	}

	QPluginLoader loader(moduleFullPath);
	if (!loader.load())
	{
		qWarning() << "Couldn't load the dynamic library: " << moduleFullPath << ": " << loader.errorString();
		qWarning() << "Plugin " << moduleID << " will not be loaded.";
		return NULL;
	}
	
	QObject* obj = loader.instance();
	if (!obj)
	{
		qWarning() << "Couldn't open the dynamic library: " << moduleFullPath << ": " << loader.errorString();
		qWarning() << "Plugin " << moduleID << " will not be open.";
		return NULL;
	}
	
	StelPluginInterface* plugInt = qobject_cast<StelPluginInterface *>(obj);
	StelModule* sMod = plugInt->getStelModule();
	qDebug() << "Loaded plugin " << moduleID << ".";
	return sMod;
}

struct StelModuleOrderComparator
{
	StelModuleOrderComparator(StelModule::StelModuleActionName aaction) : action(aaction) {;}
	bool operator()(StelModule* x, StelModule* y) {return x->getCallOrder(action)<y->getCallOrder(action);}
private:
	StelModule::StelModuleActionName action;
};


// Unload all plugins
void StelModuleMgr::unloadAllPlugins()
{
	QListIterator<PluginDescriptor> i(getPluginsList());
	i.toBack();
	while (i.hasPrevious())
	{
		const PluginDescriptor& d = i.previous();
		if (d.loadAtStartup==false)
			continue;
		unloadModule(d.key);
		qDebug() << "Unloaded plugin " << d.key << ".";
	}
}

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
QList<StelModuleMgr::PluginDescriptor> StelModuleMgr::getPluginsList()
{
	QList<StelModuleMgr::PluginDescriptor> result;
	QSet<QString> moduleDirs;
	
	StelFileMgr& fileMan(StelApp::getInstance().getFileMgr());
	
	try
	{
		moduleDirs = fileMan.listContents("modules",StelFileMgr::Directory);
	}
	catch(std::runtime_error& e)
	{
		qWarning() << "ERROR while trying list list modules:" << e.what();	
	}
	
	for (QSet<QString>::iterator dir=moduleDirs.begin(); dir!=moduleDirs.end(); dir++)
	{
		try
		{
			StelModuleMgr::PluginDescriptor mDesc;
			QSettings pd(fileMan.findFile("modules/" + *dir + "/module.ini"), StelIniFormat);
			mDesc.key = *dir;
			mDesc.name = pd.value("module/name").toString();
			mDesc.author = pd.value("module/author").toString();
			mDesc.contact = pd.value("module/contact").toString();
			mDesc.description = pd.value("module/description").toString();
			mDesc.loadAtStartup = pd.value("module/load_at_startup").toBool();
			result.push_back(mDesc);
		}
		catch (std::runtime_error& e)
		{
			qWarning() << "WARNING: unable to successfully read module.ini file from plugin " << *dir;
		}
	}

	return result;
}
