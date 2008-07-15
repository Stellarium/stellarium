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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "Projector.hpp"
#include "Observer.hpp"
#include "Navigator.hpp"
#include "StelGuiItems.hpp"
#include "StelLocaleMgr.hpp"

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <QRectF>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QTimeLine>
#include <QMouseEvent>
#include <QAction>
#include <QRegExp>
#include <QPixmapCache>
#include <QProgressBar>
#include <QGraphicsWidget>
#include <QGraphicsProxyWidget>


StelButton::StelButton(QGraphicsItem* parent, const QPixmap& apixOn, const QPixmap& apixOff,
		const QPixmap& apixHover, QAction* aaction, QGraphicsSimpleTextItem* ahelpLabel, bool noBackground) : 
		QGraphicsPixmapItem(apixOff, parent), pixOn(apixOn), pixOff(apixOff), pixHover(apixHover),
		checked(false), action(aaction), helpLabel(ahelpLabel), noBckground(noBackground)
{
	assert(!pixOn.isNull());
	assert(!pixOff.isNull());
	setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
	setAcceptsHoverEvents(true);
	timeLine = new QTimeLine(250, this);
	timeLine->setCurveShape(QTimeLine::EaseOutCurve);
	connect(timeLine, SIGNAL(valueChanged(qreal)), this, SLOT(animValueChanged(qreal)));
	
	if (action!=NULL)
	{
		QObject::connect(action, SIGNAL(toggled(bool)), this, SLOT(setChecked(bool)));
		if (action->isCheckable())
		{
			setChecked(action->isChecked());
			QObject::connect(this, SIGNAL(toggled(bool)), action, SLOT(setChecked(bool)));
		}
		else
		{
			QObject::connect(this, SIGNAL(triggered()), action, SLOT(trigger()));
		}
	}
}

void StelButton::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	QGraphicsItem::mousePressEvent(event);
	event->accept();
	setChecked(!checked);
	emit(toggled(checked));
	emit(triggered());
}

void StelButton::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	timeLine->setDirection(QTimeLine::Forward);
	if (timeLine->state()!=QTimeLine::Running)
		timeLine->start();
	if (helpLabel && action)
	{
		QString tip(action->toolTip());
		QString shortcut(action->shortcut().toString());
		if (!shortcut.isEmpty())
		{
			if (shortcut == "Space")
				shortcut = q_("Space");
			tip += "  [" + shortcut + "]";
		}
		helpLabel->setText(tip);
	}
}
		
void StelButton::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	timeLine->setDirection(QTimeLine::Backward);
	if (timeLine->state()!=QTimeLine::Running)
		timeLine->start();
	if (helpLabel && action)
		helpLabel->setText("");
}

void StelButton::animValueChanged(qreal value)
{
	QPixmap pix;
	if (pixBackground.isNull() || noBckground)
	{
		pix = QPixmap(pixOn.size());
		pix.fill(QColor(0,0,0,0));
	}
	else
		pix = pixBackground;
	QPainter painter(&pix);
	painter.drawPixmap(0,0, checked ? pixOn : pixOff);
	if (value>0)
	{
		painter.setOpacity(value);
		painter.drawPixmap(0,0, pixHover);
	}
	setPixmap(pix);
}

void StelButton::setChecked(bool b)
{
	checked=b;
	animValueChanged(timeLine->currentValue());
}

LeftStelBar::LeftStelBar(QGraphicsItem* parent) : QGraphicsItem(parent)
{
}

LeftStelBar::~LeftStelBar()
{
}

void LeftStelBar::addButton(StelButton* button)
{
	double posY = 0;
	if (children().size()!=0)
	{
		const QRectF& r = childrenBoundingRect();
		posY += r.bottom()-1;
	}
	button->setParentItem(this);
	button->setPos(0.5, posY+10.5);
}

void LeftStelBar::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
}

QRectF LeftStelBar::boundingRect() const
{
	return childrenBoundingRect();
}

BottomStelBar::BottomStelBar(QGraphicsItem* parent, const QPixmap& pixLeft, const QPixmap& pixRight, 
		const QPixmap& pixMiddle, const QPixmap& pixSingle) : QGraphicsItem(parent), pixBackgroundLeft(pixLeft), pixBackgroundRight(pixRight),
		pixBackgroundMiddle(pixMiddle), pixBackgroundSingle(pixSingle)
{
	QFont font("DejaVuSans", 10);
	
	// The text is dummy just for testing
	datetime = new QGraphicsSimpleTextItem("2008-02-06  17:33", this);
	location = new QGraphicsSimpleTextItem("Munich, Earth, 500m", this);
	fov = new QGraphicsSimpleTextItem("FOV 43.45", this);
	fps = new QGraphicsSimpleTextItem("43.2 FPS", this);
	
	QColor color = QColor::fromRgbF(1,1,1,1);
	setColor(color);
	
	datetime->setFont(font);
	location->setFont(font);
	fov->setFont(font);
	fps->setFont(font);
	
	flagShowTime = true;
	flagShowLocation = true;
}

void BottomStelBar::addButton(StelButton* button, const QString& groupName, const QString& beforeActionName)
{
	QList<StelButton*>& g = buttonGroups[groupName].elems;
	bool done=false;
	for (int i=0;i<g.size();++i)
	{
		if (g[i]->action && g[i]->action->objectName()==beforeActionName)
		{
			g.insert(i, button);
			done=true;
			break;
		}
	}
	if (done==false)
		g.append(button);
	
	updateButtonsGroups();
	button->setParentItem(this);
}

StelButton* BottomStelBar::hideButton(const QString& actionName)
{
	QString gName;
	StelButton* bToRemove=NULL;
	for (QMap<QString, ButtonGroup>::iterator iter=buttonGroups.begin();iter!=buttonGroups.end();++iter)
	{
		int i=0;
		foreach (StelButton* b, iter.value().elems)
		{
			if (b->action && b->action->objectName()==actionName)
			{
				gName=iter.key();
				bToRemove = b;
				iter.value().elems.removeAt(i);
				break;
			}
			++i;
		}
	}
	if (bToRemove==NULL)
		return NULL;
	if (buttonGroups[gName].elems.size()==0)
	{
		buttonGroups.remove(gName);
	}
	// Cannot really delete because some part of the GUI depend on the presence of some buttons
	// so just make invisible
	bToRemove->setParent(NULL);
	bToRemove->setVisible(false);
	updateButtonsGroups();
	return bToRemove;
}

// Set the margin at the left and right of a button group in pixels
void BottomStelBar::setGroupMargin(const QString& groupName, int left, int right)
{
	if (!buttonGroups.contains(groupName))
		return;
	buttonGroups[groupName].leftMargin=left;
	buttonGroups[groupName].rightMargin=right;
	updateButtonsGroups();
}
	
QRectF BottomStelBar::getButtonsBoundingRect()
{
	location->setParentItem(NULL);
	datetime->setParentItem(NULL);
	fov->setParentItem(NULL);
	fps->setParentItem(NULL);
	
	QRectF rectCh = boundingRect();
	
	location->setParentItem(this);
	datetime->setParentItem(this);
	fov->setParentItem(this);
	fps->setParentItem(this);
	
	return rectCh;
}

void BottomStelBar::updateButtonsGroups()
{
	double x=0;
	double y = datetime->boundingRect().height()+3;
	for (QMap<QString, ButtonGroup >::iterator iter=buttonGroups.begin();iter!=buttonGroups.end();++iter)
	{
		if (iter.value().elems.empty())
			continue;
		x+=iter.value().leftMargin;
		int n=0;
		foreach (StelButton* b, iter.value().elems)
		{
			if (n==0)
			{
				if (iter.value().elems.size()==1)
					b->pixBackground = pixBackgroundSingle;
				else
					b->pixBackground = pixBackgroundLeft;
			}
			else if (n==iter.value().elems.size()-1)
			{
				if (iter.value().elems.size()!=1)
					b->pixBackground = pixBackgroundSingle;
				b->pixBackground = pixBackgroundRight;
			}
			else
			{
				b->pixBackground = pixBackgroundMiddle;
			}
			// Update the button pixmap
			b->animValueChanged(0.);
			b->setPos(x, y);
			x+=b->pixOn.width();
			++n;
		}
		x+=iter.value().rightMargin;
	}
	updateText();
}

void BottomStelBar::updateText()
{
	StelCore* core = StelApp::getInstance().getCore();
	double jd = core->getNavigation()->getJDay();
	
	datetime->setText(flagShowTime ? StelApp::getInstance().getLocaleMgr().getPrintableDateLocal(jd) +"   "
	                  +StelApp::getInstance().getLocaleMgr().getPrintableTimeLocal(jd) : "");
	
	location->setText(flagShowLocation ? core->getObservatory()->getHomePlanetNameI18n() +", "
	                  +core->getObservatory()->getLocationName() + ", "
			  // xgettext:no-c-format
	                  +q_("%1m").arg(core->getObservatory()->getAltitude()) : "");
	
	QString str;
	QTextStream wos(&str);
	wos << "FOV " << qSetRealNumberPrecision(3) << core->getProjection()->getFov() << QChar(0x00B0);
	fov->setText(str);
	
	str="";
	QTextStream wos2(&str);
	wos2 << qSetRealNumberPrecision(3) << StelApp::getInstance().getFps() << " FPS";
	fps->setText(str);
	
	QRectF rectCh = getButtonsBoundingRect();
	location->setPos(0, 0);
	datetime->setPos(rectCh.right()-datetime->boundingRect().width()-5,0);
	
	fov->setPos(datetime->x()-230, 0);
	fps->setPos(datetime->x()-140, 0);
}

void BottomStelBar::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	updateText();
}

QRectF BottomStelBar::boundingRect() const
{
	if (children().size()==0)
		return QRectF();
	const QRectF& r = childrenBoundingRect();
	return QRectF(0, 0, r.width()-1, r.height()-1);
}

// Set the pen for all the sub elements
void BottomStelBar::setColor(const QColor& c)
{
	datetime->setBrush(c);
	location->setBrush(c);
	fov->setBrush(c);
	fps->setBrush(c);
}

StelBarsPath::StelBarsPath(QGraphicsItem* parent) : QGraphicsPathItem(parent)
{
	roundSize = 6;
	QPen aPen(QColor::fromRgbF(0.7,0.7,0.7,0.5));
	aPen.setWidthF(1.);
	setBrush(QBrush(QColor::fromRgbF(0.1, 0.13, 0.23, 0.2)));
	setPen(aPen);
}

void StelBarsPath::updatePath(BottomStelBar* bot, LeftStelBar* lef)
{
	QPainterPath newPath;
	QPointF p = lef->pos();
	QRectF r = lef->boundingRect();
	QPointF p2 = bot->pos();
	QRectF r2 = bot->boundingRect();
	
	newPath.moveTo(p.x()-roundSize, p.y()-roundSize);
	newPath.lineTo(p.x()+r.width(),p.y()-roundSize);
	newPath.arcTo(p.x()+r.width()-roundSize, p.y()-roundSize, 2.*roundSize, 2.*roundSize, 90, -90);
	newPath.lineTo(p.x()+r.width()+roundSize, p2.y()-roundSize);
	newPath.lineTo(p2.x()+r2.width(),p2.y()-roundSize);
	newPath.arcTo(p2.x()+r2.width()-roundSize, p2.y()-roundSize, 2.*roundSize, 2.*roundSize, 90, -90);
	newPath.lineTo(p2.x()+r2.width()+roundSize, p2.y()+r2.height()+roundSize);
	newPath.lineTo(p.x()-roundSize, p2.y()+r2.height()+roundSize);
	setPath(newPath);
}


StelProgressBarMgr::StelProgressBarMgr(QGraphicsItem* parent)
{
}
		
void StelProgressBarMgr::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	// Do nothing. Just paint the child widgets
}

QRectF StelProgressBarMgr::boundingRect() const
{
	if (QGraphicsItem::children().size()==0)
		return QRectF();
	const QRectF& r = childrenBoundingRect();
	return QRectF(0, 0, r.width()-1, r.height()-1);
}

QProgressBar* StelProgressBarMgr::addProgressBar()
{
	QProgressBar* pb = new QProgressBar();
	pb->setFixedHeight(15);
	pb->setFixedWidth(250);
	pb->setTextVisible(true);
	pb->setValue(66);
	QGraphicsProxyWidget* pbProxy = new QGraphicsProxyWidget();
	pbProxy->setWidget(pb);
	pbProxy->setParentItem(this);
	pbProxy->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	pbProxy->setZValue(150);
	updateBarsPositions();
	connect(pbProxy, SIGNAL(destroyed(QObject*)), this, SLOT(oneDestroyed(QObject*)));
	return pb;
}

void StelProgressBarMgr::updateBarsPositions()
{
	int y=0;
	foreach(QGraphicsItem* item, childItems())
	{
		item->setPos(0, y);
		y+=18;
	}
}

void StelProgressBarMgr::oneDestroyed(QObject* obj)
{
	updateBarsPositions();
}
