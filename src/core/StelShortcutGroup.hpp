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

#ifndef STELSHORTCUT_HPP
#define STELSHORTCUT_HPP

#include <QObject>
#include <QMap>
#include <QAction>

QT_FORWARD_DECLARE_CLASS(StelShortcutGroup)

class StelShortcut : public QObject
{
	Q_OBJECT
public:
	StelShortcut(const QString& id, StelShortcutGroup* group, const QString& text,
							 const QString& primaryKey, const QString& altKey,
							 bool checkable, bool autoRepeat = true, bool global = false, QGraphicsWidget *parent = NULL);

	~StelShortcut();

	QAction* getAction() const { return m_action; }
	StelShortcutGroup* getGroup() const { return m_group; }

	QString getId() const { return m_id; }
	QString getText() const { return m_text; }
	QKeySequence getPrimaryKey() const { return m_primaryKey; }
	QKeySequence getAltKey() const { return m_altKey; }
	bool isTemporary() const { return m_temporary; }

	QVariant toQVariant() const;

	void setText(const QString& text);
	void setPrimaryKey(const QKeySequence& key);
	void setAltKey(const QKeySequence& key);
	void setCheckable(bool c);
	void setAutoRepeat(bool ar);
	void setGlobal(bool g);
	void setTemporary(bool temp);
#ifndef DISABLE_SCRIPTING
	void setScript(const QString& scriptText);
	void setScriptPath(const QString& scriptPath);
#endif

signals:
	void shortcutChanged(StelShortcut* shortcut);

#ifndef DISABLE_SCRIPTING
public slots:
	void runScript() const;
#endif

protected slots:
	void updateActionShortcuts();

private:
	QAction* m_action;
	StelShortcutGroup* m_group;

	QString m_id;
	QString m_text;
	QKeySequence m_primaryKey;
	QKeySequence m_altKey;
	bool m_checkable;
	bool m_autoRepeat;
	bool m_global;
	// defines whether shortcut exists only in current session
	bool m_temporary;
	QString m_script;
	QString m_scriptFile;
};



class StelShortcutGroup : public QObject
{
	Q_OBJECT
public:
	StelShortcutGroup(QString id, QString text = "");

	~StelShortcutGroup();

	QAction* registerAction(const QString& actionId, bool temporary, const QString& text, const QString& primaryKey,
													const QString& altKey, bool checkable, bool autoRepeat = true,
                                                    bool global = false, QGraphicsWidget *parent = 0);

	QAction* getAction(const QString &actionId);
	StelShortcut* getShortcut(const QString& id);
	QList<StelShortcut*> getActionList() const;

	QString getId() const { return m_id; }
	QString getText() const { return m_text; }
	QString getPluginId() const {return m_pluginId; }
	bool isEnabled() const { return m_enabled; }

	QVariant toQVariant() const;

signals:
	void shortcutChanged(StelShortcut* shortcut);

public slots:
	// enable/disable all actions of the group
	// need for editing shortcuts without trigging any actions
	void setEnabled(bool enable);
	void setPluginId(const QString& pluginId);
	// clear and delete all shortuts

private:
	QString m_id;
	QString m_text;
	QString m_pluginId;
	bool m_enabled;
	QMap<QString, StelShortcut*> m_shortcuts;
};

#endif // STELSHORTCUT_HPP
