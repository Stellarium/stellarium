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

#ifndef STELSHORTCUTMGR_HPP
#define STELSHORTCUTMGR_HPP

#include <QObject>

#include "StelAppGraphicsWidget.hpp"

class StelShortcutMgr : public QObject
{
	Q_OBJECT
public:
	StelShortcutMgr();
	
	void init();

	bool loadShortcuts(const QString &filePath);
	void loadShortcuts();

	//! Add a new action managed by the GUI. This method should be used to add new shortcuts to the program
	//! @param actionName qt object name. Used as a reference for later uses
	//! @param text the text to display when hovering, or in the help window
	//! @param shortCut the qt shortcut to use
	//! @param helpGroup hint on how to group the text in the help window
	//! @param checkable whether the action should be checkable
	//! @param autoRepeat whether the action should be autorepeated
	//! @param global whether the action should be global (affect in dialogs)
	QAction* addGuiAction(const QString& actionName, const QString& text, const QString& shortcuts, const QString& helpGroup, bool checkable=true, bool autoRepeat=false, bool global=false);

	//! Get a pointer on an action managed by the GUI
	//! @param actionName qt object name for this action
	//! @return a pointer on the QAction object or NULL if don't exist
	QAction* getGuiAction(const QString& actionName);

signals:
	
public slots:

private:
	StelAppGraphicsWidget* stelAppGraphicsWidget;
	
};

#endif // STELSHORTCUTMGR_HPP
