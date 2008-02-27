/*
 * Stellarium
 * Copyright (C) 2008 Guillaume Chereau
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


#ifndef _MAPVIEW_HPP_
#define _MAPVIEW_HPP_

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QPainter>

class City;


//! @class MapView
//! Special QGraphicsView that shows a worl map and permits city selections
class MapView : public QGraphicsView
{
Q_OBJECT
public:
	enum SelectionMode {SELECT_CITIES = 0, SELECT_POSITIONS}; 
protected:
	//! The scene
	QGraphicsScene scene;
	//! The world map image
    QPixmap pix;
    QGraphicsPixmapItem* pixItem;
    //! The pointeur image
    QPixmap pointeurPix;
    //! The seleted position
    QPointF pointeurPos;
    //! The list of all the cities in the map
    QList<City> cities;
    //! Set to true just after we select a new position
    bool justSelected;
public:
	//! Get the scale at wich we are seeing the map
	float getScale() const {return matrix().m11();}
	MapView(QWidget *parent = 0);
	virtual void wheelEvent(QWheelEvent * event);
	virtual void mousePressEvent(QMouseEvent * event);
	virtual void mouseReleaseEvent(QMouseEvent * event);
	virtual void mouseMoveEvent(QMouseEvent * event);
	~MapView();
	//! Select a city
	void select(const City* city);
	//! Select a position
	void select(float longitude, float latitude);
	//! Highlight a city (0) if unhighlighted
	void highlightCity(const City* city);
signals:
	//! Signal emitted when we select a new location
	//! If no city is selected we set city to the empty string
	void positionSelected(float longitude, float latitude, QString city);
	//! Signal emitted when we move over a city
	//! When we move out we emmit the same signal with city set to the empty string
	void positionHighlighted(float longitude, float latitude, QString city);
protected:
	//! Add all the cities into the scene
	void populate(const QString& filename = "data/cities_Earth.fab");
	
	virtual void drawItems(QPainter* painter, int numItems, QGraphicsItem* items[], const QStyleOptionGraphicsItem options[]);
};

#endif // _MAPVIEW_HPP_
