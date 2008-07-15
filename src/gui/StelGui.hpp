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
#include <QDebug>
#include <QGraphicsItem>

class QGraphicsSceneMouseEvent;
class QAction;
class QGraphicsTextItem;
class QTimeLine;
class StelButton;

//! The informations about the currently selected object
class InfoPanel : public QGraphicsItem
{
public:
	InfoPanel(QGraphicsItem* parent);
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	virtual QRectF boundingRect() const;
	void setInfoTextFilters(const StelObject::InfoStringGroup& flags) {infoTextFilters=flags;}
	const StelObject::InfoStringGroup& getInfoTextFilters(void) const {return infoTextFilters;}

private:
	QGraphicsTextItem* text;
	StelObjectP object;
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
	//! @param conf application settings object
	//! @param section in the application settings object which contains desired color scheme
	virtual void setColorScheme(const QSettings* conf, const QString& section);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods specific to the StelGui class
	//! Load a Qt style sheet to define the widgets style
	void loadStyle(const QString& fileName);

	//! Get a pointer on the info panel used to display selected object info
	InfoPanel* getInfoPanel(void) { return infoPanel; }
	
	//! Add a new progress bar in the lower right corner of the screen.
	//! When the progress bar is deleted with removeProgressBar() the layout is automatically rearranged.
	//! @return a pointer to the progress bar
	class QProgressBar* addProgessBar();
	
	//! Add a new action managed by the GUI. This method should be used to add new shortcuts to the program
	//! @param actionName qt object name. Used as a reference for later uses
	//! @param text the text to display when hovering, or in the help window
	//! @param shortCut the qt shortcut to use
	//! @param helpGroup hint on how to group the text in the help window
	//! @param checkable whether the action should be checkable
	//! @param autoRepeat whether the action should be autorepeated
	void addGuiActions(const QString& actionName, const QString& text, const QString& shortCut, const QString& helpGroup, bool checkable=true, bool autoRepeat=false);
	
	//! Get a pointer on an action managed by the GUI
	//! @param actionName qt object name for this action
	//! @return a pointer on the QAction object or NULL if don't exist
	QAction* getGuiActions(const QString& actionName);
	
	//! Add a button in a group in the button bar. Group are displayed in alphabetic order.
	//! @param b the button to add
	//! @param groupName the name of the button group to which the button belongs to. If the group doesn't exist yet, a new one is created.
	void addBottomBarButton(StelButton* button, const QString& groupName="defaultGroup");
	
	//! Hide the button associated with the action of the passed name
	void hideBottomBarButton(const QString& actionName);
	
	//! Set whether time must be displayed in the bottom bar
	void setFlagShowTime(bool b);
	
	//! Set whether location info must be displayed in the bottom bar
	void setFlagShowLocation(bool b);
	
private slots:
	//! Update the position of the button bars in the main window
	void updateBarsPos(qreal value);
	
	//! Reload the current Qt Style Sheet (Debug only)
	void reloadStyle();
	
private:
	void retranslateUi(QWidget *Form);
	
	class LeftStelBar* winBar;
	class BottomStelBar* buttonBar;
	InfoPanel* infoPanel;
	class StelBarsPath* buttonBarPath;
	QGraphicsSimpleTextItem* buttonHelpLabel;

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
	
	class StelProgressBarMgr* progressBarMgr;
};

#endif // _NEWGUI_HPP_
