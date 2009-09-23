/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _STELGUI_HPP_
#define _STELGUI_HPP_

#include "StelModule.hpp"
#include "StelObject.hpp"
#include "LocationDialog.hpp"
#include "ViewDialog.hpp"
#include "HelpDialog.hpp"
#include "DateTimeDialog.hpp"
#include "SearchDialog.hpp"
#include "ConfigurationDialog.hpp"
#ifdef ENABLE_SCRIPT_CONSOLE
#include "ScriptConsole.hpp"
#endif

#include <QGraphicsTextItem>

class QGraphicsSceneMouseEvent;
class QAction;
class QTimeLine;
class StelButton;
class BottomStelBar;

//! The informations about the currently selected object
class InfoPanel : public QGraphicsTextItem
{
public:
	InfoPanel(QGraphicsItem* parent);
	void setInfoTextFilters(const StelObject::InfoStringGroup& aflags) {infoTextFilters=aflags;}
	const StelObject::InfoStringGroup& getInfoTextFilters(void) const {return infoTextFilters;}
	void setTextFromObjects(const QList<StelObjectP>&);
private:
	StelObject::InfoStringGroup infoTextFilters;
};

//! @class StelGui
//! Main class for the GUI based on QGraphicView.
//! It manages the various qt configuration windows, the buttons bars, the list of QAction/shortcuts.
class StelGui : public StelModule
{
	Q_OBJECT
public:
	friend class ViewDialog;
	
	StelGui();
	virtual ~StelGui();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the StelGui object.
	virtual void init();
	virtual void draw(StelCore* core) {;}
	virtual void update(double deltaTime);
	virtual void updateI18n();
	virtual double getCallOrder(StelModuleActionName actionName) const;
	//! Load color scheme from the given ini file and section name
	virtual void setStelStyle(const StelStyle& style);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods specific to the StelGui class
	//! Load a Qt style sheet to define the widgets style
	void loadStyle(const QString& fileName);

	//! Get a pointer on the info panel used to display selected object info
	InfoPanel* getInfoPanel(void);
	
	//! Add a new progress bar in the lower right corner of the screen.
	//! When the progress bar is deleted with removeProgressBar() the layout is automatically rearranged.
	//! @return a pointer to the progress bar
	class QProgressBar* addProgressBar();
	
	//! Add a new action managed by the GUI. This method should be used to add new shortcuts to the program
	//! @param actionName qt object name. Used as a reference for later uses
	//! @param text the text to display when hovering, or in the help window
	//! @param shortCut the qt shortcut to use
	//! @param helpGroup hint on how to group the text in the help window
	//! @param checkable whether the action should be checkable
	//! @param autoRepeat whether the action should be autorepeated
	//! @param persistenceName name of the attribute for persistence in the config.ini. Not persistent if empty.
	QAction* addGuiActions(const QString& actionName, const QString& text, const QString& shortCut, const QString& helpGroup, bool checkable=true, bool autoRepeat=false, const QString& persistenceName=QString());
	
	//! Get a pointer on an action managed by the GUI
	//! @param actionName qt object name for this action
	//! @return a pointer on the QAction object or NULL if don't exist
	QAction* getGuiActions(const QString& actionName);
	
	//! Get the button bar at the bottom of the screen
	BottomStelBar* getButtonBar();
	
	//! Get the button bar of the left of the screen
	class LeftStelBar* getWindowsButtonBar();
	
	//! Transform the pixmap so that it look red for night vision mode
	static QPixmap makeRed(const QPixmap& p);

	//! Get whether the buttons toggling image flip are visible
	bool getFlagShowFlipButtons() {return flagShowFlipButtons;}
	
	//! Get whether the button toggling nebulae background is visible
	bool getFlagShowNebulaBackgroundButton() {return flagShowNebulaBackgroundButton;}

	//! returns true if the gui has complted init process.
	bool initComplete(void) {return initDone;}

#ifdef ENABLE_SCRIPT_CONSOLE
	ScriptConsole* getScriptConsole() {return &scriptConsole;}
#endif

	//! Used to force a refreshing of the GUI elements such as the button bars.
	void forceRefreshGui();
	
public slots:
	//! Define whether the buttons toggling image flip should be visible
	void setFlagShowFlipButtons(bool b);
	
	//! Define whether the button toggling nebulae background should be visible
	void setFlagShowNebulaBackgroundButton(bool b);

	//! Get the auto-hide status of the horizontal toolbar.
	bool getAutoHideHorizontalButtonBar() const;
	//! Set the auto-hide status of the horizontal toolbar.
	//! When set to true, the horizontal toolbar will auto-hide itself, only
	//! making an appearance when the mouse is nearby.  When false, it will 
	//! remain on screen.
	//! @param b to hide or not to hide	
	void setAutoHideHorizontalButtonBar(bool b);
	
	//! Get the auto-hide status of the vertical toolbar.
	bool getAutoHideVerticalButtonBar() const;
	//! Set the auto-hide status of the vertical toolbar.
	//! When set to true, the vertical toolbar will auto-hide itself, only
	//! making an appearance when the mouse is nearby.  When false, it will 
	//! remain on screen.
	//! @param b to hide or not to hide
	void setAutoHideVerticalButtonBar(bool b);

	//! show or hide the toolbars
	//! @param b when true, toolbars will be shown, else they will be hidden.
	void setHideGui(bool b);
	//! get the current visible status of the toolbars
	bool getHideGui();

	//! change keys when a script is running / not running
	void setScriptKeys(bool b);
	void increaseScriptSpeed();
	void decreaseScriptSpeed();
	void setRealScriptSpeed();

private slots:
	void reloadStyle();
	
	//! Called each time a GUI action is triggered
	void guiActionTriggered(bool b=false);
	
private:
	
	class SkyGui* skyGui;
	
	StelButton* buttonTimeRewind;
	StelButton* buttonTimeRealTimeSpeed;
	StelButton* buttonTimeCurrent;
	StelButton* buttonTimeForward;
	
	StelButton* buttonGotoSelectedObject;
	
	LocationDialog locationDialog;
	HelpDialog helpDialog;
	DateTimeDialog dateTimeDialog;
	SearchDialog searchDialog;
	ViewDialog viewDialog;
	ConfigurationDialog configurationDialog;
#ifdef ENABLE_SCRIPT_CONSOLE
	ScriptConsole scriptConsole;
#endif

	bool flagShowFlipButtons;
	class StelButton* flipVert;
	class StelButton* flipHoriz;
	
	bool flagShowNebulaBackgroundButton;
	class StelButton* btShowNebulaeBackground;

	bool initDone;
	bool guiHidden;
	
	QSizeF savedProgressBarSize;
};

#endif // _STELGUI_HPP_
