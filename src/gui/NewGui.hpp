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

#ifndef _NEWGUI_H_
#define _NEWGUI_H_

#include "StelModule.hpp"
#include "LocationDialog.hpp"
#include "ViewDialog.hpp"
#include "StelObjectType.hpp"
#include "HelpDialog.hpp"
#include "DateTimeDialog.hpp"
#include "SearchDialog.hpp"
#include "ConfigurationDialog.hpp"
#include <QGraphicsPixmapItem>
#include <QDebug>

class QGraphicsSceneMouseEvent;
class QTimeLine;
class QAction;
class QGraphicsTextItem;
class QTimer;

//! Implement a button for use in Stellarium's graphic widgets
class StelButton : public QObject, public QGraphicsPixmapItem
{
Q_OBJECT;
public:
	//! Constructor
	//! @param parent the parent item
	//! @param pixOn the pixmap to display when the button is toggled
	//! @param pixOff the pixmap to display when the button is not toggled
	//! @param pixHover a pixmap slowly blended when mouse is over the button
	//! @param action the associated action. Connections are automatically done with the signals if relevant.
	//! @param helpLabel the label in which the button will display it's help when hovered
	StelButton(QGraphicsItem* parent, const QPixmap& pixOn, const QPixmap& pixOff, const QPixmap& pixHover=QPixmap(),
		QAction* action=NULL, QGraphicsSimpleTextItem* helpLabel=NULL);

	//! Get whether the button is checked
	bool isChecked() const {return checked;}

signals:
	//! Triggered when the button state changes
	void toggled(bool);
	//! Triggered when the button state changes
	void triggered();

public slots:
	//! set whether the button is checked
	void setChecked(bool b);

protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
private slots:
	void animValueChanged(qreal value);	
private:
	QPixmap pixOn;
	QPixmap pixOff;
	QPixmap pixHover;
	bool checked;
	QTimeLine* timeLine;
	QAction* action;
	QGraphicsSimpleTextItem* helpLabel;
};

//! The button bar on the left containing windows toggle buttons
class LeftStelBar : public QGraphicsItem
{
public:
	LeftStelBar(QGraphicsItem* parent);
	~LeftStelBar();
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	virtual QRectF boundingRect() const;
	void addButton(StelButton* button);
private:
	QTimeLine* hideTimeLine;
};

//! The button bar on the bottom containing actions toggle buttons
class BottomStelBar : public QGraphicsItem
{
public:
	BottomStelBar(QGraphicsItem* parent);
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	virtual QRectF boundingRect() const;
	void addButton(StelButton* button);
private:
	void updateText();
	QRectF getButtonsBoundingRect();
	QGraphicsSimpleTextItem* location;
	QGraphicsSimpleTextItem* datetime;
	QGraphicsSimpleTextItem* fov;
	QGraphicsSimpleTextItem* fps;
};

//! The informations about the currently selected object
class InfoPanel : public QGraphicsItem
{
public:
	InfoPanel(QGraphicsItem* parent);
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	virtual QRectF boundingRect() const;
private:
	QGraphicsTextItem* text;
	StelObjectP object;
};

class StelBarsPath : public QGraphicsPathItem
{
public:
	StelBarsPath(QGraphicsItem* parent);
	void updatePath(BottomStelBar* bot, LeftStelBar* lef);
	double getRoundSize() const {return roundSize;}
private:
	double roundSize;
};

//! @class NewGui
//! New GUI based on QGraphicView
class NewGui : public StelModule
{
	Q_OBJECT;
public:
	friend class ViewDialog;
	
	NewGui();
	virtual ~NewGui();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the NewGui object.
	virtual void init();
	
	//! Draws
	virtual double draw(StelCore* core) {return 0;}
	
	//! Update state which is time dependent.
	virtual void update(double deltaTime);
	
	//! Update i18 names from English names according to passed translator.
	//! The translation is done using gettext with translated strings defined 
	//! in translations.h
	virtual void updateI18n();
	
	//! Determines the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
	virtual void glWindowHasBeenResized(int w, int h);

	virtual bool handleMouseMoves(int x, int y);

	//! Load a Qt style sheet to define the widgets style
	void loadStyle(const QString& fileName);
	
private slots:
	void updateBarsPos(qreal value);
	
	//! Reload the current Qt Style Sheet (Debug only)
	void reloadStyle();
	
private:
	void addGuiActions(const QString& actionName, const QString& text, const QString& shortCut, const QString& helpGroup, bool checkable=true, bool autoRepeat=false);
	QAction* getGuiActions(const QString& actionName);
	void retranslateUi(QWidget *Form);
	
	LeftStelBar* winBar;
	BottomStelBar* buttonBar;
	InfoPanel* infoPanel;
	StelBarsPath* buttonBarPath;
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
};

#endif // _NEWGUI_H_
