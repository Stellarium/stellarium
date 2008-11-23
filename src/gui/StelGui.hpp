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

#ifndef _NEWGUI_HPP_
#define _NEWGUI_HPP_

#include "StelModule.hpp"
#include "LocationDialog.hpp"
#include "ViewDialog.hpp"
#include "StelObjectType.hpp"
#include "StelObject.hpp"
#include "HelpDialog.hpp"
#include "DateTimeDialog.hpp"
#include "SearchDialog.hpp"
#include "ConfigurationDialog.hpp"
#include "DownloadPopup.hpp"
#include <QDebug>
#include <QGraphicsItem>

class QGraphicsSceneMouseEvent;
class QAction;
class QGraphicsTextItem;
class QTimeLine;
class StelButton;
class BottomStelBar;

//! The informations about the currently selected object
class InfoPanel : public QGraphicsTextItem
{
public:
	InfoPanel(QGraphicsItem* parent);
	void setInfoTextFilters(const StelObject::InfoStringGroup& flags) {infoTextFilters=flags;}
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
	Q_OBJECT;
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
	virtual void glWindowHasBeenResized(int w, int h);
	virtual bool handleMouseMoves(int x, int y, Qt::MouseButtons b);
	//! Load color scheme from the given ini file and section name
	virtual void setStelStyle(const StelStyle& style);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods specific to the StelGui class
	//! Load a Qt style sheet to define the widgets style
	void loadStyle(const QString& fileName);

	//! Get a pointer on the info panel used to display selected object info
	InfoPanel* getInfoPanel(void) { return infoPanel; }
	
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
	BottomStelBar* getButtonBar() {return buttonBar;}
	
	//! Transform the pixmap so that it look red for night vision mode
	static QPixmap makeRed(const QPixmap& p);

	//! Get whether the buttons toggling image flip are visible
	bool getFlagShowFlipButtons() {return flagShowFlipButtons;}
	
	//! Get whether the button toggling nebulae background is visible
	bool getFlagShowNebulaBackgroundButton() {return flagShowNebulaBackgroundButton;}

	//! returns true if the gui has complted init process.
	bool initComplete(void) {return initDone;}
	
	DownloadPopup* getDownloadPopup() {return &downloadPopup;}

public slots:
	//! Define whether the buttons toggling image flip should be visible
	void setFlagShowFlipButtons(bool b);
	
	//! Define whether the button toggling nebulae background should be visible
	void setFlagShowNebulaBackgroundButton(bool b);

	//! Set the auto-hide status of the horizontal toolbar.
	//! When set to true, the horizontal toolbar will auto-hide itself, only
	//! making an appearance when the mouse is nearby.  When false, it will 
	//! remain on screen.
	//! @param b to hide or not to hide
	bool getAutoHideHorizontalButtonBar() const {return autoHideHorizontalButtonBar;}
	//! Get the auto-hide status of the horizontal toolbar.
	void setAutoHideHorizontalButtonBar(bool b) {autoHideHorizontalButtonBar=b;}
	
	//! Set the auto-hide status of the vertical toolbar.
	//! When set to true, the vertical toolbar will auto-hide itself, only
	//! making an appearance when the mouse is nearby.  When false, it will 
	//! remain on screen.
	//! @param b to hide or not to hide
	bool getAutoHideVerticalButtonBar() const {return autoHideVerticalButtonBar;}
	//! Get the auto-hide status of the vertical toolbar.
	void setAutoHideVerticalButtonBar(bool b) {autoHideVerticalButtonBar=b;}

	//! show or hide the toolbars
	//! @param b when true, toolbars will be shown, else they will be hidden.
	void setHideGui(bool b);
	//! get the current visible status of the toolbars
	bool getHideGui();
	//! toggle the status of the toolbars
	void toggleHideGui(void) {setHideGui(!getHideGui());}

private slots:
	//! Update the position of the button bars in the main window
	void updateBarsPos();
	void reloadStyle();
	void quitStellarium();
	void cancelDownloadAndQuit();
	void dontQuit();
	
	//! Called each time a GUI action is triggered
	void guiActionTriggered(bool b=false);
	
private:
	void retranslateUi(QWidget *Form);
	
	class LeftStelBar* winBar;
	BottomStelBar* buttonBar;
	InfoPanel* infoPanel;
	class StelBarsPath* buttonBarPath;

	QTimeLine* animLeftBarTimeLine;
	QTimeLine* animBottomBarTimeLine;
	
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
	DownloadPopup downloadPopup;
	
	class StelProgressBarMgr* progressBarMgr;
	
	int lastButtonbarWidth;
	
	// The 2 auto hide buttons in the lower left corner
	StelButton* btHorizAutoHide;
	StelButton* btVertAutoHide;
	
	class CornerButtons* autoHidebts;
	
	bool autoHideHorizontalButtonBar;
	bool autoHideVerticalButtonBar;

	bool flagShowFlipButtons;
	class StelButton* flipVert;
	class StelButton* flipHoriz;
	
	bool flagShowNebulaBackgroundButton;
	class StelButton* btShowNebulaeBackground;

	bool initDone;
	bool guiHidden;
};

#endif // _STELGUI_HPP_
