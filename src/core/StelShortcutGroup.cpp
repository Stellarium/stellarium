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
	m_id(aid), script()
{
	if (parent == NULL)
	{
		parent = StelMainGraphicsView::getInstance().getStelAppGraphicsWidget();
	}
	m_action = new QAction(parent);
	m_action->setObjectName(aid);

	setText(atext);
	setKeys(akeys);
	setCheckable(acheckable);
	setAutoRepeat(aautoRepeat);
	setGlobal(aglobal);

	parent->addAction(m_action);
}

StelShortcut::~StelShortcut()
{
	delete m_action; m_action = NULL;
}

QString StelShortcut::getKeysString() const
{
	return (m_primaryKey.toString() + ";" + m_altKey.toString());
}

void StelShortcut::setText(const QString &text)
{
	m_text = text;
	m_action->setText(q_(text));
	m_action->setProperty("englishText", QVariant(text));
}

void StelShortcut::setKeys(const QString &keys)
{
	QList<QKeySequence> keyList = splitShortcuts(keys);
	int len = keyList.length();
	if (len > 2)
	{
		qWarning() << "Attempt to set more then 2 shortcuts in " << m_id << " action, use only first 2";
	}
	if (len > 0)
	{
		setPrimaryKey(keyList.at(0).toString());
	}
	if (len > 1)
	{
		setAltKey(keyList.at(1).toString());
	}
	// set only first 2 shortcuts
	m_action->setShortcuts(keyList.mid(0, 2));
}

void StelShortcut::setPrimaryKey(const QKeySequence &key)
{
	m_primaryKey = key;
	updateShortcuts();
}

void StelShortcut::setAltKey(const QKeySequence &key)
{
	m_altKey = key;
	updateShortcuts();
}

void StelShortcut::setCheckable(bool c)
{
	m_checkable = c;
	m_action->setCheckable(c);
}

void StelShortcut::setAutoRepeat(bool ar)
{
	m_autoRepeat = ar;
	m_action->setAutoRepeat(ar);
}

void StelShortcut::setGlobal(bool g)
{
	m_global = g;
	if (g)
	{
		m_action->setShortcutContext(Qt::ApplicationShortcut);
	}
	else
	{
		m_action->setShortcutContext(Qt::WidgetShortcut);
	}
}

void StelShortcut::setTemporary(bool temp)
{
	temporary = temp;
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
	connect(m_action, SIGNAL(triggered()), this, SLOT(runScript()));
}

void StelShortcut::runScript()
{
	StelMainGraphicsView::getInstance().getScriptMgr().runPreprocessedScript(script);
}

void StelShortcut::updateShortcuts()
{
	QList<QKeySequence> list;
	list << m_primaryKey << m_altKey;
	m_action->setShortcuts(list);
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



StelShortcutGroup::StelShortcutGroup(QString id, QString text) :
	m_id(id), m_text(text)
{
}

QAction* StelShortcutGroup::registerAction(const QString &actionId, const QString &text, const QString &keys,
																					 bool checkable, bool autoRepeat, bool global, QGraphicsWidget *parent)
{
	if (m_shortcuts.contains(actionId))
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
	m_shortcuts[actionId] = newShortcut;
	return newShortcut->getAction();
}

QAction *StelShortcutGroup::getAction(const QString &actionId)
{
	if (!m_shortcuts.contains(actionId))
	{
		qDebug() << "Attempt to get non-existing shortcut by id: " << actionId << endl;
		return NULL;
	}
	return m_shortcuts[actionId]->getAction();
}

StelShortcut* StelShortcutGroup::getShortcut(const QString &id)
{
	if (!m_shortcuts.contains(id))
	{
		qDebug() << "Attempt to get non-existing shortcut by id: " << id << endl;
		return NULL;
	}
	return m_shortcuts[id];
}

QList<StelShortcut *> StelShortcutGroup::getActionList() const
{
	QList<StelShortcut*> res;
	foreach (StelShortcut* action, m_shortcuts)
	{
		res << action;
	}
	return res;
}

void StelShortcutGroup::setAllActionsEnabled(bool enable)
{
	foreach (StelShortcut* sh, m_shortcuts)
	{
		sh->getAction()->setEnabled(enable);
	}
}
