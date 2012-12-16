/*
 * Stellarium
 * Copyright (C) 2012 Anton Samoylov
 * Copyright (C) 2012 Bogdan Marinov
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

#include "StelShortcutMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelApp.hpp"
#include "StelAppGraphicsWidget.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelTranslator.hpp"
#include "StelShortcutGroup.hpp"

#include <QDateTime>
#include <QAction>
#include <QDebug>
#include <QFileInfo>

StelShortcutMgr::StelShortcutMgr()
{
}

void StelShortcutMgr::init()
{
	stelAppGraphicsWidget = StelMainGraphicsView::getInstance().getStelAppGraphicsWidget();
}

QAction* StelShortcutMgr::addGuiAction(const QString& actionId,
                                       bool temporary,
                                       const QString& text,
                                       const QString& primaryKey,
                                       const QString& altKey,
                                       const QString& groupId,
                                       bool checkable,
                                       bool autoRepeat,
                                       bool global)
{
	if (!shGroups.contains(groupId))
	{
		qWarning() << "Implicitly creating group " << groupId
		           << "for action " << actionId << "; group text is empty";
		shGroups[groupId] = new StelShortcutGroup(groupId);
	}
	return shGroups[groupId]->registerAction(actionId,
	                                         temporary,
	                                         text,
	                                         primaryKey,
	                                         altKey,
	                                         checkable,
	                                         autoRepeat,
	                                         global,
	                                         stelAppGraphicsWidget);
}

void StelShortcutMgr::changeActionPrimaryKey(const QString& actionId, const QString& groupId, QKeySequence newKey)
{
	StelShortcut* shortcut = getShortcut(groupId, actionId);
	if (shortcut)
		shortcut->setPrimaryKey(newKey);
}

void StelShortcutMgr::changeActionAltKey(const QString& actionId, const QString& groupId, QKeySequence newKey)
{
	StelShortcut* shortcut = getShortcut(groupId, actionId);
	if (shortcut)
		shortcut->setAltKey(newKey);
}

void StelShortcutMgr::setShortcutText(const QString &actionId, const QString &groupId, const QString &description)
{
	StelShortcut* shortcut = getShortcut(groupId, actionId);
	if (shortcut)
		shortcut->setText(description);
}

QAction* StelShortcutMgr::getGuiAction(const QString& actionName)
{
	QAction* a = stelAppGraphicsWidget->findChild<QAction*>(actionName);
	if (!a)
	{
		qWarning() << "Can't find action " << actionName;
		return NULL;
	}
	return a;
}

QAction* StelShortcutMgr::getAction(const QString& groupId,
                                    const QString& actionId)
{
	StelShortcutGroup* group = shGroups.value(groupId, 0);
	if (group)
		return group->getAction(actionId);
	else
		return 0;
}

#ifndef DISABLE_SCRIPTING
QAction *StelShortcutMgr::addScriptToAction(const QString &actionId, const QString &script, const QString& scriptPath)
{
	StelShortcut* sc = 0;
	// firstly search in "Scripts" group, all the scripts actions should be there
	if (shGroups.contains("Scripts"))
	{
		sc = shGroups["Scripts"]->getShortcut(actionId);
	}
	// if required action not found in "Scripts" group, iterate over map of all groups, searching
	if (!sc)
	{
		for (QMap<QString, StelShortcutGroup*>::const_iterator it = shGroups.begin(); it != shGroups.end(); ++it)
		{
			sc = it.value()->getShortcut(actionId);
		}
	}
	if (sc)
	{
		sc->setScript(script);
		if (!scriptPath.isEmpty())
		{
			sc->setScriptPath(scriptPath);
		}
		return sc->getAction();
	}
	// else
	qWarning() << "Attempt to set script to non-existing action"
	           << actionId;
	return NULL;
}
#endif

QList<StelShortcutGroup *> StelShortcutMgr::getGroupList() const
{
	return shGroups.values();
}

void StelShortcutMgr::setAllActionsEnabled(bool enable)
{
	foreach (StelShortcutGroup* group, shGroups)
	{
		group->setEnabled(enable);
	}
}

StelShortcut* StelShortcutMgr::getShortcut(const QString& groupId,
                                           const QString& shId)
{
	StelShortcutGroup* group = shGroups.value(groupId, 0);
	if (group)
	{
		StelShortcut* shortcut = group->getShortcut(shId);
		if (!shortcut)
		{
			qWarning() << "Can't find shortcut" << shId
			           << "in group" << groupId;
			return 0;
		}
		return shortcut;
	}
	qWarning() << "Action group" << groupId << "does not exist: "
	           << "Unable to find shortcut" << shId;
	return 0;
}

void StelShortcutMgr::addGroup(const QString &id, QString text, const QString &pluginId)
{
	bool enabled = true;
	if (!pluginId.isEmpty())
	{
		// search for plugin
		StelModuleMgr::PluginDescriptor pluginDescriptor;
		bool found = false;
		foreach (StelModuleMgr::PluginDescriptor desc, StelApp::getInstance().getModuleMgr().getPluginsList())
		{
			if (desc.info.id == pluginId)
			{
				pluginDescriptor = desc;
				found = true;
			}
		}
		if (!found)
		{
			qWarning() << "Can't find plugin with id" << pluginId;
		}
		else
		{
			// if no text provided in file, get text from plugin descriptor
			if (text.isEmpty())
			{
				text = pluginDescriptor.info.displayedName;
			}
			// enable group only when plugin is enabled
			enabled = pluginDescriptor.loadAtStartup;
		}
	}

	// create group
	if (!shGroups.contains(id))
	{
		StelShortcutGroup* newGroup = new StelShortcutGroup(id, text);
		shGroups[id] = newGroup;
		connect(newGroup, SIGNAL(shortcutChanged(StelShortcut*)),
		        this, SIGNAL(shortcutChanged(StelShortcut*)));
	}

	// applying group properties
	shGroups[id]->setEnabled(enabled);
	shGroups[id]->setPluginId(pluginId);
}

bool StelShortcutMgr::copyDefaultFile()
{
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir() + "/data/");
		QString shortcutsFileFullPath = StelFileMgr::getUserDir() + "/data/shortcuts.json";
		qDebug() << "Creating file" << shortcutsFileFullPath;
		QString defaultPath = StelFileMgr::findFile("data/default_shortcuts.json");
		QFile::copy(defaultPath, shortcutsFileFullPath);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "Could not create shortcuts file data/shortcuts.json. Error: " << e.what();
		return false;
	}
	return true;
}

bool StelShortcutMgr::loadShortcuts(const QString& filePath, bool overload)
{
	QVariantMap groups;
	try
	{
		QFile* jsonFile = new QFile(filePath);
		jsonFile->open(QIODevice::ReadOnly);
		groups = StelJsonParser::parse(jsonFile->readAll()).toMap()["groups"].toMap();
		jsonFile->close();
		delete jsonFile;
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "Error while parsing shortcuts file. Error: "
		           << e.what();
		return false;
	}
	// parsing shortcuts groups from file
	for (QVariantMap::const_iterator group = groups.begin(); group != groups.end(); ++group)
	{
		QVariantMap groupMap = group.value().toMap();
		// parsing shortcuts' group properties
		QString groupId = group.key();
		// If no such keys exist, value() will return empty strings.
		QString groupText = groupMap.value("text").toString();
		QString pluginId = groupMap.value("pluginId").toString();
		// add group to map, if it doesn't exist
		if (!groupMap.contains(groupId))
		{
			addGroup(groupId, groupText, pluginId);
		}
		// parsing group's actions (shortcuts)
		QVariantMap actions = groupMap["actions"].toMap();
		for (QVariantMap::const_iterator action = actions.begin(); action != actions.end(); ++action)
		{
			QString actionId = action.key();
			// Skip exisiting actions if overloading is disabled
			if (!overload && getAction(groupId, actionId))
			{
				continue;
			}
			
			QVariantMap actionMap = action.value().toMap();
			QString text = actionMap.value("text").toString();
			QString primaryKey = actionMap["primaryKey"].toString();
			QString altKey = actionMap["altKey"].toString();
			
			// get behavior properties of shortcut
			// TODO: Why are actions considered checkable by default? --BM
			bool checkable = actionMap.value("checkable", true).toBool();
			bool autorepeat = actionMap.value("autorepeat", false).toBool();
			bool global = actionMap.value("global", true).toBool();
			
			// create & init shortcut
			addGuiAction(actionId, false, text, primaryKey, altKey,
			             groupId, checkable, autorepeat, global);
#ifndef DISABLE_SCRIPTING
			// set script if it exist
			QString script = actionMap.value("script").toString();
			if (!script.isEmpty())
			{
				addScriptToAction(actionId, script);
			}
			QString filePath = actionMap.value("scriptFile").toString();
			if (!filePath.isEmpty())
			{
				QString scriptFilePath = StelFileMgr::findFile(filePath);
				if (!QFileInfo(scriptFilePath).exists())
				{
					qWarning() << "Couldn't find script file" << scriptFilePath
						   << "for shortcut" << actionId;
				}
				else
				{
					QFile scriptFile(scriptFilePath);
					if (scriptFile.open(QIODevice::ReadOnly))
					{
						QString code = QString(scriptFile.readAll());
						addScriptToAction(actionId, code, filePath);
						scriptFile.close();
					}
				}
			}
#endif
		}
	}
	return true;
}

void StelShortcutMgr::loadShortcuts()
{
	qDebug() << "Loading shortcuts...";
	QString shortcutsFilePath = StelFileMgr::getUserDir() + "/data/shortcuts.json";
	if (!StelFileMgr::exists(shortcutsFilePath))
	{
		qWarning() << "shortcuts.json doesn't exist, copying default...";
		if (!copyDefaultFile())
		{
			qWarning() << "Couldn't copy default shortcuts file, shortcuts aren't loaded";
			return;
		}
	}
	if (!loadShortcuts(shortcutsFilePath))
	{
		qWarning() << "Invalid shortcuts file, no shortcuts were loaded.";
		return;
	}
	
	// merge with default for getting actual shortcuts info
	QString defaultFilePath = StelFileMgr::getInstallationDir() + "/data/default_shortcuts.json";
	loadShortcuts(defaultFilePath);
}

void StelShortcutMgr::restoreDefaultShortcuts()
{
	QString defaultPath = StelFileMgr::getInstallationDir() + "/data/default_shortcuts.json";
	if (!QFileInfo(defaultPath).exists())
	{
		qWarning() << "Default shortcuts file (" << defaultPath
		           << ") doesn't exist, restore defaults failed.";
		return;
	}
	
	// Reload default shortcuts
	loadShortcuts(defaultPath, true);
	
	// save shortcuts to actual file
	saveShortcuts();
}

void StelShortcutMgr::saveShortcuts()
{
	QString shortcutsFilePath = StelFileMgr::getUserDir() + "/data/shortcuts.json";
	try
	{
		StelFileMgr::findFile(shortcutsFilePath,
		                      StelFileMgr::Flags(StelFileMgr::File | StelFileMgr::Writable));
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "Creating a new shortcuts.json file...";
		QString userDataPath = StelFileMgr::getUserDir() + "/data";
		if (!StelFileMgr::exists(userDataPath))
		{
			if (!StelFileMgr::mkDir(userDataPath))
			{
				qWarning() << "ERROR - cannot create non-existent data directory"
				           << userDataPath;
				qWarning() << "Shortcuts aren't' saved";
				return;
			}
		}
	}

	QFile shortcutsFile(shortcutsFilePath);
	if (!shortcutsFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qWarning() << "ERROR: Could not save shortcuts file"
		           << shortcutsFilePath;
		return;
	}

//	QTextStream stream(&shortcutsFile);
	saveShortcuts(&shortcutsFile);
	shortcutsFile.close();
	qDebug() << "New shortcuts file saved to" << shortcutsFilePath;
}

void StelShortcutMgr::saveShortcuts(QIODevice* output) const
{
	QVariantMap resMap, groupsMap;
	for(QMap<QString, StelShortcutGroup*>::const_iterator it = shGroups.begin(); it != shGroups.end(); ++it)
	{
		groupsMap[it.key()] = it.value()->toQVariant();
	}
	resMap["groups"] = QVariant(groupsMap);
	StelJsonParser::write(QVariant(resMap), output);
}
