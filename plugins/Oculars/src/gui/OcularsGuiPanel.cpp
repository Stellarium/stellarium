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

#include "Oculars.hpp"
#include "OcularsGuiPanel.hpp"

#include <QVBoxLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QPushButton>
#include <QWidget>

OcularsGuiPanel::OcularsGuiPanel(Oculars* plugin,
                                 QGraphicsWidget *parent,
                                 Qt::WindowFlags wFlags):
	QGraphicsWidget(parent, wFlags),
	ocularsPlugin(plugin),
	parentWidget(parent),
	proxy(0),
	mainWidget(0)
{
	QGraphicsLinearLayout* layout = new QGraphicsLinearLayout(Qt::Vertical, this);

	mainWidget = new QWidget();
	mainWidget->setBaseSize(100,100);

	QVBoxLayout* verticalLayout = new QVBoxLayout();
	verticalLayout->setMargin(0);
	mainWidget->setLayout(verticalLayout);
	//TODO
	verticalLayout->addWidget(new QPushButton("Test"));

	proxy = new QGraphicsProxyWidget();
	proxy->setWidget(mainWidget);
	proxy->setZValue(500.);

	layout->addItem(proxy);

	updatePosition();
	connect (parentWidget, SIGNAL(geometryChanged()),
	         this, SLOT(updatePosition()));
}

OcularsGuiPanel::~OcularsGuiPanel()
{
	if (mainWidget)
		delete mainWidget;
	//Does this take care of the proxy, too?
}

void OcularsGuiPanel::updatePosition()
{
	updateGeometry();

	qreal xPos = parentWidget->size().width() - size().width();
	qreal yPos = 0;
	setPos(xPos, yPos);
}
