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


#ifndef MAPLABEL_HPP
#define MAPLABEL_HPP

#include <QLabel>

//! @class MapLabel
//! Special QLabel that shows a world map 
class MapLabel : public QLabel
{
	Q_OBJECT

public:
	MapLabel(QWidget *parent = Q_NULLPTR);
	~MapLabel() Q_DECL_OVERRIDE;
	//! Set the current cursor position
	//! @param longitude longitude in degree in range [-180;180[
	//! @param latitude latitude in degree in range [-90;90]
	void setCursorPos(double longitude, double latitude);

	void setPixmap(const QPixmap &pixmap);
	void resizePixmap();
	
signals:
	//! Signal emitted when we click on the map
	void positionChanged(double longitude, double latitude);

protected:
	void mousePressEvent(QMouseEvent * event) Q_DECL_OVERRIDE;
	void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

private:
	//QLabel* cursor;

	QPixmap map;
	//map without location cursor drawn on
	QPixmap origMap;

	QPixmap locCursor;
};

#endif // _MAPLABEL_HPP
