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
#include <QScrollBar>

#include <cmath>

#include "StelUtils.hpp"
#include "StelFileMgr.hpp"
#include "StelApp.hpp"
#include "MapView.hpp"
#include "SpinBox.hpp"


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

//! Construct a city from a text line using the city .fab format
City City::fromLine(const QString& line)
{
	City ret;
	static QRegExp exp("\\s+");
	QStringList tokens = line.split(exp, QString::SkipEmptyParts);
	if (tokens.size() != 8) {
		qWarning() << "error " << tokens;
	}
	ret.name = tokens[0]; 
	ret.state = tokens[1];
	ret.country = tokens[2];
	ret.latitude = StelUtils::get_dec_angle(tokens[3]);
	ret.longitude = StelUtils::get_dec_angle(tokens[4]);
	ret.altitude = tokens[5].toInt();
	ret.zone = tokens[6].toFloat();
	ret.showatzoom = tokens[7].toInt();
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
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
};

QPen CityItem::pen(QColor(255,0,0));

CityItem::CityItem(const City* city_, MapView* v):
	city(city_), selected(false), view(v)
{
	setPos(city->getLongitude(), -city->getLatitude());
	setAcceptsHoverEvents(true);
	setCursor(Qt::ArrowCursor);
}

QRectF CityItem::boundingRect() const
{
	return QRect(-10, -10, 20, 20);
}

//! Paint the city item
//! We redefine this method to bypass all the matrix transformation and gain speed
void CityItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	int cityZoom = city->getShowAtZoom();
	if (cityZoom == 0) cityZoom = 8;
	if (view->getScale() < cityZoom && !selected) 
	{
		// If the city is not visible, we can't select it either
		// setEnabled(false);
		setAcceptsHoverEvents(false);
		setAcceptedMouseButtons(0);
		return;
	}
	// If the city is visible we can select it with the mouse
	// setEnabled(true);
	setAcceptsHoverEvents(true);
	setAcceptedMouseButtons(Qt::LeftButton);
	// We scale the city so that the selection shape stays the same
	float scale = 1. / view->getScale();
	setMatrix(QMatrix(scale, 0, 0, scale, 0, 0));
	
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

void CityItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	view->select(city);
}

MapView::MapView(QWidget *parent)
 : QGraphicsView(parent), pix("./data/gui/world.png"), pointeurPix("./data/gui/map-pointeur.png")
{
	
	scene.setSceneRect(-180, -90, 360, 180);
	// Add the world bitmap and scale it so that is is in the same coordinate that the cities
	pixItem = scene.addPixmap(pix);
	pixItem->translate(-180, -90);
	pixItem->scale(360. / pix.size().width(), 180. / pix.size().height());
	
	float scale = 2; // TODO: find a way to get the real initial scalling !!!
	QGraphicsView::scale(scale, scale);
	
	populate();
	
	setScene(&scene);
	setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
}

MapView::~MapView()
{
}

void MapView::wheelEvent(QWheelEvent* event)
{
	float scale = exp(event->delta() / 240.);	// We use an exponential scaling
	QGraphicsView::scale(scale, scale);
}

void MapView::populate(const QString& filename)
{
	QString cityDataPath;
	try
	{
		cityDataPath = StelApp::getInstance().getFileMgr().findFile(filename);
	}
	catch (exception& e)
	{
		qWarning() << "ERROR: Failed to locate city data for location map: " << filename << e.what();
		return;
	}

	QFile file(cityDataPath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "ERROR: Could not open city data file for location map: " << cityDataPath;
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
	
	QList<City>::iterator city;
	for(city = cities.begin(); city < cities.end(); ++city)
	{
		scene.addItem(new CityItem(&*city, this));
	}
}

void MapView::select(const City* city)
{
	select(city->getLongitude(), city->getLatitude());
	QLabel* label = parent()->findChild<QLabel*>("selectedLabel");
	label->setText(city->getName());
	update();
}

void MapView::select(float longitude, float latitude)
{
	// Set the longitude
	LongitudeSpinBox* longitudeSpinBox = parent()->findChild<LongitudeSpinBox*>("longitudeSpinBox");
	assert(longitudeSpinBox);
	longitudeSpinBox->setValue(longitude);
	// Set the latitude
	LatitudeSpinBox* latitudeSpinBox = parent()->findChild<LatitudeSpinBox*>("latitudeSpinBox");
	assert(latitudeSpinBox);
	latitudeSpinBox->setValue(latitude);
	// Set the pointeur to the position
	pointeurPos = QPointF(longitude, -latitude);
	update();
}

void MapView::drawItems(QPainter* painter, int numItems, QGraphicsItem* items[], const QStyleOptionGraphicsItem options[])
{
	// First we paint as usual
	QGraphicsView::drawItems(painter, numItems, items, options);
	// Then we paint the pointeur, without doing any zoom
	QPointF pos = mapFromScene(pointeurPos);
	painter->save();
	painter->resetMatrix();
	painter->drawPixmap(pos - QPointF(pointeurPix.width() / 2, pointeurPix.height() / 2), pointeurPix);
	painter->restore();
}

void MapView::mousePressEvent(QMouseEvent * event)
{
	if(event->modifiers() & Qt::ShiftModifier)
	{
		setDragMode(ScrollHandDrag);
		setInteractive(false);
	}
	else if (selectionMode == SELECT_POSITIONS)
	{
		QPointF pos = mapToScene(event->pos());
		select(pos.x(), -pos.y());
		return;
	}
	QGraphicsView::mousePressEvent(event);
}

void MapView::mouseReleaseEvent(QMouseEvent * event)
{
	QGraphicsView::mouseReleaseEvent(event);
	setDragMode(NoDrag);
	setInteractive(true);
}


void MapView::setSelectionMode(int mode)
{
	selectionMode = (SelectionMode)mode;
}




