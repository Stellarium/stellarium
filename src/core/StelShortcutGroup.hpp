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

class StelShortcut : public QObject
{
	Q_OBJECT
public:
	StelShortcut(const QString& aid, const QString& atext, const QString& akeys,
							 bool acheckable, bool aautoRepeat = true, bool aglobal = false, QGraphicsWidget *parent = NULL);

	~StelShortcut();

	QAction* getAction() const { return m_action; }

	QString getId() const { return m_id; }
	QString getText() const { return m_text; }
	QKeySequence getPrimaryKey() const { return m_primaryKey; }
	QKeySequence getAltKey() const { return m_altKey; }
	QString getKeysString() const;

	void setText(const QString& text);
	void setKeys(const QString& keys);
	void setPrimaryKey(const QString& key);
	void setPrimaryKey(const QKeySequence& key);
	void setAltKey(const QString& key);
	void setAltKey(const QKeySequence& key);
	void setCheckable(bool c);
	void setAutoRepeat(bool ar);
	void setGlobal(bool g);
	void setTemporary(bool temp);
	void setScript(const QString& scriptText);

signals:

public slots:
	void runScript();
	void updateShortcuts();

private:
	QAction* m_action;
	QString m_id;
	QString m_text;
	QKeySequence m_primaryKey;
	QKeySequence m_altKey;
	bool m_checkable;
	bool m_autoRepeat;
	bool m_global;
	// defines whether shortcut exists only in current session
	bool temporary;
	QString script;

	QList<QKeySequence> splitShortcuts(const QString& shortcuts);
};



class StelShortcutGroup : public QObject
{
	Q_OBJECT
public:
	StelShortcutGroup(QString id, QString text = "");
	QAction* registerAction(const QString& actionId, const QString& text, const QString& keys, bool checkable,
													bool autoRepeat = true, bool global = false, QGraphicsWidget *parent = false);

	QAction* getAction(const QString &actionId);
	StelShortcut* getShortcut(const QString& id);
	QList<StelShortcut*> getActionList() const;

	QString getId() const { return m_id; }
	QString getText() const { return m_text; }

signals:

public slots:
	// enable/disable all actions of the group
	// need for editing shortcuts without trigging any actions
	void setAllActionsEnabled(bool enable);

private:
	QString m_id;
	QString m_text;
	QMap<QString, StelShortcut*> m_shortcuts;
};

#endif // STELSHORTCUT_HPP
