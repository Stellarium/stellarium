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

#include <QDebug>
#include <QPluginLoader>
#include <QSettings>

#include "StelModuleMgr.hpp"
#include "StelApp.hpp"
#include "StelModule.hpp"
#include "StelFileMgr.hpp"
#include "StelPluginInterface.hpp"
#include "StelIniParser.hpp"



StelModuleMgr::StelModuleMgr() : callingListsToRegenerate(true), pluginDescriptorListLoaded(false)
{
	// Initialize empty call lists for each possible actions
	callOrders[StelModule::ActionDraw]=QList<StelModule*>();
	callOrders[StelModule::ActionUpdate]=QList<StelModule*>();
	callOrders[StelModule::ActionHandleMouseClicks]=QList<StelModule*>();
	callOrders[StelModule::ActionHandleMouseMoves]=QList<StelModule*>();
	callOrders[StelModule::ActionHandleKeys]=QList<StelModule*>();
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
	QString name = m->objectName();
	if (modules.find(name) != modules.end())
	{
		qWarning() << "Module \"" << name << "\" is already loaded.";
		return;
	}
	modules.insert(name, m);
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
	{
		m->deinit();
		delete m;
	}
}

/*************************************************************************
 Get the corresponding module or NULL if can't find it.
*************************************************************************/
StelModule* StelModuleMgr::getModule(const QString& moduleID, bool noWarning)
{
	QMap<QString, StelModule*>::ConstIterator iter = modules.find(moduleID);
	if (iter==modules.constEnd())
	{
		if (noWarning==false)
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
	foreach (const PluginDescriptor& desc, getPluginsList())
	{
		if (desc.info.id==moduleID)
		{
			Q_ASSERT(desc.pluginInterface);
			StelModule* sMod = desc.pluginInterface->getStelModule();
			qDebug() << "Loaded plugin " << moduleID << ".";
			pluginDescriptorList[moduleID].loaded=true;
			return sMod;
		}
	}
	qWarning() << "Can't find plugin called " << moduleID;
	return NULL;
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
		if (d.loaded==false)
			continue;
		unloadModule(d.info.id, true);
		qDebug() << "Unloaded plugin " << d.info.id << ".";
	
	}
	// Call update now to make sure that all references to the now deleted plugins modules
	// are removed (fix crashes at application shutdown).
	update();
}

void StelModuleMgr::setPluginLoadAtStartup(const QString& key, bool b)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("plugins_load_at_startup/"+key, b);
	if (pluginDescriptorList.contains(key))
	{
		pluginDescriptorList[key].loadAtStartup=b;
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
	if (pluginDescriptorListLoaded)
	{
		return pluginDescriptorList.values();
	}

	// First list all static plugins.
	// If a dynamic plugin with the same ID exists, it will take precedence on the static one.
	foreach (QObject *plugin, QPluginLoader::staticInstances())
	{
		StelPluginInterface* pluginInterface = qobject_cast<StelPluginInterface*>(plugin);
		if (pluginInterface)
		{
			StelModuleMgr::PluginDescriptor mDesc;
			mDesc.info = pluginInterface->getPluginInfo();
			mDesc.pluginInterface = pluginInterface;
			pluginDescriptorList.insert(mDesc.info.id, mDesc);
		}
	}

	// Then list dynamic libraries from the modules/ directory
	QSet<QString> moduleDirs;
	try
	{
		moduleDirs = StelFileMgr::listContents("modules",StelFileMgr::Directory);
	}
	catch(std::runtime_error& e)
	{
		qWarning() << "ERROR while trying list list modules:" << e.what();
	}

	foreach (QString dir, moduleDirs)
	{
		QString moduleFullPath = QString("modules/") + dir + "/lib" + dir;
#ifdef Q_OS_WIN
		moduleFullPath += ".dll";
#else
#ifdef Q_OS_MAC
		moduleFullPath += ".dylib";
#else
		moduleFullPath += ".so";
#endif
#endif
		try
		{
			moduleFullPath = StelFileMgr::findFile(moduleFullPath, StelFileMgr::File);
		}
		catch (std::runtime_error& e)
		{
			continue;
		}

		QPluginLoader loader(moduleFullPath);
		if (!loader.load())
		{
			qWarning() << "Couldn't load the dynamic library: " << moduleFullPath << ": " << loader.errorString();
			qWarning() << "Plugin " << dir << " will not be loaded.";
			continue;
		}

		QObject* obj = loader.instance();
		if (!obj)
		{
			qWarning() << "Couldn't open the dynamic library: " << moduleFullPath << ": " << loader.errorString();
			qWarning() << "Plugin " << dir << " will not be open.";
			continue;
		}

		StelPluginInterface* pluginInterface = qobject_cast<StelPluginInterface *>(obj);
		if (pluginInterface)
		{
			StelModuleMgr::PluginDescriptor mDesc;
			mDesc.info = pluginInterface->getPluginInfo();
			mDesc.pluginInterface = pluginInterface;
			pluginDescriptorList.insert(mDesc.info.id, mDesc);
		}
	}

	// Load for each plugin if it should be loaded at startup
	QSettings* conf = StelApp::getInstance().getSettings();
	for (QMap<QString, StelModuleMgr::PluginDescriptor>::Iterator iter=pluginDescriptorList.begin();iter!=pluginDescriptorList.end();++iter)
	{
		if (!conf->contains("plugins_load_at_startup/"+iter.key()))
			conf->setValue("plugins_load_at_startup/"+iter.key(), iter.value().info.startByDefault);
		iter->loadAtStartup = conf->value("plugins_load_at_startup/"+iter.key()).toBool();
	}

	pluginDescriptorListLoaded = true;
	return pluginDescriptorList.values();
}
