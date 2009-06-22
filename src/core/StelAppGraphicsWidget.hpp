/*
 * Stellarium
 * Copyright (C) 2009 Fabien Chereau
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

#ifndef _STELAPPGRAPHICSWIDGET_HPP_
#define _STELAPPGRAPHICSWIDGET_HPP_

#include <QGraphicsWidget>

//! @class StelAppGraphicsWidget
//! A QGraphicsWidget encapsulating all the Stellarium main sky view and the embedded widgets
//! such as the moving button bars.
//! It manages initialization of the program and redirects users inputs to the core and GUI
class StelAppGraphicsWidget : public QGraphicsWidget
{
	Q_OBJECT
public:
	StelAppGraphicsWidget();
	~StelAppGraphicsWidget();

	//! Initialize the StelAppGraphicsWidget
	void init();

	//! Paint the whole core of stellarium.
	//! This method is called automatically by the GraphicsView
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget=0);

protected:
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	virtual void wheelEvent(QGraphicsSceneWheelEvent * wheelEvent);
	virtual void resizeEvent(QGraphicsSceneResizeEvent* event);
	
private:
	double previousTime;
};

#endif // _STELAPPGRAPHICSWIDGET_HPP_

