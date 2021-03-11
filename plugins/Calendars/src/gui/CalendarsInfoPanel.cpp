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
Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include "Calendars.hpp"
#include "CalendarsInfoPanel.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "SkyGui.hpp"
#include "StelGuiItems.hpp"
#include "StelTranslator.hpp"
#include "StelActionMgr.hpp"

#include <float.h>
#include <QGridLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsPathItem>
#include <QGraphicsProxyWidget>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QWidget>

CalendarsInfoPanel::CalendarsInfoPanel(Calendars* plugin,
				 QGraphicsWidget *parent):
	QGraphicsTextItem("", parent),
	plugin(plugin),
	parentWidget(parent),
	xPos(static_cast<qreal>(FLT_MAX)),
	yPos(static_cast<qreal>(FLT_MAX))
{
	StelApp& stelApp = StelApp::getInstance();
	QFont newFont = font();

	updatePosition();
	connect (parentWidget, SIGNAL(geometryChanged()), this, SLOT(updatePosition()));
	//Night mode
	connect(&stelApp, SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setColorScheme(const QString&)));
	setColorScheme(stelApp.getCurrentStelStyle());
}


void CalendarsInfoPanel::updatePosition()
{
	qreal bottomBoundingHeight = static_cast<SkyGui*>(parentWidget)->getBottomBarHeight();

	if (sender())
	{
		xPos=parentWidget->size().width(); // reset when window has been resized.
		yPos=static_cast<qreal>(FLT_MAX);
	}
	qreal xPosCand = parentWidget->size().width() -  boundingRect().width();
	xPos=qMin(xPos, xPosCand);
	qreal yPosCand = parentWidget->size().height() - boundingRect().height() - bottomBoundingHeight;
	yPos=qMin(yPos, yPosCand);
	setPos(xPos, yPos);
}



void CalendarsInfoPanel::setColorScheme(const QString &schemeName)
{
	Q_UNUSED(schemeName);
}
