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

#ifndef STELSHORTCUTMGR_HPP
#define STELSHORTCUTMGR_HPP

#include <QObject>
#include <QAction>

QT_FORWARD_DECLARE_CLASS(StelShortcutGroup)
QT_FORWARD_DECLARE_CLASS(StelAppGraphicsWidget)

class StelShortcutMgr : public QObject
{
	Q_OBJECT
public:
	StelShortcutMgr();
	
	void init();

	// load shortcuts from existing file
	bool loadShortcuts(const QString &filePath);
	// search for file with shortcuts, load shortcuts from it
	void loadShortcuts();

	// Add a new action managed by the GUI. This method should be used to add new shortcuts to the program
	QAction* addGuiAction(const QString& actionId, const QString& text, const QString& primaryKey, const QString& altKey,
												const QString& groupId, bool checkable=true, bool autoRepeat=false, bool global=false);

	void changeActionPrimaryKey(const QString& actionId, const QString& groupId, QKeySequence newKey);
	void changeActionAltKey(const QString& actionId, const QString& groupId, QKeySequence newKey);

	//! Get a pointer on an action managed by the GUI
	//! @param actionName qt object name for this action
	//! @return a pointer on the QAction object or NULL if don't exist
	QAction* getGuiAction(const QString& actionName);
	// unlike getGuiAction with only actionId parameter search by
	// iterationg over map of groups and map of shortcuts in each group
	QAction* getGuiAction(const QString& groupId, const QString& actionId);

	// bind script evaluation to given action
	QAction* addScriptToAction(const QString& actionId, const QString& script);

	// get list of all group of shortcuts
	QList<StelShortcutGroup*> getGroupList() const;

signals:
	
public slots:
	// enable/disable all actions of application
	// need for editing shortcuts without trigging any actions
	void setAllActionsEnabled(bool enable);

private:
	// init and add to map group with parsed from file properties
	void addGroup(const QString& id, QString text, const QString& pluginId = "");

	// pointer to StelAppGraphicsWidget, used for obtaining actions by their object names
	StelAppGraphicsWidget* stelAppGraphicsWidget;
	// QMap, containing all shortcuts groups
	QMap<QString, StelShortcutGroup*> shGroups;
};

#endif // STELSHORTCUTMGR_HPP
