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
#include <QGraphicsPixmapItem>
#include <QDebug>

class QGraphicsSceneMouseEvent;
class Ui_Form;
class QTimeLine;
class QAction;
class QGraphicsTextItem;

class StelWinBarButton : public QObject, public QGraphicsPixmapItem
{
Q_OBJECT;
public:
	StelWinBarButton(QGraphicsItem* parent, const QPixmap& pixOn, const QPixmap& pixOff, const QPixmap& pixHover=QPixmap(),
		QAction* action=NULL, QGraphicsTextItem* helpLabel=NULL);

signals:
	//! Triggered when the button state changes
	void toggled(bool);
	void triggered();

public slots:
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
	QGraphicsTextItem* helpLabel;
};

class LeftStelBar : public QGraphicsPathItem
{
public:
	LeftStelBar(QGraphicsItem* parent);
	void addButton(StelWinBarButton* button);
private:
	void updatePath();
	double roundSize;
};

class BottomStelBar : public QGraphicsPathItem
{
public:
	BottomStelBar(QGraphicsItem* parent);
	void addButton(StelWinBarButton* button);
private:
	void updatePath();
	double roundSize;
};

//! @class NewGui
//! New GUI based on QGraphicView
class NewGui : public StelModule
{

public:
	NewGui();
	virtual ~NewGui();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the NewGui object.
	virtual void init();
	
	//! Draws all nebula objects.
	virtual double draw(StelCore* core);
	
	//! Update state which is time dependent.
	virtual void update(double deltaTime);
	
	//! Update i18 names from English names according to passed translator.
	//! The translation is done using gettext with translated strings defined 
	//! in translations.h
	virtual void updateI18n();
	
	//! Determines the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
	virtual void glWindowHasBeenResized(int w, int h);
	
private:
	LeftStelBar* winBar;
	BottomStelBar* buttonBar;
	QGraphicsTextItem* buttonHelpLabel;
	Ui_Form* ui;
};

#endif // _NEWGUI_H_
