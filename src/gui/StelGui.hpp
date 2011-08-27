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
#include "StelGuiBase.hpp"
#include "StelStyle.hpp"

#include <QGraphicsTextItem>

class QGraphicsSceneMouseEvent;
class QAction;
class QTimeLine;
class StelButton;
class BottomStelBar;
class InfoPanel;

//! @class StelGui
//! Main class for the GUI based on QGraphicView.
//! It manages the various qt configuration windows, the buttons bars, the list of QAction/shortcuts.
class StelGui : public QObject, public StelGuiBase
{
	Q_OBJECT
public:
	friend class ViewDialog;
	
	StelGui();
	virtual ~StelGui();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the StelGui object.
	virtual void init(QGraphicsWidget* topLevelGraphicsWidget, StelAppGraphicsWidget* stelAppGraphicsWidget);
	void update();

	StelStyle getStelStyle() const {return currentStelStyle;}
	
	///////////////////////////////////////////////////////////////////////////
	// Methods specific to the StelGui class
	//! Load a Qt style sheet to define the widgets style
	void loadStyle(const QString& fileName);
	
	//! Add a new progress bar in the lower right corner of the screen.
	//! When the progress bar is deleted with removeProgressBar() the layout is automatically rearranged.
	//! @return a pointer to the progress bar
	class QProgressBar* addProgressBar();
	
	//! Get the button bar at the bottom of the screen
	BottomStelBar* getButtonBar();
	
	//! Get the button bar of the left of the screen
	class LeftStelBar* getWindowsButtonBar();

	//! Get the SkyGui instance (useful for adding other interface elements).
	//! It will return a valid object only if called after init().
	class SkyGui* getSkyGui();
	
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
	virtual void forceRefreshGui();
	
	virtual void setVisible(bool b);

	virtual bool getVisible() const;

	virtual bool isCurrentlyUsed() const;
	
	virtual void setInfoTextFilters(const StelObject::InfoStringGroup& aflags);
	virtual const StelObject::InfoStringGroup& getInfoTextFilters() const;
	
	virtual QAction* addGuiActions(const QString& actionName, const QString& text, const QString& shortCut, const QString& helpGroup, bool checkable=true, bool autoRepeat=false);
	
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

	//! change keys when a script is running / not running
	void setScriptKeys(bool b);
	void increaseScriptSpeed();
	void decreaseScriptSpeed();
	void setRealScriptSpeed();

	//! Hide or show the GUI.  Public so it can be called from scripts.
	void setGuiVisible(bool);

private slots:
	void reloadStyle();
	void scriptStarted();
	void scriptStopped();
	//! Load color scheme from the given ini file and section name
	void setStelStyle(const QString& section);
	void quit();
	void updateI18n();
	
private:
	QGraphicsWidget* topLevelGraphicsWidget;
			
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
	ConfigurationDialog* configurationDialog;
#ifdef ENABLE_SCRIPT_CONSOLE
	ScriptConsole scriptConsole;
#endif

	bool flagShowFlipButtons;
	StelButton* flipVert;
	StelButton* flipHoriz;
	
	bool flagShowNebulaBackgroundButton;
	StelButton* btShowNebulaeBackground;

	bool initDone;
	bool guiHidden;
	
	QSizeF savedProgressBarSize;

	// Currently used StelStyle
	StelStyle currentStelStyle;
};

//! Allow to load the GUI as a static plugin
class StelStandardGuiPluginInterface : public QObject, public StelGuiPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelGuiPluginInterface)
public:
	virtual class StelGuiBase* getStelGuiBase() const;
};

#endif // _STELGUI_HPP_
