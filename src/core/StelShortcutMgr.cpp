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
QAction* StelShortcutMgr::addGuiAction(const QString& actionId, const QString& text, const QString& primaryKey,
																			 const QString& altKey, const QString &groupId, bool checkable, bool autoRepeat, bool global)
{
	if (!shGroups.contains(groupId))
	{
		qWarning() << "Implicitly creating group " << groupId
							 << "for action " << actionId << "; group text is empty";
		shGroups[groupId] = new StelShortcutGroup(groupId);
	}
	return shGroups[groupId]->registerAction(actionId, text, primaryKey, altKey, checkable,
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

QAction *StelShortcutMgr::addScriptToAction(const QString &actionId, const QString &script)
{
	// firstly search in "Scripts" group, all the scripts actions should be there
	if (shGroups.contains("Scripts"))
	{
		StelShortcut* sc = shGroups["Scripts"]->getShortcut(actionId);
		if (sc)
		{
			sc->setScript(script);
			return sc->getAction();
		}
	}
	// if required action not found in "Scripts" group, iterate over map of all groups, searching
	for (QMap<QString, StelShortcutGroup*>::iterator it = shGroups.begin(); it != shGroups.end(); ++it)
	{
		StelShortcut* sc = it.value()->getShortcut(actionId);
		if (sc)
		{
			sc->setScript(script);
			return sc->getAction();
		}
	}
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
	// creating group
	if (shGroups.contains(id))
	{
		qWarning() << "Dubbing group id - " << id;
	}
	else
	{
		shGroups[id] = new StelShortcutGroup(id, text);
	}
	// applying group properties
	shGroups[id]->setEnabled(enabled);
}

bool StelShortcutMgr::loadShortcuts(const QString &filePath)
{
	QFile jsonFile(filePath);
	jsonFile.open(QIODevice::ReadOnly);
	QMap<QString, QVariant> groups = StelJsonParser::parse(jsonFile.readAll()).toMap()["groups"].toMap();

	// parsing shortcuts groups from file
	for (QMap<QString, QVariant>::iterator group = groups.begin(); group != groups.end(); ++group)
	{
		QMap<QString, QVariant> groupMap = group.value().toMap();
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
		QMap<QString, QVariant> actions = groupMap["actions"].toMap();
		for (QMap<QString, QVariant>::iterator action = actions.begin(); action != actions.end(); ++action)
		{
			QString actionId = action.key();
			QMap<QString, QVariant> actionMap = action.value().toMap();
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
			addGuiAction(actionId, text, primaryKey, altKey, groupId, checkable, autorepeat, global);
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
					addScriptToAction(actionId, QString(scriptFile.readAll()));
					scriptFile.close();
				}
			}
		}
	}
	jsonFile.close();
	return true;
}

void StelShortcutMgr::loadShortcuts()
{
	qDebug() << "Loading shortcuts ...";
	QStringList shortcutFiles;
	try
	{
		shortcutFiles = StelFileMgr::findFileInAllPaths("data/default_shortcuts.json");
	}
	catch(std::runtime_error& e)
	{
		qWarning() << "ERROR while loading default_shortcuts.json (unable to find data/default_shortcuts.json): " << e.what() << endl;
		return;
	}
	foreach (QString shortcutFile, shortcutFiles)
	{
		if (loadShortcuts(shortcutFile))
			break;
		else
		{
			if (shortcutFile.contains(StelFileMgr::getUserDir()))
			{
				QString newName = QString("%1/data/shortcuts-%2.json").arg(StelFileMgr::getUserDir()).arg(QDateTime::currentDateTime().toString("yyyyMMddThhmmss"));
				if (QFile::rename(shortcutFile, newName))
					qWarning() << "Invalid shortcuts file" << shortcutFile << "has been renamed to" << newName;
				else
				{
					qWarning() << "Invalid shortcuts file" << shortcutFile << "cannot be removed!";
					qWarning() << "Please either delete it, rename it or move it elsewhere.";
				}
			}
		}
	}
}
