/*
 * Stellarium
 * Copyright (C) 2013 Guillaume Chereau
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

#ifndef _STELACTIONMGR_HPP_
#define _STELACTIONMGR_HPP_

#include <QObject>
#include <QKeySequence>
#include <QList>

class StelAction : public QObject
{
	Q_OBJECT
public:
	friend class StelActionMgr;
	Q_PROPERTY(QString text MEMBER text CONSTANT)
	Q_PROPERTY(QKeySequence shortcut MEMBER keySequence CONSTANT)
	Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
	Q_PROPERTY(bool checkable READ isCheckable CONSTANT)

	StelAction() {}
	//! Don't use this constructor, this is just there to ease the migration from QAction.
	StelAction(QObject *parent)
		: QObject(parent)
		, global(false)
		, target(NULL)
		, property(NULL)
	{}

	StelAction(const QString& actionId,
	           const QString& groupId,
	           const QString& text,
	           QObject* target, const char* slot,
	           const QString& primaryKey="",
	           const QString& altKey="",
	           bool global=false);
	bool isGlobal() const {return global;}
	void setShortcut(const QString& key);
	void setAltShortcut(const QString& key);
	QKeySequence::SequenceMatch matches(const QKeySequence& seq) const;

	QString getId() const {return objectName();}
	QString getGroup() const {return group;}
	const QKeySequence getShortcut() const {return keySequence;}
	const QKeySequence getAltShortcut() const {return altKeySequence;}
	QString getText() const;
	void setText(const QString& value) {text = value; emit changed();}

	bool isCheckable() const;
	bool isChecked() const;
signals:
	void toggled();
	void triggered();
	void changed();
public slots:
	void setChecked(bool);
	void trigger();
	void toggle();
private slots:
	void onTargetPropertyChanged();
private:
	QString group;
	QString text;
	bool global;
	QKeySequence keySequence;
	QKeySequence altKeySequence;
	const QKeySequence defaultKeySequence;
	const QKeySequence defaultAltKeySequence;
	QObject* target;
	const char* property;
};

class StelActionMgr : public QObject
{
	Q_OBJECT
public:
	StelActionMgr();
	~StelActionMgr();
	//! Create and add a new StelAction, connected to an object property or slot.
	//! @param id Global identifier.
	//!	@param groupId Group identifier.
	//! @param text Short human-readable description in English.
	//! @param shortcut Default shortcut.
	//! @param target The QObject the action is linked to.
	//! @param slot The target slot or property that the action will trigger.
	//!             Either a slot name of the form 'func()' and in that case the
	//!             action is made non checkable, a slot name of the form
	//!             'func(bool)' and in that case the action is made checkable,
	//!             or a property name and in that case the action is made
	//!             checkable.
	StelAction* addAction(const QString& id, const QString& groupId, const QString& text,
						  QObject* target, const char* slot,
						  const QString& shortcut="", const QString& altShortcut="",
						  bool global=false);
	Q_INVOKABLE StelAction* findAction(const QString& id);
	bool pushKey(int key, bool global=false);

	QStringList getGroupList() const;
	QList<StelAction*> getActionList(const QString& group) const;

	//! Save current shortcuts to file.
	void saveShortcuts();
	//! Restore the default shortcuts combinations
	void restoreDefaultShortcuts();

public slots:
	//! Enable/disable all actions of application.
	//! need for editing shortcuts without trigging any actions
	//! @todo find out if this is really necessary and why.
	void setAllActionsEnabled(bool value) {actionsEnabled = value;}
private:
	bool actionsEnabled;
	QList<int> keySequence;
};

#endif // _STELACTIONMGR_HPP_
