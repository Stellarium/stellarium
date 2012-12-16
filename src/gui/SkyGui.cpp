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

#include "SkyGui.hpp"
#include "StelObjectMgr.hpp"
#include "StelGuiItems.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelCore.hpp"
#include <QGraphicsView>
#include <QDebug>
#include <QTimeLine>
#include <QGraphicsSceneMouseEvent>
#include <QSettings>
#include <QTextDocument>

InfoPanel::InfoPanel(QGraphicsItem* parent) : QGraphicsTextItem("", parent)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	QString objectInfo = conf->value("gui/selected_object_info", "all").toString();
	if (objectInfo == "all")
	{
		infoTextFilters = StelObject::InfoStringGroup(StelObject::AllInfo);
	}
	else if (objectInfo == "short")
	{
		infoTextFilters = StelObject::InfoStringGroup(StelObject::ShortInfo);
	}
	else if (objectInfo == "none")
	{
		infoTextFilters = StelObject::InfoStringGroup(0);
	}
	else if (objectInfo == "custom")
	{
		infoTextFilters = StelObject::InfoStringGroup(0);
		
		conf->beginGroup("custom_selected_info");
		if (conf->value("flag_show_name", false).toBool())
			infoTextFilters |= StelObject::Name;
		if (conf->value("flag_show_catalognumber", false).toBool())
			infoTextFilters |= StelObject::CatalogNumber;
		if (conf->value("flag_show_magnitude", false).toBool())
			infoTextFilters |= StelObject::Magnitude;
		if (conf->value("flag_show_absolutemagnitude", false).toBool())
			infoTextFilters |= StelObject::AbsoluteMagnitude;
		if (conf->value("flag_show_radecj2000", false).toBool())
			infoTextFilters |= StelObject::RaDecJ2000;
		if (conf->value("flag_show_radecofdate", false).toBool())
			infoTextFilters |= StelObject::RaDecOfDate;
		if (conf->value("flag_show_hourangle", false).toBool())
			infoTextFilters |= StelObject::HourAngle;
		if (conf->value("flag_show_altaz", false).toBool())
			infoTextFilters |= StelObject::AltAzi;
		if (conf->value("flag_show_distance", false).toBool())
			infoTextFilters |= StelObject::Distance;
		if (conf->value("flag_show_size", false).toBool())
			infoTextFilters |= StelObject::Size;
		if (conf->value("flag_show_extra1", false).toBool())
			infoTextFilters |= StelObject::Extra1;
		if (conf->value("flag_show_extra2", false).toBool())
			infoTextFilters |= StelObject::Extra2;
		if (conf->value("flag_show_extra3", false).toBool())
			infoTextFilters |= StelObject::Extra3;
		conf->endGroup();
	}
	else
	{
		qWarning() << "config.ini option gui/selected_object_info is invalid, using \"all\"";
		infoTextFilters = StelObject::InfoStringGroup(StelObject::AllInfo);
	}

#ifdef ANDROID
	setCacheMode(QGraphicsItem::DeviceCoordinateCache);
#endif
}

void InfoPanel::setTextFromObjects(const QList<StelObjectP>& selected)
{
	if (selected.isEmpty())
	{
		if (!document()->isEmpty())
			document()->clear();
	}
	else
	{
		// just print details of the first item for now
		QString s = selected[0]->getInfoString(StelApp::getInstance().getCore(), infoTextFilters);
		setHtml(s);
	}
}

const QString InfoPanel::getSelectedText(void)
{
	return toPlainText();
}

SkyGui::SkyGui(QGraphicsItem * parent): QGraphicsWidget(parent), stelGui(NULL)
{
	setObjectName("StelSkyGui");

	// Construct the Windows buttons bar
	winBar = new LeftStelBar(this);
	// Construct the bottom buttons bar
	buttonBar = new BottomStelBar(this, QPixmap(":/graphicGui/btbg-left.png"), QPixmap(":/graphicGui/btbg-right.png"),
																QPixmap(":/graphicGui/btbg-middle.png"), QPixmap(":/graphicGui/btbg-single.png"));
	infoPanel = new InfoPanel(this);

	// Used to display some progress bar in the lower right corner, e.g. when loading a file
	progressBarMgr = new StelProgressBarMgr(this);

	// The path drawn around the button bars
	buttonBarPath = new StelBarsPath(this);

	lastButtonbarWidth = 0;
	autoHidebts = NULL;

	autoHideHorizontalButtonBar = true;
	autoHideVerticalButtonBar = true;

	animLeftBarTimeLine = new QTimeLine(200, this);
	animLeftBarTimeLine->setCurveShape(QTimeLine::EaseInOutCurve);
	connect(animLeftBarTimeLine, SIGNAL(valueChanged(qreal)), this, SLOT(updateBarsPos()));

	animBottomBarTimeLine = new QTimeLine(200, this);
	animBottomBarTimeLine->setCurveShape(QTimeLine::EaseInOutCurve);
	connect(animBottomBarTimeLine, SIGNAL(valueChanged(qreal)), this, SLOT(updateBarsPos()));

	setAcceptHoverEvents(true);
}

void SkyGui::init(StelGui* astelGui)
{
	stelGui = astelGui;

	winBar->setParentItem(this);
	buttonBar->setParentItem(this);
	buttonBarPath->setParentItem(this);
	infoPanel->setParentItem(this);
	progressBarMgr->setParentItem(this);

	// Create the 2 auto hide buttons in the bottom left corner
	autoHidebts = new CornerButtons();
	QPixmap pxmapOn = QPixmap(":/graphicGui/HorizontalAutoHideOn.png");
	QPixmap pxmapOff = QPixmap(":/graphicGui/HorizontalAutoHideOff.png");
	btHorizAutoHide = new StelButton(autoHidebts, pxmapOn, pxmapOff, QPixmap(), stelGui->getGuiAction("actionAutoHideHorizontalButtonBar"), true);
	btHorizAutoHide->setChecked(autoHideHorizontalButtonBar);
	pxmapOn = QPixmap(":/graphicGui/VerticalAutoHideOn.png");
	pxmapOff = QPixmap(":/graphicGui/VerticalAutoHideOff.png");
	btVertAutoHide = new StelButton(autoHidebts, pxmapOn, pxmapOff, QPixmap(), stelGui->getGuiAction("actionAutoHideVerticalButtonBar"), true);
	btVertAutoHide->setChecked(autoHideVerticalButtonBar);

	btHorizAutoHide->setPos(1,btVertAutoHide->pixmap().height()-btHorizAutoHide->pixmap().height()+1);
	btVertAutoHide->setPos(0,0);
	btVertAutoHide->setZValue(1000);
	autoHidebts->setParentItem(this);

	infoPanel->setPos(8,8);

	// If auto hide is off, show the relevant toolbars
	if (!autoHideHorizontalButtonBar)
	{
		animBottomBarTimeLine->setDirection(QTimeLine::Forward);
		animBottomBarTimeLine->start();
	}

	if (!autoHideVerticalButtonBar)
	{
		animLeftBarTimeLine->setDirection(QTimeLine::Forward);
		animLeftBarTimeLine->start();
	}

	buttonBarPath->setZValue(-0.1);
	updateBarsPos();
	connect(&StelApp::getInstance(), SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
}

void SkyGui::resizeEvent(QGraphicsSceneResizeEvent* event)
{
	QGraphicsWidget::resizeEvent(event);
	updateBarsPos();
}

void SkyGui::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
	const int hh = geometry().height();

	double x = event->pos().x();
	double y = event->pos().y();
	double maxX = winBar->boundingRect().width()+2.*buttonBarPath->getRoundSize();
	double maxY = hh-(winBar->boundingRect().height()+buttonBar->boundingRect().height()+2.*buttonBarPath->getRoundSize());
	double minX = 0;

	if (x<=maxX && y>=maxY && animLeftBarTimeLine->state()==QTimeLine::NotRunning && winBar->pos().x()<minX)
	{
		animLeftBarTimeLine->setDirection(QTimeLine::Forward);
		animLeftBarTimeLine->start();
	}
	if (autoHideVerticalButtonBar && (x>maxX+30 || y<maxY-30) && animLeftBarTimeLine->state()==QTimeLine::NotRunning && winBar->pos().x()>=minX)
	{
		animLeftBarTimeLine->setDirection(QTimeLine::Backward);
		animLeftBarTimeLine->start();
	}

	maxX = winBar->boundingRect().width()+buttonBar->boundingRect().width()+2.*buttonBarPath->getRoundSize();
	maxY = hh-buttonBar->boundingRect().height()+2.*buttonBarPath->getRoundSize();
	if (x<=maxX && y>=maxY && animBottomBarTimeLine->state()==QTimeLine::NotRunning && animBottomBarTimeLine->currentValue()<1.)
	{
		animBottomBarTimeLine->setDirection(QTimeLine::Forward);
		animBottomBarTimeLine->start();
	}
	if (autoHideHorizontalButtonBar && (x>maxX+30 || y<maxY-30) && animBottomBarTimeLine->state()==QTimeLine::NotRunning && animBottomBarTimeLine->currentValue()>=0.9999999)
	{
		animBottomBarTimeLine->setDirection(QTimeLine::Backward);
		animBottomBarTimeLine->start();
	}
}

//! Update the position of the button bars in the main window
void SkyGui::updateBarsPos()
{
	const int ww = geometry().width();
	const int hh = geometry().height();
	bool updatePath = false;

	// Use a position cache to avoid useless redraw triggered by the position set if the bars don't move
	double rangeX = winBar->boundingRectNoHelpLabel().width()+2.*buttonBarPath->getRoundSize()+1.;
	const qreal newWinBarX = buttonBarPath->getRoundSize()-(1.-animLeftBarTimeLine->currentValue())*rangeX-0.5;
	const qreal newWinBarY = hh-winBar->boundingRectNoHelpLabel().height()-buttonBar->boundingRectNoHelpLabel().height()-20;
	if (winBar->pos().x()!=newWinBarX || winBar->pos().y()!=newWinBarY)
	{
		winBar->setPos(round(newWinBarX), round(newWinBarY));
		updatePath = true;
	}

	double rangeY = buttonBar->boundingRectNoHelpLabel().height()+0.5-7.-buttonBarPath->getRoundSize();
	const qreal newButtonBarX = winBar->boundingRectNoHelpLabel().right()+buttonBarPath->getRoundSize();
	const qreal newButtonBarY = hh-buttonBar->boundingRectNoHelpLabel().height()-buttonBarPath->getRoundSize()+0.5+(1.-animBottomBarTimeLine->currentValue())*rangeY;
	if (buttonBar->pos().x()!=newButtonBarX || buttonBar->pos().y()!=newButtonBarY)
	{
		buttonBar->setPos(round(newButtonBarX), round(newButtonBarY));
		updatePath = true;
	}

	if (lastButtonbarWidth != buttonBar->boundingRectNoHelpLabel().width())
	{
		updatePath = true;
		lastButtonbarWidth = (int)(buttonBar->boundingRectNoHelpLabel().width());
	}

	if (updatePath)
		buttonBarPath->updatePath(buttonBar, winBar);

	const qreal newProgressBarX = ww-progressBarMgr->boundingRect().width()-20;
	const qreal newProgressBarY = hh-progressBarMgr->boundingRect().height()+7;
	progressBarMgr->setPos(newProgressBarX, newProgressBarY);
	progressBarMgr->setZValue(400);

	// Update position of the auto-hide buttons
	autoHidebts->setPos(0, hh-autoHidebts->childrenBoundingRect().height()+1);
	double opacity = qMax(animLeftBarTimeLine->currentValue(), animBottomBarTimeLine->currentValue());
	autoHidebts->setOpacity(opacity < 0.01 ? 0.01 : opacity);	// Work around a qt bug
}

void SkyGui::setStelStyle(const QString& style)
{
	if (style == "night_color")
	{
		buttonBarPath->setPen(QColor::fromRgbF(0.7,0.0,0.0,0.5));
		buttonBarPath->setBrush(QColor::fromRgbF(0.23, 0.0, 0.00, 0.2));
		buttonBar->setColor(QColor::fromRgbF(0.9, 0.0, 0.0, 0.9));
		winBar->setColor(QColor::fromRgbF(0.9, 0.0, 0.0, 0.9));
		winBar->setRedMode(true);
		buttonBar->setRedMode(true);
		btHorizAutoHide->setRedMode(true);
		btVertAutoHide->setRedMode(true);
	}
	else
	{
		buttonBarPath->setPen(QColor::fromRgbF(0.7,0.7,0.7,0.5));
		buttonBarPath->setBrush(QColor::fromRgbF(0.15, 0.16, 0.19, 0.2));
		buttonBar->setColor(QColor::fromRgbF(0.9, 0.91, 0.95, 0.9));
		winBar->setColor(QColor::fromRgbF(0.9, 0.91, 0.95, 0.9));
		buttonBar->setRedMode(false);
		winBar->setRedMode(false);
		btHorizAutoHide->setRedMode(false);
		btVertAutoHide->setRedMode(false);
	}
}

// Add a new progress bar in the lower right corner of the screen.
QProgressBar* SkyGui::addProgressBar()
{
	return progressBarMgr->addProgressBar();
}

void SkyGui::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
{
	stelGui->update();
}
