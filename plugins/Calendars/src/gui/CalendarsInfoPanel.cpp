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
#include "SkyGui.hpp"

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

	updatePosition();
	connect (parentWidget, SIGNAL(geometryChanged()), this, SLOT(updatePosition()));
	// when user switches a calendar on or off, we must force a recalculation of a minimal bounding box before we can rebuild the calendar list.
	connect (this->plugin, &Calendars::showJulianChanged             , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showGregorianChanged          , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showISOChanged                , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showIcelandicChanged          , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showRomanChanged              , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showOlympicChanged            , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showEgyptianChanged           , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showArmenianChanged           , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showZoroastrianChanged        , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showCopticChanged             , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showEthiopicChanged           , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showChineseChanged            , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showIslamicChanged            , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showHebrewChanged             , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showOldHinduSolarChanged      , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showOldHinduLunarChanged      , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showNewHinduSolarChanged      , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showNewHinduLunarChanged      , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showAstroHinduSolarChanged    , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showAstroHinduLunarChanged    , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showTibetanChanged            , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showMayaLongCountChanged      , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showMayaHaabChanged           , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showMayaTzolkinChanged        , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showAztecXihuitlChanged       , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showAztecTonalpohualliChanged , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showBalineseChanged           , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showFrenchAstronomicalChanged , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showFrenchArithmeticChanged   , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showPersianArithmeticChanged  , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showPersianAstronomicalChanged, this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showBahaiArithmeticChanged    , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showBahaiAstronomicalChanged  , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showChineseChanged            , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showJapaneseChanged           , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showKoreanChanged             , this, [=](bool){setHtml("a"); updatePosition();});
	connect (this->plugin, &Calendars::showVietnameseChanged         , this, [=](bool){setHtml("a"); updatePosition();});

	//Night mode
	connect(&stelApp, SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setColorScheme(const QString&)));
	setColorScheme(stelApp.getCurrentStelStyle());
}

void CalendarsInfoPanel::updatePosition(bool resetPos)
{
	qreal bottomBoundingHeight = static_cast<SkyGui*>(parentWidget)->getBottomBarHeight()+5.;

	if (sender() || resetPos)
	{
		xPos=parentWidget->size().width(); // reset when window has been resized.
		yPos=static_cast<qreal>(FLT_MAX);
	}
	qreal xPosCand = parentWidget->size().width() - boundingRect().width();
	xPos=qMin(xPos, xPosCand);
	qreal yPosCand = parentWidget->size().height() - boundingRect().height() - bottomBoundingHeight;
	yPos=qMin(yPos, yPosCand);
	setPos(xPos, yPos);
}

void CalendarsInfoPanel::setColorScheme(const QString &schemeName)
{
	Q_UNUSED(schemeName)
}
