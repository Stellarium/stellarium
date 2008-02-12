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
#include <QLabel>
#include <QFile>
#include <QDebug>

#include <cmath>

#include "StelUtils.hpp"
#include "MapView.hpp"

class City 
{
public:
	City(const QString& _name = "", const QString& _state = "", const QString& _country = "", 
		double _longitude = 0.f, double _latitude = 0.f, float zone = 0, int _showatzoom = 0, int _altitude = 0);
	/// Construct a city from a text line using the city .fab format
	static City fromLine(const QString& line);
	
	QString getName(void) { return name; }
	QString getState(void) { return state; }
	QString getCountry(void) { return country; }
	double getLatitude(void) { return latitude; }
	double getLongitude(void) { return longitude; }
	int getShowAtZoom(void) { return showatzoom; }
	int getAltitude(void) { return altitude; }
private:
	QString name;
	QString state;
	QString country;
	double latitude;
	double longitude;
	float zone;
	int showatzoom;
	int altitude;
};


City::City(const QString& _name, const QString& _state, const QString& _country, 
	double _longitude, double _latitude, float zone, int _showatzoom, int _altitude) :
	name(_name), state(_state), country(_country), latitude(_latitude), longitude(_longitude), showatzoom(_showatzoom), altitude(_altitude)
{
}

/// Construct a city from a text line using the city .fab format
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



class CityItem : public QGraphicsEllipseItem
{
protected:
	static QBrush brush;
	static QPen pen;
	
	City city;
	QGraphicsTextItem* label;
	MapView* view;
public:
	CityItem(const City& city_, MapView* view);
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
	virtual void advance(int phase);
};

QBrush CityItem::brush(QColor(255,0,0));
QPen CityItem::pen(QColor(255,0,0));

CityItem::CityItem(const City& city_, MapView* v):
	QGraphicsEllipseItem(- 2, - 2, 4, 4), city(city_), view(v)
{
	setPos(city.getLongitude(), -city.getLatitude());
	
	setBrush(brush);
	setPen(pen);
	setAcceptsHoverEvents(true);
	
	label = new QGraphicsTextItem(city.getName(), this);
	label->setDefaultTextColor(pen.color());
	label->setPos(4, -20);
}

void CityItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	QLabel* cityName = view->parent()->findChild<QLabel*>("cursorLabel");
	cityName->setText(city.getName());
}

void CityItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	QLabel* cityName = view->parent()->findChild<QLabel*>("cursorLabel");
	cityName->setText("");
}

/// Update the scale of the item to compensate the map zoom
void CityItem::advance(int phase)
{
	if (phase != 1) return;
	int cityZoom = city.getShowAtZoom();
	if (cityZoom != 0 && view->getScale() >= cityZoom)
	{
		setVisible(true);
		float scale = 1 / view->getScale();
		resetMatrix();
		this->scale(scale, scale);
	}
	else
	{
		setVisible(false);
	}
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
	resetMatrix();
	QGraphicsView::scale(scale, scale);
	QGraphicsView::centerOn(center);
	scene.advance(); // Update the items scales
}

void MapView::populate(const QString& filename)
{
	qDebug("populate");
	QFile file(filename);
	
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug("can't open");
		return;
	}
	
	QTextStream in(&file);
	while (!in.atEnd())
	{
		QString line = file.readLine();
		if (line.startsWith("#")) continue;
		City city = City::fromLine(line);
		scene.addItem(new CityItem(city, this));
	}
	qDebug("end populate");
}

