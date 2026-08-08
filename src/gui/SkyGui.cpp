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
#include "StelModuleMgr.hpp"
#include "StelProgressController.hpp"
#include <QGraphicsView>
#include <QDebug>
#include <QTimeLine>
#include <QGraphicsSceneMouseEvent>
#include <QSettings>
#include <QTextDocument>
#include <QRegularExpression>
#include <QGraphicsDropShadowEffect>

InfoPanel::InfoPanel(QGraphicsItem* parent) : QGraphicsTextItem("", parent)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	QString objectInfo = conf->value("gui/selected_object_info", "default").toString();
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
		infoTextFilters = GETSTELMODULE(StelObjectMgr)->getCustomInfoStrings();
	}
	else
	{
		if (objectInfo != "default")
			qWarning() << "config.ini option gui/selected_object_info is invalid, using \"default\"";
		infoTextFilters = StelObject::InfoStringGroup(StelObject::DefaultInfo);
	}

	QFont font=QGuiApplication::font();
	font.setPixelSize(StelApp::getInstance().getScreenFontSize());
	setFont(font);
	connect(&StelApp::getInstance(), &StelApp::fontChanged, this, &QGraphicsTextItem::setFont);
	connect(&StelApp::getInstance(), &StelApp::screenFontSizeChanged, this, [=](int size){
		QFont font=QGuiApplication::font();
		font.setPixelSize(StelApp::getInstance().getScreenFontSize());
		setFont(font);
	});

	if (conf->value("gui/flag_info_shadow", false).toBool())
	{
		// Add a drop shadow for better visibility
		QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
		effect->setBlurRadius(6);
		effect->setColor(QColor(0, 0, 0));
		effect->setOffset(0,0);
		setGraphicsEffect(effect);
	}
}

InfoPanel::~InfoPanel()
{
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
		// Must set lastRTS for currently selected object here...
		StelCore *core=StelApp::getInstance().getCore();
		infoHTML = selected[0]->getInfoString(core, infoTextFilters);
		selected[0]->removeExtraInfoStrings(StelObject::AllInfo);
		setHtml(infoHTML);
	}
}

QString InfoPanel::getSelectedText() const
{
	return toPlainText();
}

QString InfoPanel::getSelectedHTML() const
{
	return infoHTML;
}

SkyGui::SkyGui(QGraphicsItem * parent)
	: QGraphicsWidget(parent)
	, lastBottomBarWidth(0)
	, btHorizAutoHide(nullptr)
	, btVertAutoHide(nullptr)
	, autoHidebts(nullptr)
	, autoHideBottomBar(true)
	, autoHideLeftBar(true)
	, toolbarAtTop(false)
	, toolbarAtRight(false)
	, stelGui(nullptr)
{
	setObjectName("StelSkyGui");

	// Construct the Windows buttons bar
	leftBar = new LeftStelBar(this);
	// Construct the bottom buttons bar
	bottomBar = new BottomStelBar(this,
				      QPixmap(":/graphicGui/btbgLeft.png"),
				      QPixmap(":/graphicGui/btbgRight.png"),
				      QPixmap(":/graphicGui/btbgMiddle.png"),
				      QPixmap(":/graphicGui/btbgSingle.png"));
	infoPanel = new InfoPanel(this);

	// Used to display some progress bar in the lower right corner, e.g. when loading a file
	progressBarMgr = new StelProgressBarMgr(this);
	connect(&StelApp::getInstance(), &StelApp::progressBarAdded,   progressBarMgr, &StelProgressBarMgr::addProgressBar);
	connect(&StelApp::getInstance(), &StelApp::progressBarRemoved, progressBarMgr, &StelProgressBarMgr::removeProgressBar);

	// The path drawn around the button bars
	buttonBarsFrame = new StelBarsFrame(this);

	animLeftBarTimeLine = new QTimeLine(200, this);
	animLeftBarTimeLine->setEasingCurve(QEasingCurve(QEasingCurve::InOutSine));
	connect(animLeftBarTimeLine, &QTimeLine::valueChanged, this, &SkyGui::updateBarsPos);

	animBottomBarTimeLine = new QTimeLine(200, this);
	animBottomBarTimeLine->setEasingCurve(QEasingCurve(QEasingCurve::InOutSine));
	connect(animBottomBarTimeLine, &QTimeLine::valueChanged, this, &SkyGui::updateBarsPos);

	setAcceptHoverEvents(true);
}

void SkyGui::init(StelGui* astelGui)
{
	stelGui = astelGui;

	// Create the 2 auto hide buttons placed in the active toolbar corner
	autoHidebts = new CornerButtons(this);
	QPixmap pxmapOn = QPixmap(":/graphicGui/miscHorAutoHide-on.png");
	QPixmap pxmapOff = QPixmap(":/graphicGui/miscHorAutoHide-off.png");
	btHorizAutoHide = new StelButton(autoHidebts, pxmapOn, pxmapOff, QPixmap(), "actionAutoHideHorizontalButtonBar", true);
	pxmapOn = QPixmap(":/graphicGui/miscVertAutoHide-on.png");
	pxmapOff = QPixmap(":/graphicGui/miscVertAutoHide-off.png");
	btVertAutoHide = new StelButton(autoHidebts, pxmapOn, pxmapOff, QPixmap(), "actionAutoHideVerticalButtonBar", true);

	btHorizAutoHide->setPos(1,btVertAutoHide->getButtonPixmapHeight()-btHorizAutoHide->getButtonPixmapHeight()+1);
	btVertAutoHide->setPos(0,0);
	btVertAutoHide->setZValue(1000);

	updateInfoPanelPos();

	// If auto hide is off, show the relevant toolbars
	if (!autoHideBottomBar)
	{
		animBottomBarTimeLine->setDirection(QTimeLine::Forward);
		animBottomBarTimeLine->start();
	}

	if (!autoHideLeftBar)
	{
		animLeftBarTimeLine->setDirection(QTimeLine::Forward);
		animLeftBarTimeLine->start();
	}

	buttonBarsFrame->setZValue(-0.1);
	updateBarsPos();
	connect(&StelApp::getInstance(), &StelApp::colorSchemeChanged, this, &SkyGui::setStelStyle);
	connect(&StelApp::getInstance(), &StelApp::screenFontSizeChanged, this, &SkyGui::updateInfoPanelPos);
	connect(bottomBar, &BottomStelBar::sizeChanged, this, &SkyGui::updateBarsPos);
	// The first draw of path may show overshooting date line if there are too few buttons in the bottom bar.
	// Correct this by a redraw 1/2s after startup
	QTimer::singleShot(500, this, [=](){buttonBarsFrame->updatePath(bottomBar, leftBar, toolbarAtTop, toolbarAtRight);});
}

void SkyGui::updateInfoPanelPos()
{
	const auto factor = StelApp::getInstance().screenFontSizeRatio();
	const qreal margin = 8 * factor;
	qreal infoX = margin;
	if (toolbarAtTop && !toolbarAtRight)
	{
		const qreal barRight = leftBar->pos().x() + leftBar->boundingRectNoHelpLabel().width();
		infoX = qMax(margin, barRight + margin);
	}
	qreal infoY = margin;
	if (toolbarAtTop)
	{
		const qreal barBottom = bottomBar->pos().y() + bottomBar->boundingRectNoHelpLabel().height();
		infoY = qMax(margin, barBottom + margin);
	}

	infoPanel->setPos(infoX, infoY);
}

void SkyGui::resizeEvent(QGraphicsSceneResizeEvent* event)
{
	QGraphicsWidget::resizeEvent(event);
	updateBarsPos();
}

void SkyGui::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
	const int ww = getSkyGuiWidth();
	const int hh = getSkyGuiHeight();

	const double x = event->pos().x();
	const double y = event->pos().y();
	const double rs = buttonBarsFrame->getRoundSize();
	const double leftBarW = leftBar->boundingRect().width();
	const double leftBarH = leftBar->boundingRect().height();
	const double bottomBarW = bottomBar->boundingRect().width();
	const double bottomBarH = bottomBar->boundingRect().height();

	// vertical bar hover trigger
	bool inLeftBarZone, outOfLeftBarZone;
	if (toolbarAtRight)
	{
		const double edgeX = ww - leftBarW - 2.*rs;
		if (toolbarAtTop)
		{
			inLeftBarZone    = (x >= edgeX) && (y <= leftBarH + bottomBarH + 2.*rs);
			outOfLeftBarZone = (x < edgeX - 30) || (y > leftBarH + bottomBarH + 2.*rs + 30);
		}
		else
		{
			inLeftBarZone    = (x >= edgeX) && (y >= hh - leftBarH - bottomBarH - 2.*rs);
			outOfLeftBarZone = (x < edgeX - 30) || (y < hh - leftBarH - bottomBarH - 2.*rs - 30);
		}
	}
	else
	{
		const double edgeX = leftBarW + 2.*rs;
		if (toolbarAtTop)
		{
			inLeftBarZone    = (x <= edgeX) && (y <= leftBarH + bottomBarH + 2.*rs);
			outOfLeftBarZone = (x > edgeX + 30) || (y > leftBarH + bottomBarH + 2.*rs + 30);
		}
		else
		{
			inLeftBarZone    = (x <= edgeX) && (y >= hh - leftBarH - bottomBarH - 2.*rs);
			outOfLeftBarZone = (x > edgeX + 30) || (y < hh - leftBarH - bottomBarH - 2.*rs - 30);
		}
	}

	if (inLeftBarZone && animLeftBarTimeLine->state()==QTimeLine::NotRunning && animLeftBarTimeLine->currentValue()<1.)
	{
		animLeftBarTimeLine->setDirection(QTimeLine::Forward);
		animLeftBarTimeLine->start();
	}
	if (autoHideLeftBar && outOfLeftBarZone && animLeftBarTimeLine->state()==QTimeLine::NotRunning && animLeftBarTimeLine->currentValue()>=0.9999999)
	{
		animLeftBarTimeLine->setDirection(QTimeLine::Backward);
		animLeftBarTimeLine->start();
	}

	// horizontal bar hover trigger
	bool inBottomBarZone, outOfBottomBarZone;
	if (toolbarAtRight)
	{
		const double edgeX = ww - leftBarW - bottomBarW - 2.*rs;
		if (toolbarAtTop)
		{
			inBottomBarZone    = (x >= edgeX) && (y <= bottomBarH + 2.*rs);
			outOfBottomBarZone = (x < edgeX - 30) || (y > bottomBarH + 2.*rs + 30);
		}
		else
		{
			inBottomBarZone    = (x >= edgeX) && (y >= hh - bottomBarH + 2.*rs);
			outOfBottomBarZone = (x < edgeX - 30) || (y < hh - bottomBarH + 2.*rs - 30);
		}
	}
	else
	{
		const double edgeX = leftBarW + bottomBarW + 2.*rs;
		if (toolbarAtTop)
		{
			inBottomBarZone    = (x <= edgeX) && (y <= bottomBarH + 2.*rs);
			outOfBottomBarZone = (x > edgeX + 30) || (y > bottomBarH + 2.*rs + 30);
		}
		else
		{
			inBottomBarZone    = (x <= edgeX) && (y >= hh - bottomBarH + 2.*rs);
			outOfBottomBarZone = (x > edgeX + 30) || (y < hh - bottomBarH + 2.*rs - 30);
		}
	}

	if (inBottomBarZone && animBottomBarTimeLine->state()==QTimeLine::NotRunning && animBottomBarTimeLine->currentValue()<1.)
	{
		animBottomBarTimeLine->setDirection(QTimeLine::Forward);
		animBottomBarTimeLine->start();
	}
	if (autoHideBottomBar && outOfBottomBarZone && animBottomBarTimeLine->state()==QTimeLine::NotRunning && animBottomBarTimeLine->currentValue()>=0.9999999)
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
	return bottomBar->boundingRect().height();
}

qreal SkyGui::getLeftBarWidth() const
{
	return leftBar->boundingRect().width();
}

//! Update the position of the button bars in the main window
void SkyGui::updateBarsPos()
{
	// auto-hide buttons are created in init(), so return if called before that
	if (!autoHidebts) return;

	const int ww = getSkyGuiWidth();  // actually: window width
	const int hh = getSkyGuiHeight(); // actually: window height
	bool updatePath = false;

	// Update the layout of the horizontal bar (text above or below buttons)
	bottomBar->setBarAtTop(toolbarAtTop);
	leftBar->setBarAtRight(toolbarAtRight);

	const qreal leftBarW   = leftBar->boundingRectNoHelpLabel().width();
	const qreal leftBarH   = leftBar->boundingRectNoHelpLabel().height();
	const qreal bottomBarH = bottomBar->boundingRectNoHelpLabel().height();
	const qreal rs         = buttonBarsFrame->getRoundSize();

	// vertical bar position
	const double rangeX = leftBarW + 2. * rs + 1.;
	qreal newLeftBarX, newLeftBarY;

	const double hiddenOffsetX = (1. - animLeftBarTimeLine->currentValue()) * rangeX;
	if (toolbarAtRight)
	{
		newLeftBarX = ww - leftBarW - rs + hiddenOffsetX + 0.5;
	}
	else
	{
		newLeftBarX = rs - hiddenOffsetX - 0.5;
	}

	if (toolbarAtTop)
	{
		newLeftBarY = bottomBarH + 20;
	}
	else
	{
		newLeftBarY = hh - leftBarH - bottomBarH - 20;
	}

	if (!qFuzzyCompare(leftBar->pos().x(), newLeftBarX) || !qFuzzyCompare(leftBar->pos().y(), newLeftBarY))
	{
		leftBar->setPos(qRound(newLeftBarX), qRound(newLeftBarY));
		updatePath = true;
	}

	// horizontal bar position
	const double rangeY = bottomBar->getButtonsBoundingRect().height() + 1.5 + bottomBar->getGap();
	qreal newBottomBarX, newBottomBarY;

	const double hiddenOffsetY = (1. - animBottomBarTimeLine->currentValue()) * rangeY;
	if (toolbarAtRight)
	{
		newBottomBarX = ww - bottomBar->boundingRectNoHelpLabel().width() - leftBarW - rs;
	}
	else
	{
		newBottomBarX = leftBarW + rs;
	}

	if (toolbarAtTop)
	{
		newBottomBarY = rs - bottomBar->getGap() - 0.5 - hiddenOffsetY;
	}
	else
	{
		newBottomBarY = hh - bottomBar->boundingRectNoHelpLabel().height() + bottomBar->getGap() - rs + 0.5 + hiddenOffsetY;
	}

	if (!qFuzzyCompare(bottomBar->pos().x(), newBottomBarX) || !qFuzzyCompare(bottomBar->pos().y(), newBottomBarY))
	{
		bottomBar->setPos(qRound(newBottomBarX), qRound(newBottomBarY));
		updatePath = true;
	}

	if (lastBottomBarWidth != static_cast<int>(bottomBar->boundingRectNoHelpLabel().width()))
	{
		lastBottomBarWidth = static_cast<int>(bottomBar->boundingRectNoHelpLabel().width());
		updatePath = true;
	}

	if (updatePath)
		buttonBarsFrame->updatePath(bottomBar, leftBar, toolbarAtTop, toolbarAtRight);

	const qreal newProgressBarX = ww-progressBarMgr->boundingRect().width()-20;
	const qreal newProgressBarY = hh-progressBarMgr->boundingRect().height()+7;	
	progressBarMgr->setPos(newProgressBarX, newProgressBarY);
	progressBarMgr->setZValue(400);

	// Update position of the auto-hide buttons.
	const int vW = btVertAutoHide->getButtonPixmapWidth();
	const int vH = btVertAutoHide->getButtonPixmapHeight();
	const int hW = btHorizAutoHide->getButtonPixmapWidth();
	const int hH = btHorizAutoHide->getButtonPixmapHeight();

	// Mirror the button pixmaps to keep the angled edge facing the correct corner.
	const QTransform t = QTransform::fromScale(toolbarAtRight ? -1. : 1., toolbarAtTop ? -1. : 1.);
	btVertAutoHide->setTransform(t);
	btHorizAutoHide->setTransform(t);

	// Desired content origin in autoHidebts coords (before flip-transform offset is applied).
	const int vDesX = toolbarAtRight ? (hW - 1) : 0;
	const int hDesX = toolbarAtRight ? (vW - 2) : 1;
	const int hDesY = toolbarAtTop ? 0 : (vH - hH + 1);

	// Update position of the auto-hide buttons
	btVertAutoHide->setPos(vDesX + (toolbarAtRight ? vW : 0), toolbarAtTop ? vH : 0);
	btHorizAutoHide->setPos(hDesX + (toolbarAtRight ? hW : 0), hDesY + (toolbarAtTop ? hH : 0));

	// Position the autoHidebts widget so its content touches the active screen corner.
	autoHidebts->setPos(toolbarAtRight ? (ww - autoHidebts->childrenBoundingRect().right()) : 0,
	                    toolbarAtTop ? 0 : (hh - autoHidebts->childrenBoundingRect().bottom() + 1));

	double opacity = qMax(animLeftBarTimeLine->currentValue(), animBottomBarTimeLine->currentValue());
	autoHidebts->setOpacity(qMax(0.01, opacity));	// Work around a qt bug

	// Update the screen as soon as possible.
	updateInfoPanelPos();
	StelMainView::getInstance().thereWasAnEvent();
}

void SkyGui::setStelStyle(const QString& style)
{
	Q_UNUSED(style)
	buttonBarsFrame->setPen(QColor::fromRgbF(0.7,0.7,0.7,0.5));
	buttonBarsFrame->setBrush(QColor::fromRgbF(0.15, 0.16, 0.19, 0.2));
	bottomBar->setColor(QColor::fromRgbF(0.9, 0.91, 0.95, 0.9));
	leftBar->setColor(QColor::fromRgbF(0.9, 0.91, 0.95, 0.9));
}

// Add a new progress bar in the lower right corner of the screen.
void SkyGui::addProgressBar(StelProgressController* p)
{
	progressBarMgr->addProgressBar(p);
}

void SkyGui::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
{
	stelGui->update();
}
