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

#include <QWheelEvent>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QLabel>
#include <QFile>
#include <QDebug>

#include <cmath>

#include "StelUtils.hpp"
#include "MapView.hpp"


//! @class City
//! Contains informations relative to a city (name, location, etc)
class City 
{
public:
	City(const QString& _name = "", const QString& _state = "", const QString& _country = "", 
		double _longitude = 0.f, double _latitude = 0.f, float zone = 0, int _showatzoom = 0, int _altitude = 0);
	/// Construct a city from a text line using the city .fab format
	static City fromLine(const QString& line);
	
	QString getName(void) const { return name; }
	QString getState(void) const { return state; }
	QString getCountry(void) const { return country; }
	double getLatitude(void) const { return latitude; }
	double getLongitude(void) const { return longitude; }
	int getShowAtZoom(void) const { return showatzoom; }
	int getAltitude(void) const { return altitude; }
	//! Get the larger radius of a circle centered on the city not overlaping other cities radius.
	//! This method is used to implement city selection.
	float getRadius(void) const { return radius; }
	void setRadius(float r) { radius = r; }
private:
	QString name;
	QString state;
	QString country;
	double latitude;
	double longitude;
	float zone;
	int showatzoom;
	int altitude;
	float radius;
};


City::City(const QString& _name, const QString& _state, const QString& _country, 
	double _longitude, double _latitude, float zone, int _showatzoom, int _altitude) :
	name(_name), state(_state), country(_country), latitude(_latitude), longitude(_longitude), showatzoom(_showatzoom), altitude(_altitude)
{
}

//! Construct a city from a text line using the city .fab format
City City::fromLine(const QString& line)
{
	City ret;
	static QRegExp exp("\\s+");
	QStringList tokens = line.split(exp, QString::SkipEmptyParts);
	if (tokens.size() != 8) {
		qDebug() << "error " << tokens;
	}
	ret.name = tokens[0]; 
	ret.state = tokens[1];
	ret.country = tokens[2];
	ret.latitude = StelUtils::get_dec_angle(tokens[3]);
	ret.longitude = StelUtils::get_dec_angle(tokens[4]);
	ret.altitude = tokens[5].toInt();
	ret.zone = tokens[6].toFloat();
	ret.showatzoom = tokens[7].toInt();
	// qDebug() << "loaded" << ret.name << "," << ret.longitude << "," << ret.latitude;
	return ret;
}


//! @class CityItem
//! QGraphicsItem representing a city
class CityItem : public QGraphicsItem
{
protected:
	static QPen pen;
	
	const City* city;
	bool selected;
	MapView* view;
	
	virtual QRectF boundingRect() const;
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
public:
	//! @param city_  the city pointer
	//! @param view   the view we draw on
	CityItem(const City* city_, MapView* view);
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
};

QPen CityItem::pen(QColor(255,0,0));

CityItem::CityItem(const City* city_, MapView* v):
	city(city_), selected(false), view(v)
{
	setPos(city->getLongitude(), -city->getLatitude());
	setAcceptsHoverEvents(true);

}

QRectF CityItem::boundingRect() const
{
	float r = city->getRadius();
	return QRectF(-r, -r, 2 * r, 2 * r);
}

//! Paint the city item
//! We redefine this method to bypass all the matrix transformation and gain speed
void CityItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	int cityZoom = city->getShowAtZoom();
	if (cityZoom == 0) cityZoom = 8;
	if (view->getScale() < cityZoom && !selected) return;
	
	if (selected)
	{
		// painter->setBrush(brush);
		painter->setPen(pen);
	}

	// Draw the point
	QPointF pos = view->mapFromScene(this->pos());
	painter->save();
	painter->resetMatrix();
	painter->drawEllipse((int)pos.x() - 2, (int)pos.y() - 2, 4, 4);
	
	// then the text
	if (city->getShowAtZoom() != 0 || selected)
	{
		painter->drawText((int)pos.x() + 2, (int)pos.y() - 2, city->getName());
	}
	painter->restore();
}

void CityItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	selected = true;
	QLabel* cityName = view->parent()->findChild<QLabel*>("cursorLabel");
	cityName->setText(city->getName());
	cityName->update();
	view->update();
}

void CityItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	selected = false;
	QLabel* cityName = view->parent()->findChild<QLabel*>("cursorLabel");
	cityName->setText("");
	view->update();
}

MapView::MapView(QWidget *parent)
 : QGraphicsView(parent), scale(1), pix("./data/gui/world.png")
{
	
	scene.setSceneRect(-180, -90, 360, 180);
	// Add the world bitmap and scale it so that is is in the same coordinate that the cities
	pixItem = scene.addPixmap(pix);
	pixItem->translate(-180, -90);
	pixItem->scale(360. / pix.size().width(), 180. / pix.size().height());
	
	populate();
	
	setScene(&scene);
	setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	updateScale();
}

MapView::~MapView()
{
}

void MapView::wheelEvent(QWheelEvent* event)
{
	center = mapToScene(event->pos());
	scale *= exp(event->delta() / 240.);
	updateScale();
}

void MapView::updateScale()
{
	this->setUpdatesEnabled(false);
	resetMatrix();
	QGraphicsView::scale(scale, scale);
	QGraphicsView::centerOn(center);
	scene.advance(); // Update the items scales
	this->setUpdatesEnabled(true);
}

void MapView::populate(const QString& filename)
{
	// TODO: use config file to get the cities filename
	qDebug("populate");
	QFile file(filename);
	
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug("can't open");
		return;
	}
	
	// Parse the cities file
	QTextStream in(&file);
	while (!in.atEnd())
	{
		QString line = file.readLine();
		if (line.startsWith("#")) continue;
		cities.append(City::fromLine(line));
	}
	
	// Now we have to set the cities radii
	// For every city the radius is half the distance to the closest other city
	QList<City>::iterator city;
	for(city = cities.begin(); city < cities.end(); ++city)
	{
		float min_dist = 360;
		QPointF p1(city->getLatitude(), city->getLongitude());
		QList<City>::const_iterator city2;
		for(city2 = cities.begin(); city2 < cities.end(); ++city2)
		{
			QPointF p2(city2->getLatitude(), city2->getLongitude());
			QPointF d = p1 - p2;
			float dist = sqrt(pow(d.x(), 2) + pow(d.y(), 2));
			if (dist < min_dist && dist != 0) min_dist = dist;
		}
		city->setRadius(min_dist / 2);
	}
	
	// We add the cities in the view
	for(city = cities.begin(); city < cities.end(); ++city)
	{
		scene.addItem(new CityItem(&*city, this));
	}
	qDebug("end populate");
}

