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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "MapLabel.hpp"
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>

MapLabel::MapLabel(QWidget *parent) : QLabel(parent), locCursor(":/graphicGui/map-pointeur.png")
{
	/*cursor = new QLabel(this);
	cursor->setPixmap(QPixmap(":/graphicGui/map-pointeur.png"));
	cursor->resize(cursor->pixmap()->size());
	cursor->show();*/
}

MapLabel::~MapLabel()
{
}

void MapLabel::setCursorPos(double longitude, double latitude)
{
	//resets the map to the original map
	map = origMap;
	const int x = ((int)((longitude+180.)/360.*map.width()));
	const int y = ((int)((latitude-90.)/-180.*map.height()));
	//draws the location cursor on the map every time position is changed
	QPainter painter(&map);
	painter.drawPixmap(x-locCursor.width()/2,y-locCursor.height()/2,locCursor.width(),locCursor.height(),locCursor);
	resizePixmap();
}

void MapLabel::mousePressEvent(QMouseEvent* event)
{
	const int offsetX = (width() - pixmap()->width())/2;
	const int offsetY = (height() - pixmap()->height())/2;

	const int posX = event->pos().x();
	const int posY = event->pos().y();

	//checks if position of mouse click is inside the map
	if((unsigned)(posX-offsetX) > (unsigned)pixmap()->width() || (unsigned)(posY-offsetY) > (unsigned)pixmap()->height())
	{
		return;
	}
	const double lon = ((double)(posX-offsetX))/pixmap()->size().width()*360.-180.;
	const double lat = 90.-((double)(posY-offsetY))/pixmap()->size().height()*180.;
	emit(positionChanged(lon, lat));
}

void MapLabel::setPixmap(const QPixmap &pixmap)
{
	map = pixmap;
	origMap = pixmap;
	QLabel::setPixmap(map);
}

void MapLabel::resizePixmap()
{
	QLabel::setPixmap(map.scaled(width(),height(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
}

void MapLabel::resizeEvent(QResizeEvent *event)
{
	resizePixmap();
	QLabel::resizeEvent(event);
}
