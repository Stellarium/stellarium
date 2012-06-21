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

#include "StelShortcutGroup.hpp"
#include "StelAppGraphicsWidget.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelTranslator.hpp"
#include "StelScriptMgr.hpp"
#include "StelFileMgr.hpp"

#include <QDebug>

StelShortcut::StelShortcut(const QString &aid, const QString &atext, const QString &akeys,
													 bool acheckable, bool aautoRepeat, bool aglobal, QGraphicsWidget* parent) :
	id(aid), script()
{
	if (parent == NULL)
	{
		parent = StelMainGraphicsView::getInstance().getStelAppGraphicsWidget();
	}
	action = new QAction(parent);
	action->setObjectName(aid);

	setText(atext);
	setKeys(akeys);
	setCheckable(acheckable);
	setAutoRepeat(aautoRepeat);
	setGlobal(aglobal);

	parent->addAction(action);
}

StelShortcut::~StelShortcut()
{
	delete action; action = NULL;
}

void StelShortcut::setText(const QString &atext)
{
	text = atext;
	action->setText(q_(atext));
	action->setProperty("englishText", QVariant(atext));
}

void StelShortcut::setKeys(const QString &akeys)
{
	keys = akeys;
	action->setShortcuts(splitShortcuts(keys));
}

void StelShortcut::setCheckable(bool c)
{
	checkable = c;
	action->setCheckable(c);
}

void StelShortcut::setAutoRepeat(bool ar)
{
	autoRepeat = ar;
	action->setAutoRepeat(ar);
}

void StelShortcut::setGlobal(bool g)
{
	global = g;
	if (g)
	{
		action->setShortcutContext(Qt::ApplicationShortcut);
	}
	else
	{
		action->setShortcutContext(Qt::WidgetShortcut);
	}
}

void StelShortcut::setScript(const QString &scriptText)
{
	QString scriptsDir = StelFileMgr::findFile("scripts/", StelFileMgr::Directory);
	QString preprocessedScript;
	if (!StelMainGraphicsView::getInstance().getScriptMgr().preprocessScript(
				scriptText, preprocessedScript, scriptsDir))
	{
		qWarning() << "Failed to preprocess script " << script;
		return;
	}
	script = preprocessedScript;
	connect(action, SIGNAL(triggered()), this, SLOT(runScript()));
}

void StelShortcut::runScript()
{
	StelMainGraphicsView::getInstance().getScriptMgr().runPreprocessedScript(script);
}


QList<QKeySequence> StelShortcut::splitShortcuts(const QString &shortcuts)
{
	QList<QKeySequence> keySeqs;
	QRegExp shortCutSplitRegEx(";(?!;|$)");
	QStringList shortcutStrings = shortcuts.split(shortCutSplitRegEx);
	foreach (QString shortcut, shortcutStrings)
	{
		keySeqs << QKeySequence(shortcut.remove(QRegExp("\\s+")).remove(QRegExp("^\"|\"$")));
	}
	return keySeqs;
}



StelShortcutGroup::StelShortcutGroup(QString id) :
	id(id)
{
}

QAction* StelShortcutGroup::registerAction(const QString &actionId, const QString &text, const QString &keys,
																			 bool checkable, bool autoRepeat, bool global, QGraphicsWidget *parent)
{
	if (shortcuts.contains(actionId))
	{
		qWarning() << "Attempt to add an existing shortcut with id: " << actionId << ", rewrite properties";
		StelShortcut *shortcut = getShortcut(actionId);
		shortcut->setText(text);
		shortcut->setKeys(keys);
		shortcut->setCheckable(checkable);
		shortcut->setAutoRepeat(autoRepeat);
		shortcut->setGlobal(global);
		return shortcut->getAction();
	}
	StelShortcut* newShortcut = new StelShortcut(actionId, text, keys, checkable, autoRepeat, global, parent);
	shortcuts[actionId] = newShortcut;
	return newShortcut->getAction();
}

QAction *StelShortcutGroup::getAction(const QString &actionId)
{
	if (!shortcuts.contains(actionId))
	{
		qDebug() << "Attempt to get non-existing shortcut by id: " << actionId << endl;
		return NULL;
	}
	return shortcuts[actionId]->getAction();
}

StelShortcut* StelShortcutGroup::getShortcut(const QString &id)
{
	if (!shortcuts.contains(id))
	{
		qDebug() << "Attempt to get non-existing shortcut by id: " << id << endl;
		return NULL;
	}
	return shortcuts[id];
}
