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
#ifndef DISABLE_SCRIPTING
#include "StelScriptMgr.hpp"
#endif
#include "StelFileMgr.hpp"

#include <QDebug>


StelShortcut::StelShortcut(const QString &id,
                           StelShortcutGroup* group,
                           const QString &text,
                           const QString &primaryKey,
                           const QString &altKey,
                           bool checkable,
                           bool autoRepeat,
                           bool global,
                           QGraphicsWidget *parent) :
    m_id(id), m_temporary(false)
{
	if (parent == NULL)
	{
		parent = StelMainGraphicsView::getInstance().getStelAppGraphicsWidget();
	}
	m_action = new QAction(parent);
	m_action->setObjectName(id);
	m_group = group;

	setText(text);
	setPrimaryKey(primaryKey);
	setAltKey(altKey);
	setCheckable(checkable);
	setAutoRepeat(autoRepeat);
	setGlobal(global);

	parent->addAction(m_action);
}

StelShortcut::~StelShortcut()
{
	delete m_action; m_action = NULL;
}

QVariant StelShortcut::toQVariant() const
{
	QVariantMap resMap;
	resMap["text"] = QVariant(m_text);
	resMap["primaryKey"] = QVariant(m_primaryKey.toString());
	resMap["altKey"] = QVariant(m_altKey.toString());
	resMap["checkable"] = QVariant(m_checkable);
	resMap["autorepeat"] = QVariant(m_autoRepeat);
	resMap["global"] = QVariant(m_global);
	if (!m_scriptFile.isEmpty())
	{
		resMap["scriptFile"] = QVariant(m_scriptFile);
	}
	// store script text only if no script filepath specified
	else if (!m_script.isEmpty())
	{
		resMap["script"] = QVariant(m_script);
	}
	return QVariant(resMap);
}

void StelShortcut::setText(const QString &text)
{
	m_text = text;
	m_action->setText(text);
	m_action->setProperty("englishText", QVariant(text));
	emit shortcutChanged(this);
}

void StelShortcut::setPrimaryKey(const QKeySequence &key)
{
	m_primaryKey = key;
	updateActionShortcuts();
	emit shortcutChanged(this);
}

void StelShortcut::setAltKey(const QKeySequence &key)
{
	m_altKey = key;
	updateActionShortcuts();
	emit shortcutChanged(this);
}

void StelShortcut::setCheckable(bool c)
{
	m_checkable = c;
	m_action->setCheckable(c);
	emit shortcutChanged(this);
}

void StelShortcut::setAutoRepeat(bool ar)
{
	m_autoRepeat = ar;
	m_action->setAutoRepeat(ar);
	emit shortcutChanged(this);
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
	emit shortcutChanged(this);
}

void StelShortcut::setTemporary(bool temp)
{
	m_temporary = temp;
	emit shortcutChanged(this);
}

#ifndef DISABLE_SCRIPTING
void StelShortcut::setScript(const QString &scriptText)
{
	QString scriptsDir = StelFileMgr::findFile("scripts/", StelFileMgr::Directory);
	QString preprocessedScript;
	if (!StelMainGraphicsView::getInstance().getScriptMgr().preprocessScript(
				scriptText, preprocessedScript, scriptsDir))
	{
		qWarning() << "Failed to preprocess script " << m_script;
		return;
	}
	m_script = preprocessedScript;
	connect(m_action, SIGNAL(triggered()), this, SLOT(runScript()));
	emit shortcutChanged(this);
}

void StelShortcut::setScriptPath(const QString &scriptPath)
{
	m_scriptFile = scriptPath;
	emit shortcutChanged(this);
}

void StelShortcut::runScript() const
{
	StelMainGraphicsView::getInstance().getScriptMgr().runPreprocessedScript(m_script);
}
#endif

void StelShortcut::updateActionShortcuts()
{
	QList<QKeySequence> list;
	list << m_primaryKey << m_altKey;
	m_action->setShortcuts(list);
}


StelShortcutGroup::StelShortcutGroup(QString id, QString text) :
	m_id(id), m_text(text), m_enabled(true)
{
}

StelShortcutGroup::~StelShortcutGroup()
{
	foreach (StelShortcut* sh, m_shortcuts)
	{
		delete sh; sh = NULL;
	}
	m_shortcuts.clear();
}

QAction* StelShortcutGroup::registerAction(const QString &actionId, bool temporary, const QString &text, const QString &primaryKey,
																					 const QString &altKey, bool checkable, bool autoRepeat, bool global, QGraphicsWidget *parent)
{
	if (m_shortcuts.contains(actionId))
	{
		StelShortcut *shortcut = getShortcut(actionId);
		shortcut->setTemporary(temporary);
		shortcut->setText(text);
		shortcut->setPrimaryKey(primaryKey);
		shortcut->setAltKey(altKey);
		shortcut->setCheckable(checkable);
		shortcut->setAutoRepeat(autoRepeat);
		shortcut->setGlobal(global);
		return shortcut->getAction();
	}
	StelShortcut* newShortcut = new StelShortcut(actionId, this, text, primaryKey, altKey, checkable, autoRepeat, global, parent);
	connect(newShortcut, SIGNAL(shortcutChanged(StelShortcut*)), this, SIGNAL(shortcutChanged(StelShortcut*)));
	m_shortcuts[actionId] = newShortcut;
	return newShortcut->getAction();
}

QAction *StelShortcutGroup::getAction(const QString &actionId)
{
	if (!m_shortcuts.contains(actionId))
	{
		//qDebug() << "Attempt to get non-existing shortcut by id: " << actionId << endl;
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

QVariant StelShortcutGroup::toQVariant() const
{
	QVariantMap resMap;
	resMap["text"] = QVariant(m_text);
	resMap["pluginId"] = QVariant(m_pluginId);
	QVariantMap actionsMap;
	for (QMap<QString, StelShortcut*>::const_iterator it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it)
	{
		StelShortcut* sc = it.value();
		if (!sc->isTemporary())
		{
			actionsMap[it.key()] = sc->toQVariant();
		}
//		else
//			qDebug() << shortcut->getGroup() << shortcut->getId()
//			         << "is not added as temporary.";
	}
	resMap["actions"] = QVariant(actionsMap);
	return resMap;
}

void StelShortcutGroup::setEnabled(bool enable)
{
	foreach (StelShortcut* sh, m_shortcuts)
	{
		sh->getAction()->setEnabled(enable);
	}
	m_enabled = enable;
}

void StelShortcutGroup::setPluginId(const QString &pluginId)
{
	m_pluginId = pluginId;
}
