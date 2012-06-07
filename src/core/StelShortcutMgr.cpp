/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
 * Copyright (C) 2012 Timothy Reaves
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

#include "StelIniParser.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelTranslator.hpp"

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
QAction* StelShortcutMgr::addGuiAction(const QString& actionName, const QString& text, const QString& shortcuts, const QString& helpGroup, bool checkable, bool autoRepeat, bool global)
{
	Q_UNUSED(helpGroup);
	QAction* a = new QAction(stelAppGraphicsWidget);
	a->setObjectName(actionName);
	a->setText(q_(text));
	QList<QKeySequence> keySeqs;
	QRegExp shortCutSplitRegEx(";(?!;|$)"); // was ",(?!,|$)" before storing in ini file
	QStringList shortcutStrings = shortcuts.split(shortCutSplitRegEx);
	foreach (QString shortcut, shortcutStrings)
	{
		keySeqs << QKeySequence(shortcut.remove(QRegExp("\\s+")).remove(QRegExp("^\"|\"$")));
	}
	a->setShortcuts(keySeqs);
	a->setCheckable(checkable);
	a->setAutoRepeat(autoRepeat);
	a->setProperty("englishText", QVariant(text));
	if (global)
	{
		a->setShortcutContext(Qt::ApplicationShortcut);
	}
	else
	{
		a->setShortcutContext(Qt::WidgetShortcut);
	}
	stelAppGraphicsWidget->addAction(a);
	return a;
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

bool StelShortcutMgr::loadShortcuts(const QString &filePath)
{
	QSettings scs(filePath, StelIniFormat);
	if (scs.status() != QSettings::NoError)
	{
		qWarning() << "ERROR while parsing" << filePath;
		return false;
	}
	// Shortcuts format example:
	// [search]
	// name = actionShow_Search_Window_Global
	// text = Search window
	// shortcuts = F3; Ctrl+F
	// group = Display Options
	// checkable = 1
	// autorepeat = 0
	// global = 1
	QStringList sections = scs.childGroups();
	foreach (QString section, sections)
	{
		QString name = section;
		QString text = scs.value(section + "/text").toString();
		QString shortcuts = scs.value(section + "/shortcuts").toString();
		QString group = scs.value(section + "/group").toString();
		// TODO: add true/false, on/off, yes/no, y/n cases (not only 1/0) for bool values
		bool checkable = scs.value(section + "/checkable", QVariant("1")).toString().toInt();
		bool autorepeat = scs.value(section + "/autorepeat", QVariant("0")).toString().toInt();
		bool global = scs.value(section + "/global", QVariant("0")).toString().toInt();
		if (name == "")
		{
			qWarning() << "Name missing in " << section << " shortcut, parsing wasn't done";
			continue;
		}
		addGuiAction(name, text, shortcuts, group, checkable, autorepeat, global);
	}
	return true;
}

void StelShortcutMgr::loadShortcuts()
{
	qDebug() << "Loading shortcuts ...";
	QStringList shortcutFiles;
	try
	{
		shortcutFiles = StelFileMgr::findFileInAllPaths("data/shortcuts.ini");
	}
	catch(std::runtime_error& e)
	{
		qWarning() << "ERROR while loading shortcuts.ini (unable to find data/shortcuts.ini): " << e.what() << endl;
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
				QString newName = QString("%1/data/shortcuts-%2.ini").arg(StelFileMgr::getUserDir()).arg(QDateTime::currentDateTime().toString("yyyyMMddThhmmss"));
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
