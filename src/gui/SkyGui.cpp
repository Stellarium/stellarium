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
#include "StelMainView.hpp"
#include <QGraphicsView>
#include <QDebug>
#include <QTimeLine>
#include <QGraphicsSceneMouseEvent>
#include <QSettings>
#include <QTextDocument>

InfoPanel::InfoPanel(QGraphicsItem* parent) : QGraphicsTextItem("", parent),
	infoPixmap(Q_NULLPTR)
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
		infoTextFilters = StelObject::InfoStringGroup(StelObject::None);
	}
	else if (objectInfo == "custom")
	{
		infoTextFilters = StelObject::InfoStringGroup(StelObject::None);
		
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
		if (conf->value("flag_show_elongation", false).toBool())
			infoTextFilters |= StelObject::Elongation;
		if (conf->value("flag_show_distance", false).toBool())
			infoTextFilters |= StelObject::Distance;
		if (conf->value("flag_show_velocity", false).toBool())
			infoTextFilters |= StelObject::Velocity;
		if (conf->value("flag_show_propermotion", false).toBool())
			infoTextFilters |= StelObject::ProperMotion;
		if (conf->value("flag_show_size", false).toBool())
			infoTextFilters |= StelObject::Size;
		if (conf->value("flag_show_extra", false).toBool())
			infoTextFilters |= StelObject::Extra;
		if (conf->value("flag_show_type", false).toBool())
			infoTextFilters |= StelObject::ObjectType;
		if (conf->value("flag_show_galcoord", false).toBool())
			infoTextFilters |= StelObject::GalacticCoord;
		if (conf->value("flag_show_supergalcoord", false).toBool())
			infoTextFilters |= StelObject::SupergalacticCoord;
		if (conf->value("flag_show_othercoord", false).toBool())
			infoTextFilters |= StelObject::OtherCoord;
		if (conf->value("flag_show_eclcoordofdate", false).toBool())
			infoTextFilters |= StelObject::EclipticCoordOfDate;
		if (conf->value("flag_show_eclcoordj2000", false).toBool())
			infoTextFilters |= StelObject::EclipticCoordJ2000;
		if (conf->value("flag_show_constellation", false).toBool())
			infoTextFilters |= StelObject::IAUConstellation;
		if (conf->value("flag_show_sidereal_time", false).toBool())
			infoTextFilters |= StelObject::SiderealTime;
		if (conf->value("flag_show_rts_time", false).toBool())
			infoTextFilters |= StelObject::RTSTime;
		conf->endGroup();
	}
	else
	{
		qWarning() << "config.ini option gui/selected_object_info is invalid, using \"all\"";
		infoTextFilters = StelObject::InfoStringGroup(StelObject::AllInfo);
	}
	if (qApp->property("text_texture")==true) // CLI option -t given?
		infoPixmap=new QGraphicsPixmapItem(this);
}

InfoPanel::~InfoPanel()
{
	if (infoPixmap)
	{
		delete infoPixmap;
		infoPixmap=Q_NULLPTR;
	}
}

// A hackish fix for broken OpenGL font situations like RasPi2 VC4 as of 2016-03-26.
// strList is the text-only representation of InfoPanel.toPlainText(), pre-split into a stringlist.
// It is assumed: The h2 element (1-2 lines) has been broken into 1-2 lines and a line "ENDHEAD", rest follows line-by-line.
// The header lines are shown in bold large font, the rest in normal size.
// There is no bold or other font mark-up, but that should be acceptable.
QPixmap getInfoPixmap(const QStringList& strList, QFont font, QColor color)
{
	// Render the text str into a QPixmap.
	// search longest string.
	int maxLenIdx=0; int maxLen=0;
	for (int i = 0; i < strList.size(); ++i)
	{
		if (strList.at(i).length() > maxLen)
		{
			maxLen=strList.at(i).length();
			maxLenIdx=i;
		}
	}
	QFont titleFont(font);
	titleFont.setBold(true);
	titleFont.setPixelSize(font.pixelSize()+7);

	QRect strRect = QFontMetrics(titleFont).boundingRect(strList.at(maxLenIdx));
	int w = strRect.width()+1+static_cast<int>(0.02f*strRect.width());
	int h = strRect.height()*strList.count()+8;

	QPixmap strPixmap(w, h);
	strPixmap.fill(Qt::transparent);
	QPainter painter(&strPixmap);
	font.setStyleStrategy(QFont::NoAntialias); // else: font problems on RasPi20160326
	//painter.setRenderHints(QPainter::TextAntialiasing);
	painter.setPen(color);
	painter.setFont(titleFont);
	int txtOffset=0; // to separate heading from rest of text.
	for (int i = 0; i < strList.size(); ++i)
	{
		if (strList.at(i).startsWith( "ENDHEAD"))
		{
			painter.setFont(font);
			txtOffset=8;
		}
		else
			painter.drawText(-strRect.x()+1, -strRect.y()+i*(painter.font().pixelSize()+2)+txtOffset, strList.at(i));
	}
	return strPixmap;
}

void InfoPanel::setTextFromObjects(const QList<StelObjectP>& selected)
{
	if (selected.isEmpty())
	{
		if (!document()->isEmpty())
			document()->clear();
		if (qApp->property("text_texture")==true) // CLI option -t given?
			infoPixmap->setVisible(false);
	}
	else
	{
		// just print details of the first item for now
		// Must set lastRTS for currently selected object here...
		StelCore *core=StelApp::getInstance().getCore();
		QString s = selected[0]->getInfoString(core, infoTextFilters);
		selected[0]->removeExtraInfoStrings(StelObject::AllInfo);
		QFont font;
		font.setPixelSize(StelApp::getInstance().getScreenFontSize());
		setFont(font);
		setHtml(s);
		if (qApp->property("text_texture")==true) // CLI option -t given?
		{
			// Extract color from HTML.
			QRegExp colorRegExp("<font color=(#[0-9a-f]{6,6})>");
			int colorInt=colorRegExp.indexIn(s);
			QString colorStr;

			if (colorInt>-1)
				colorStr=colorRegExp.cap(1);
			else
				colorStr="#ffffff";

			QColor infoColor(colorStr);
			// inject a marker word in the infostring to mark end of header.
			// In case no header exists, put it after the color tag (first closing brace).
			int endHead=s.indexOf("</h2>")+5;
			if (endHead==4)
				endHead=s.indexOf(">")+1;
			s.insert(endHead, QString("ENDHEAD<br/>"));
			setHtml(s);
			infoPixmap->setPixmap(getInfoPixmap(getSelectedText().split("\n"), this->font(), infoColor));
			// setting visible=false would hide also the child QGraphicsPixmapItem...
			setHtml("");
			infoPixmap->setVisible(true);
		}
	}
}

const QString InfoPanel::getSelectedText(void) const
{
	return toPlainText();
}

SkyGui::SkyGui(QGraphicsItem * parent)
	: QGraphicsWidget(parent)
	, lastButtonbarWidth(0)
	, btHorizAutoHide(Q_NULLPTR)
	, btVertAutoHide(Q_NULLPTR)
	, autoHidebts(Q_NULLPTR)
	, autoHideHorizontalButtonBar(true)
	, autoHideVerticalButtonBar(true)
	, stelGui(Q_NULLPTR)
{
	setObjectName("StelSkyGui");

	// Construct the Windows buttons bar
	winBar = new LeftStelBar(this);
	// Construct the bottom buttons bar
	buttonBar = new BottomStelBar(this,
				      QPixmap(":/graphicGui/btbgLeft.png"),
				      QPixmap(":/graphicGui/btbgRight.png"),
				      QPixmap(":/graphicGui/btbgMiddle.png"),
				      QPixmap(":/graphicGui/btbgSingle.png"));
	infoPanel = new InfoPanel(this);

	// Used to display some progress bar in the lower right corner, e.g. when loading a file
	progressBarMgr = new StelProgressBarMgr(this);
	connect(&StelApp::getInstance(), SIGNAL(progressBarAdded(const StelProgressController*)), progressBarMgr, SLOT(addProgressBar(const StelProgressController*)));
	connect(&StelApp::getInstance(), SIGNAL(progressBarRemoved(const StelProgressController*)), progressBarMgr, SLOT(removeProgressBar(const StelProgressController*)));

	// The path drawn around the button bars
	buttonBarPath = new StelBarsPath(this);

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

	// Create the 2 auto hide buttons in the bottom left corner
	autoHidebts = new CornerButtons(this);
	QPixmap pxmapOn = QPixmap(":/graphicGui/miscHorAutoHide-on.png");
	QPixmap pxmapOff = QPixmap(":/graphicGui/miscHorAutoHide-off.png");
	btHorizAutoHide = new StelButton(autoHidebts, pxmapOn, pxmapOff, QPixmap(), "actionAutoHideHorizontalButtonBar", true);
	pxmapOn = QPixmap(":/graphicGui/miscVertAutoHide-on.png");
	pxmapOff = QPixmap(":/graphicGui/miscVertAutoHide-off.png");
	btVertAutoHide = new StelButton(autoHidebts, pxmapOn, pxmapOff, QPixmap(), "actionAutoHideVerticalButtonBar", true);

	btHorizAutoHide->setPos(1,btVertAutoHide->pixmap().height()-btHorizAutoHide->pixmap().height()+1);
	btVertAutoHide->setPos(0,0);
	btVertAutoHide->setZValue(1000);

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
	connect(buttonBar, SIGNAL(sizeChanged()), this, SLOT(updateBarsPos()));
}

void SkyGui::resizeEvent(QGraphicsSceneResizeEvent* event)
{
	QGraphicsWidget::resizeEvent(event);
	updateBarsPos();
}

void SkyGui::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
	const int hh = getSkyGuiHeight();

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

// Used to recompute the bars position when we toggle the gui off and on.
// This was not necessary with Qt < 5.4.  So it might be a bug.
QVariant SkyGui::itemChange(GraphicsItemChange change, const QVariant & value)
{
	if (change == QGraphicsItem::ItemVisibleHasChanged && value.toBool())
		updateBarsPos();
	return QGraphicsItem::itemChange(change, value);
}

int SkyGui::getSkyGuiWidth() const
{
	return static_cast<int>(geometry().width());
}

int SkyGui::getSkyGuiHeight() const
{
	return static_cast<int>(geometry().height());
}

qreal SkyGui::getBottomBarHeight() const
{
	return buttonBar->boundingRect().height();
}

qreal SkyGui::getLeftBarWidth() const
{
	return winBar->boundingRect().width();
}

//! Update the position of the button bars in the main window
void SkyGui::updateBarsPos()
{
	const int ww = getSkyGuiWidth();
	const int hh = getSkyGuiHeight();
	bool updatePath = false;

	// Use a position cache to avoid useless redraw triggered by the position set if the bars don't move
	double rangeX = winBar->boundingRectNoHelpLabel().width()+2.*buttonBarPath->getRoundSize()+1.;
	const qreal newWinBarX = buttonBarPath->getRoundSize()-(1.-animLeftBarTimeLine->currentValue())*rangeX-0.5;
	const qreal newWinBarY = hh-winBar->boundingRectNoHelpLabel().height()-buttonBar->boundingRectNoHelpLabel().height()-20;
	if (winBar->pos().x()!=newWinBarX || winBar->pos().y()!=newWinBarY)
	{
		winBar->setPos(qRound(newWinBarX), qRound(newWinBarY));
		updatePath = true;
	}

	double rangeY = buttonBar->boundingRectNoHelpLabel().height()+0.5-7.-buttonBarPath->getRoundSize();
	const qreal newButtonBarX = winBar->boundingRectNoHelpLabel().right()+buttonBarPath->getRoundSize();
	const qreal newButtonBarY = hh-buttonBar->boundingRectNoHelpLabel().height()-buttonBarPath->getRoundSize()+0.5+(1.-animBottomBarTimeLine->currentValue())*rangeY;
	if (buttonBar->pos().x()!=newButtonBarX || buttonBar->pos().y()!=newButtonBarY)
	{
		buttonBar->setPos(qRound(newButtonBarX), qRound(newButtonBarY));
		updatePath = true;
	}

	if (lastButtonbarWidth != buttonBar->boundingRectNoHelpLabel().width())
	{
		updatePath = true;
		lastButtonbarWidth = static_cast<int>(buttonBar->boundingRectNoHelpLabel().width());
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

	// Update the screen as soon as possible.
	StelMainView::getInstance().thereWasAnEvent();
}

void SkyGui::setStelStyle(const QString& style)
{
	Q_UNUSED(style);
	buttonBarPath->setPen(QColor::fromRgbF(0.7,0.7,0.7,0.5));
	buttonBarPath->setBrush(QColor::fromRgbF(0.15, 0.16, 0.19, 0.2));
	buttonBar->setColor(QColor::fromRgbF(0.9, 0.91, 0.95, 0.9));
	winBar->setColor(QColor::fromRgbF(0.9, 0.91, 0.95, 0.9));
}

// Add a new progress bar in the lower right corner of the screen.
void SkyGui::addProgressBar(StelProgressController* p)
{
	return progressBarMgr->addProgressBar(p);
}

void SkyGui::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
{
	stelGui->update();
}
