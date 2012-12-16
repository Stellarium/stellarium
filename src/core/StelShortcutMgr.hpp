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

class StelShortcutGroup;
class StelShortcut;
class StelAppGraphicsWidget;

//! Manager of the keyboard shortcuts tied to different features.
//! At the moment, the vast majority of QActions used in Stellarium are
//! defined in a JSON file copied to the user data directory on the first
//! run, similar to the configuration file.
class StelShortcutMgr : public QObject
{
	Q_OBJECT
public:
	StelShortcutMgr();

	void init();

	//! Load shortcuts from an existing file.
	//! @param filePath full path to the file.
	//! @param overload if true, if a shortcut in the file already exists,
	//! replace its keys with the ones in the file. 
	bool loadShortcuts(const QString& filePath, bool overload = false);
	//! Search for file with shortcuts, load shortcuts from it.
	void loadShortcuts();

	//! Save current shortcuts to file.
	void saveShortcuts();

	void saveShortcuts(QIODevice* output) const;

	//! Add a new action managed by the GUI.
	//! This method should be used to add new shortcuts to the program.
	//! @param text Short human-readable description in English.
	//! \ref translation_sec "Mark the string for translation"
	//! with the N_() macro and it will be translated when displayed
	//! in ShortcutsDialog or HelpDialog.
	//! @todo Add explanation of parameters.
	QAction* addGuiAction(const QString& actionId,
	                      bool temporary,
	                      const QString& text,
	                      const QString& primaryKey,
	                      const QString& altKey,
	                      const QString &groupId,
	                      bool checkable = true,
	                      bool autoRepeat = false,
	                      bool global = false);

	void changeActionPrimaryKey(const QString& actionId, const QString& groupId, QKeySequence newKey);
	void changeActionAltKey(const QString& actionId, const QString& groupId, QKeySequence newKey);
	void setShortcutText(const QString& actionId,
	                            const QString& groupId,
	                            const QString& description);

	//! Get a pointer to an action managed by the GUI.
	//! Directly queryies the StelAppGraphicsWidget instance
	//! for a child with the given name.
	//! @param actionName Qt object name for this action
	//! @return a pointer to the QAction object or NULL if doesn't exist
	QAction* getGuiAction(const QString& actionName);

#ifndef DISABLE_SCRIPTING
	//! Bind script evaluation to given action.
	QAction* addScriptToAction(const QString& actionId, const QString& script, const QString& scriptAction = QString());
#endif

	//! Get a list of all shortcut groups.
	QList<StelShortcutGroup*> getGroupList() const;
	
signals:
	void shortcutChanged(StelShortcut* shortcut);

public slots:
	//! Enable/disable all actions of application.
	//! need for editing shortcuts without trigging any actions
	//! @todo find out if this is really necessary and why.
	void setAllActionsEnabled(bool enable);

	//! Restore the default combinations by reloading and resaving.
	void restoreDefaultShortcuts();

private:
	//! Copy the default shortcuts file to the user data directory.
	//! @returns true on success.
	bool copyDefaultFile();

	//! Get a QAction managed by this class.
	//! Searches shGroups.
	//! @returns null pointer if no such action is found.
	QAction* getAction(const QString& groupId, const QString& actionId);
	//! Get a StelShortcut managed by this class.
	//! Searches shGroups.
	//! @returns null pointer if no such shortcut is found.
	StelShortcut* getShortcut(const QString& groupId, const QString& shId);
	//! Create a new shortcut group.
	//! If the group name has been \ref translation_sec 
	//! "marked for translation" (for example, with the N_() macro),
	//! it will be translated when displayed.
	//! @param id Group identifier.
	//! @param text Human-readable group name (in English).
	//! @param pluginId Plugin identifier if the group belongs to a plugin.
	void addGroup(const QString& id,
	              QString text,
	              const QString& pluginId = QString());

	//! Used for obtaining actions by their object names.
	StelAppGraphicsWidget* stelAppGraphicsWidget;
	//! Map of shortcut groups by ID.
	QMap<QString, StelShortcutGroup*> shGroups;
};

#endif // STELSHORTCUTMGR_HPP
