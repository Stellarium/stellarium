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
#include "StelFileMgr.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelTranslator.hpp"
#include "StelShortcutGroup.hpp"

#include <QDateTime>
#include <QAction>
#include <QDebug>

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
QAction* StelShortcutMgr::addGuiAction(const QString& actionId, const QString& text, const QString& shortcuts,
																			 const QString& group, bool checkable, bool autoRepeat, bool global)
{
	if (!shGroups.contains(group))
	{
		shGroups[group] = new StelShortcutGroup(group);
	}
	return shGroups[group]->registerAction(actionId, text, shortcuts, checkable,
																				 autoRepeat, global, stelAppGraphicsWidget);
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

bool StelShortcutMgr::loadShortcuts(const QString &filePath)
{
	QFile jsonFile(filePath);
	jsonFile.open(QIODevice::ReadOnly);

	QMap<QString, QVariant> groups = StelJsonParser::parse(jsonFile.readAll()).toMap()["groups"].toMap();

	for (QMap<QString, QVariant>::iterator group = groups.begin(); group != groups.end(); ++group)
	{
		QMap<QString, QVariant> groupMap = group.value().toMap();
		QString groupId = group.key();
		QString groupPrefix = (groupMap.contains("prefix") ? groupMap["prefix"].toString() + "," : "");
		QMap<QString, QVariant> actions = group.value().toMap()["actions"].toMap();
		for (QMap<QString, QVariant>::iterator action = actions.begin(); action != actions.end(); ++action)
		{
			QString actionId = action.key();
			QMap<QString, QVariant> actionMap = action.value().toMap();
			QString text = actionMap["text"].toString();
			QString shortcuts = groupPrefix + actionMap["shortcuts"].toString();
			bool checkable = actionMap["checkable"].toBool();
			bool autorepeat = actionMap["autorepeat"].toBool();
			bool global = actionMap["global"].toBool();
			if (actionMap.contains("script"))
			{
				qDebug() << "SCRIPT FOUND! \"a\" " << actionMap["script"].toString();
			}
			addGuiAction(actionId, text, shortcuts, groupId, checkable, autorepeat, global);
		}
	}
	return true;
}

void StelShortcutMgr::loadShortcuts()
{
	qDebug() << "Loading shortcuts ...";
	QStringList shortcutFiles;
	try
	{
		shortcutFiles = StelFileMgr::findFileInAllPaths("data/shortcuts.json");
	}
	catch(std::runtime_error& e)
	{
		qWarning() << "ERROR while loading shortcuts.json (unable to find data/shortcuts.json): " << e.what() << endl;
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
