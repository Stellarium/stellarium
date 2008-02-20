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

class City;


//! @class MapView
//! Special QGraphicsView that shows a worl map and permits city selections
class MapView : public QGraphicsView
{
Q_OBJECT
protected:
	float scale;
	//! The center position used when scaling
	QPointF center;
	//! The scene
	QGraphicsScene scene;
	//! The world map image
    QPixmap pix;
    QGraphicsPixmapItem* pixItem;
    //! The list of all the cities in the map
    QList<City> cities;
public:
	//! Get the scale at wich we are seeing the map
	float getScale() const {return scale;}
	MapView(QWidget *parent = 0);
	void updateScale();
	void wheelEvent(QWheelEvent * event);
	~MapView();
	//! Select a city
	void select(const City* city);
protected:
	//! Add all the cities into the scene
	void populate(const QString& filename = "data/cities_Earth.fab");

};

#endif // _MAPVIEW_HPP_
