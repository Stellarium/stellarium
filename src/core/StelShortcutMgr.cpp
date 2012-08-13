/*
 * Stellarium
 * Copyright (C) 2012 Anton Samoylov
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

// Note: "text" and "helpGroup" must be in English -- this method and the help
// dialog take care of translating them. Of course, they still have to be
// marked for translation using the N_() macro.
QAction* StelShortcutMgr::addGuiAction(const QString& actionId, bool temporary, const QString& text, const QString& primaryKey,
																			 const QString& altKey, const QString &groupId, bool checkable, bool autoRepeat, bool global)
{
	if (!shGroups.contains(groupId))
	{
		qWarning() << "Implicitly creating group " << groupId
							 << "for action " << actionId << "; group text is empty";
		shGroups[groupId] = new StelShortcutGroup(groupId);
	}
	return shGroups[groupId]->registerAction(actionId, temporary, text, primaryKey, altKey, checkable,
																					 autoRepeat, global, stelAppGraphicsWidget);
}

void StelShortcutMgr::changeActionPrimaryKey(const QString &actionId, const QString &groupId, QKeySequence newKey)
{
	if (!shGroups.contains(groupId))
	{
		qWarning() << "Attempt to find action" << actionId << " of non-existing group " << groupId;
		return;
	}
	shGroups[groupId]->getShortcut(actionId)->setPrimaryKey(newKey);
}

void StelShortcutMgr::changeActionAltKey(const QString &actionId, const QString &groupId, QKeySequence newKey)
{
	if (!shGroups.contains(groupId))
	{
		qWarning() << "Attempt to find action" << actionId << " of non-existing group " << groupId;
		return;
	}
	shGroups[groupId]->getShortcut(actionId)->setAltKey(newKey);
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

QAction *StelShortcutMgr::getGuiAction(const QString &groupId, const QString &actionId)
{
	if (shGroups.contains(groupId))
	{
		QAction* a = shGroups[groupId]->getAction(actionId);
		if (!a)
		{
			qWarning() << "Can't find action " << actionId;
			return NULL;
		}
		return a;
	}
	qWarning() << "Attempt to find action" << actionId << " of non-existing group " << groupId;
	return NULL;
}

QAction *StelShortcutMgr::addScriptToAction(const QString &actionId, const QString &script, const QString& scriptPath)
{
	StelShortcut* sc;
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
	qWarning() << "Attempt to set script to non-existing action " << actionId;
	return NULL;
}

QList<StelShortcutGroup *> StelShortcutMgr::getGroupList() const
{
	QList<StelShortcutGroup*> res;
	foreach (StelShortcutGroup* group, shGroups)
	{
		res << group;
	}
	return res;
}

void StelShortcutMgr::setAllActionsEnabled(bool enable)
{
	foreach (StelShortcutGroup* group, shGroups)
	{
		group->setEnabled(enable);
	}
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
			qWarning() << "Can't find plugin with id " << pluginId;
		}
		else
		{
			// if no text provided in file, get text from plugin descriptor
			if (text.isEmpty())
			{
				text = pluginDescriptor.info.displayedName + " Plugin";
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
		connect(newGroup, SIGNAL(shortcutChanged(StelShortcut*)), this, SIGNAL(shortcutChanged(StelShortcut*)));
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
		qDebug() << "Creating file " << shortcutsFileFullPath;
		QFile::copy(StelFileMgr::findFile("data/default_shortcuts.json"), shortcutsFileFullPath);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "Could not create shortcuts file data/shortcuts.json. Error: " << e.what();
		return false;
	}
	return true;
}

bool StelShortcutMgr::loadShortcuts(const QString &filePath)
{
	QFile* jsonFile;
	QVariantMap groups;
	try
	{
		jsonFile = new QFile(filePath);
		jsonFile->open(QIODevice::ReadOnly);
		groups = StelJsonParser::parse(jsonFile->readAll()).toMap()["groups"].toMap();
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "Error while parsing shortcuts file. Error: " << e.what();
		return false;
	}
	// parsing shortcuts groups from file
	for (QVariantMap::const_iterator group = groups.begin(); group != groups.end(); ++group)
	{
		QVariantMap groupMap = group.value().toMap();
		// parsing shortcuts' group properties
		QString groupId = group.key();
		QString groupText;
		if (groupMap.contains("text"))
		{
			groupText = groupMap["text"].toString();
		}
		QString pluginId;
		if (groupMap.contains("pluginId"))
		{
			pluginId = groupMap["pluginId"].toString();
		}
		// add group to map
		addGroup(groupId, groupText, pluginId);
		// parsing group's actions (shortcuts)
		QVariantMap actions = groupMap["actions"].toMap();
		for (QVariantMap::const_iterator action = actions.begin(); action != actions.end(); ++action)
		{
			QString actionId = action.key();
			QVariantMap actionMap = action.value().toMap();
			// parsing action (shortcut) properties
			QString text;
			if (actionMap.contains("text"))
			{
				text = actionMap["text"].toString();
			}
			// get primary and alternative keys of shortcut
			QString primaryKey = actionMap["primaryKey"].toString();
			QString altKey = actionMap["altKey"].toString();
			// get behavior properties of shortcut
			bool checkable;
			if (actionMap.contains("checkable"))
			{
				checkable = actionMap["checkable"].toBool();
			}
			else
			{
				// default value
				checkable = true;
			}
			bool autorepeat;
			if (actionMap.contains("autorepeat"))
			{
				autorepeat = actionMap["autorepeat"].toBool();
			}
			else
			{
				// default value
				autorepeat = false;
			}
			bool global;
			if (actionMap.contains("global"))
			{
				global = actionMap["global"].toBool();
			}
			else
			{
				// default value
				global = true;
			}
			// create & init shortcut
			addGuiAction(actionId, false, text, primaryKey, altKey, groupId, checkable, autorepeat, global);
			// set script if it exist
			if (actionMap.contains("script"))
			{
				addScriptToAction(actionId, actionMap["script"].toString());
			}
			if (actionMap.contains("scriptFile"))
			{
				QString scriptFilePath = StelFileMgr::findFile(actionMap["scriptFile"].toString());
				if (!QFileInfo(scriptFilePath).exists())
				{
					qWarning() << "Couldn't find file " << actionMap["scriptFile"] <<
												"for shortcut " << actionId;
				}
				else
				{
					QFile scriptFile(scriptFilePath);
					scriptFile.open(QIODevice::ReadOnly);
					addScriptToAction(actionId, QString(scriptFile.readAll()), scriptFilePath);
					scriptFile.close();
				}
			}
		}
	}
	jsonFile->close();
	delete jsonFile;
	return true;
}

void StelShortcutMgr::loadShortcuts()
{
	qDebug() << "Loading shortcuts ...";
	QString shortcutsFilePath = StelFileMgr::getUserDir() + "/data/shortcuts.json";
	if (!StelFileMgr::exists(shortcutsFilePath))
	{
		qWarning() << "data/shortcuts.json doesn't exist, copy default";
		if (!copyDefaultFile())
		{
			qWarning() << "Couldn't copy default shortcuts file, shortcuts aren't loaded";
			return;
		}
	}
	if (!loadShortcuts(shortcutsFilePath))
	{
		qWarning() << "Invalid shortcuts file, shortcuts aren't loaded.";
	}
}

void StelShortcutMgr::restoreDefaultShortcuts()
{
	QString defaultPath = StelFileMgr::getInstallationDir() + "/data/default_shortcuts.json";
	if (!QFileInfo(defaultPath).exists())
	{
		qWarning() << "Default shortcuts file(" << defaultPath << ") doesn't exist, restore defaults failed";
		return;
	}
	loadShortcuts(defaultPath);
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
		qWarning() << "Creating non-existent shortcuts.json file... ";
		if (!StelFileMgr::exists(StelFileMgr::getUserDir() + "/data"))
		{
			if (!StelFileMgr::mkDir(StelFileMgr::getUserDir() + "/data"))
			{
				qWarning() << "ERROR - cannot create non-existent data directory" <<
											StelFileMgr::getUserDir() + "/data";
				qWarning() << "Shortcuts aren't' saved";
				return;
			}
		}
		qWarning() << "Will create a new shortcuts file: " << shortcutsFilePath;
	}

	QFile shortcutsFile(shortcutsFilePath);
	if (!shortcutsFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qWarning() << "ERROR: Could not open shortcuts file: " << shortcutsFilePath;
		return;
	}

//	QTextStream stream(&shortcutsFile);
	saveShortcuts(&shortcutsFile);
	shortcutsFile.close();
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
