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

#ifndef STELACTIONMGR_HPP
#define STELACTIONMGR_HPP

#include "StelPropertyMgr.hpp"
#include <QKeySequence>
#include <QList>

//! Wrapper around an argumentless QObject slot or a bool Q_PROPERTY with WRITE method,
//! allowing the slot to be called/property to be toggled using this action object.
//! The action object can be identified by a unique string, and found through StelActionMgr::findAction.
//! Use StelActionMgr::addAction to define a new action.
//! In StelModule subclasses, one can also use StelModule::addAction for convenience.
//!
//! StelAction objects are intended for user interaction. They automatically show up in the hotkey configuration dialog
//! (ShortcutsDialog), and can be bound to interface buttons (StelButton).
//!
//! StelAction internally uses a StelProperty, if connected to a property.
//! A new StelProperty with the name of the action is registered automatically in this case.
//! A NOTIFY signal should be provided, though not strictly necessary, it is really recommended.
//! @note If you want to have a globally accessible reference to arbitrary Q_PROPERTY instances (not just bool),
//! or don't want to expose the property to the user you could use a StelProperty directly registered through the StelPropertyMgr instead.
//! @see StelActionMgr, StelProperty
class StelAction : public QObject
{
	Q_OBJECT
public:
	//! When the StelAction is @ref checkable, this may be used to get/set the current value.
	//! Note that the @ref toggled signal may not be emitted on all changes of the connected property
	//! @warning If used on a non-checkable action, the program may crash.
	Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
	//! If this is true, this StelAction can be toggled.
	//! This is the case when connected to a boolean Q_PROPERTY.
	//! This means the @ref checked property as well as the toggle() function may be used
	//! If false, the StelAction represents a simple argumentless slot call. Using @ref checked or toggle() may
	//! result in an error.
	Q_PROPERTY(bool checkable READ isCheckable)

	//! @see checkable
	bool isCheckable() const {return boolProperty;}
	//! @see checked
	bool isChecked() const {return boolProperty ? boolProperty->getValue().toBool() : false; }
	bool isGlobal() const {return global;}
	//! Defines the key-combination used to call this action
	void setShortcut(const QString& key);
	//! Defines an alternative key-combination
	void setAltShortcut(const QString& key);
	QKeySequence::SequenceMatch matches(const QKeySequence& seq) const;

	//! The ID of this action. Must be unique.
	QString getId() const {return objectName();}
	QString getGroup() const {return group;}
	const QKeySequence getShortcut() const {return keySequence;}
	const QKeySequence getAltShortcut() const {return altKeySequence;}
	QString getText() const;
	void setText(const QString& value) {text = value; emit changed();}
signals:
	//! Emitted when the boolean state of this StelAction changes.
	//! When the action is connected to a StelProperty,
	//! this is equivalent to the StelProperty::valueChanged signal.
	//! In the other cases, this state may not always be emitted correctly
	//! (i.e. when the state changes through other mechanisms than StelAction)
	void toggled(bool);
	//! Emitted after an argumentless slot has been called
	void triggered();
	//! Emitted when additional data associated with this action changed (i.e. shortcuts, text,...)
	void changed();
public slots:
	//! @see checked
	//! @warning If used on a non-checkable action, the program may crash.
	void setChecked(bool);
	//! If the action is @ref checkable, toggle() is called.
	//! Otherwise, the connected slot is invoked.
	void trigger();
	//! If the action is @ref checkable, this toggles the value of
	//! the connected boolean property.
	//! @warning If used on a non-checkable action, the program may crash.
	void toggle();
private slots:
	void propertyChanged(bool);
private:
	friend class StelActionMgr;

	//! Constructor is used by StelActionMgr
	StelAction(const QString& actionId,
		   const QString& groupId,
		   const QString& text,
		   const QString& primaryKey="",
		   const QString& altKey="",
		   bool global=false);
	//! Connect the action to an object property or slot.
	//! @param slot A property or a slot name.  The slot can either have the signature `func()`, and in that
	//! case the action is made not checkable, or have the signature `func(bool)` and in that case the action
	//! is made checkable.  When linked to a property the action is always made checkable.
	void connectToObject(QObject* target, const char* slot);

	QString group;
	QString text;
	bool global;
	QKeySequence keySequence;
	QKeySequence altKeySequence;
	const QKeySequence defaultKeySequence;
	const QKeySequence defaultAltKeySequence;
	QObject* target;
	//If the StelAction is connected to a boolean property with a NOTIFY signal, a StelProperty is used for the connection
	StelProperty* boolProperty;
	QMetaMethod slot;

	// Currently, there is no proper way to handle shortcuts with non latin
	// keyboards layouts.  So for the moment, if we don't use QuickView, we
	// create a QAction added to the main view that will trigger the
	// StelAction when the shortcut is typed.
#ifndef USE_QUICKVIEW
private slots:
	void onChanged();
private:
	class QAction* qAction;
#endif
};

//! Manager for StelAction instances. Allows registration of new actions, and finding an existing one by name.
class StelActionMgr : public QObject
{
	Q_OBJECT
public:
	StelActionMgr();
	~StelActionMgr();
	//! Create and add a new StelAction, connected to an object property or slot.
	//! @param id Global identifier.
	//! @param groupId Group identifier.
	//! @param text Short human-readable description in English.
	//! @param target The QObject the action is linked to.
	//! @param slot The target slot or property that the action will trigger.
	//!             Either a slot name of the form 'func()' and in that case the
	//!             action is made non checkable, a slot name of the form
	//!             'func(bool)' and in that case the action is made checkable,
	//!             or a property name and in that case the action is made
	//!             checkable.
	//! @param shortcut Default shortcut/key combination for this action
	//! @param altShortcut Alternative shortcut
	//! @param global determines QAction shortcut context (not necessary anymore?)
	StelAction* addAction(const QString& id, const QString& groupId, const QString& text,
			      QObject* target, const char* slot,
			      const QString& shortcut="", const QString& altShortcut="",
			      bool global=false);

	//! Create and add a new StelAction, connected to an object slot.
	//! @param id Global identifier.
	//! @param groupId Group identifier.
	//! @param text Short human-readable description in English.
	//! @param context a reference object. When this is deleted, the Lambda function will not be called.
	//! @param lambda a void function (Lambda). This can call slots and other functions.
	//! @param shortcut Default shortcut/key combination for this action
	//! @param altShortcut Alternative shortcut
	//! @param global determines QAction shortcut context (not necessary anymore?)
	StelAction* addAction(const QString& id, const QString& groupId, const QString& text,
			      QObject* context,  std::function<void()> lambda,
			      const QString& shortcut="", const QString& altShortcut="",
			      bool global=false);

	StelAction* findAction(const QString& id);
	StelAction* findActionFromShortcut(const QString& shortcut);
	bool pushKey(int key, bool global=false);

	//! Returns a list of all current StelAction groups
	QStringList getGroupList() const;
	//! Returns all StelActions in the specified group
	QList<StelAction*> getActionList(const QString& group) const;
	//! Returns all registered StelActions
	QList<StelAction*> getActionList() const;
	QStringList getShortcutsList() const;

	//! Save current shortcuts to file.
	void saveShortcuts();
	//! Restore the default shortcuts combinations
	void restoreDefaultShortcuts();
	void restoreDefaultShortcut(StelAction* action);

signals:
	//! Emitted when any action registered with this StelActionMgr is toggled
	//! @param id The id of the action that was toggled
	//! @param value The new value of the action
	void actionToggled(const QString& id, bool value);

	void shortcutsChanged();

public slots:
	//! Enable/disable all actions of application.
	//! need for editing shortcuts without trigging any actions
	//! @todo find out if this is really necessary and why.
	void setAllActionsEnabled(bool value) {actionsEnabled = value;}

private slots:
	void onStelActionToggled(bool val);

private:
	bool actionsEnabled;
	QList<int> keySequence;
};

#endif // STELACTIONMGR_HPP
