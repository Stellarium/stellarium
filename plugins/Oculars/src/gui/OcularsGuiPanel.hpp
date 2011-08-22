/*
Oculars plug-in for Stellarium: graphical user interface widget
Copyright (C) 2011  Bogdan Marinov

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef OCULARSGUIPANEL_HPP
#define OCULARSGUIPANEL_HPP

#include <QGraphicsWidget>

class Oculars;
class QGraphicsProxyWidget;
class QPushButton;
class QWidget;

//! A screen widget similar to InfoPanel. Contains controls and information.
class OcularsGuiPanel : public QGraphicsWidget
{
	Q_OBJECT

public:
	OcularsGuiPanel(Oculars* ocularsPlugin,
	                QGraphicsWidget * parent = 0,
	                Qt::WindowFlags wFlags = 0);
	~OcularsGuiPanel();

public slots:
	//! Update the position of the widget within the parent.
	//! Tied to the parent's geometryChanged() signal.
	void updatePosition();

private:
	Oculars* ocularsPlugin;

	//! This is actually SkyGui. Perhaps it should be more specific?
	QGraphicsWidget* parentWidget;

	// For now, this uses regular widgets wrapped in a proxy. In the future
	// it may be implemented purely with classes derived from QGraphicsItem.
	QGraphicsProxyWidget* proxy;
	QWidget* mainWidget;

	QPushButton* nextTelescopeButton;
	QPushButton* nextCcdButton;
	QPushButton* nextOcularButton;
	QPushButton* prevTelescopeButton;
	QPushButton* prevCcdButton;
	QPushButton* prevOcularButton;
};

#endif // OCULARSGUIPANEL_HPP
