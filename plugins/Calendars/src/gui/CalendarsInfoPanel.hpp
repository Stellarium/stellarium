/*
Calendars plug-in for Stellarium: graphical user interface widget
Copyright (C) 2020 Georg Zotti

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#ifndef CALENDARSINFOPANEL_HPP
#define CALENDARSINFOPANEL_HPP

#include <QGraphicsWidget>
//#include "Calendars.hpp"

class Calendars;

//! A screen widget similar to InfoPanel. Contains output for selected calendars.
//! @ingroup calendars
class CalendarsInfoPanel : public QGraphicsTextItem
{
	Q_OBJECT

public:
	CalendarsInfoPanel(Calendars* plugin,
			QGraphicsWidget * parent = Q_NULLPTR);
	~CalendarsInfoPanel(){}

public slots:
	//! Update the position of the widget within the parent.
	//! Tied to the parent's geometryChanged() signal.
	void updatePosition();

private slots:
	//! Sets the color scheme (day/night mode)
	void setColorScheme(const QString& schemeName);

private:
	Calendars* plugin;

	// This is actually SkyGui. Perhaps it should be more specific?
	QGraphicsWidget* parentWidget;

	qreal xPos; // cache horizontal screen position to avoid position jitter.
	qreal yPos; // cache vertical   screen position to avoid position jitter.
};

#endif // CALENDARSINFOPANEL_HPP
