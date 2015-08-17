/*
 * Stellarium
 * Copyright (C) 2013 Guillaume Chereau
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

#include "StelQuickView.hpp"
#include "StelActionMgr.hpp"
#include "StelActionMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelPainter.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelTranslator.hpp"

#include <QQmlContext>
#include <QQuickItem>
#include <QDebug>
#include <QSettings>

StelQuickView* StelQuickView::singleton = NULL;

HoverArea::HoverArea(QQuickItem *parent) : QQuickItem(parent)
	,m_hovered(false)
{
	setAcceptHoverEvents(true);
	startTimer(16);
}

void HoverArea::timerEvent(QTimerEvent *event)
{
	Q_UNUSED(event);
	QPointF pos = window()->mapFromGlobal(QCursor::pos());
	if (contains(mapFromScene(pos)) != m_hovered)
	{
		m_hovered = !m_hovered;
		emit hoveredChanged();
	}
}

StelQuickView::StelQuickView() : stelApp(NULL)
	, initState(0)
{
	singleton = this;
	connect(this, SIGNAL(beforeRendering()), this, SLOT(paint()), Qt::DirectConnection);
	connect(this, SIGNAL(beforeSynchronizing()), this, SLOT(synchronize()), Qt::DirectConnection);
	connect(this, SIGNAL(initialized()), this, SLOT(showGui()));
	qmlRegisterType<HoverArea>("org.stellarium", 1, 0, "HoverArea");
	qmlRegisterType<StelAction>("org.stellarium", 1, 0, "StelAction");
}

void StelQuickView::init(QSettings* conf)
{
	this->conf = conf;
	setResizeMode(QQuickView::SizeRootObjectToView);
	setClearBeforeRendering(false);
	int width = conf->value("video/screen_w", 480).toInt();
	int height = conf->value("video/screen_h", 700).toInt();
	resize(width, height);
	show();
	startTimer(16);
}

void StelQuickView::deinit()
{
	delete stelApp;
	stelApp = NULL;
	StelApp::deinitStatic();
	StelPainter::deinitGLShaders();
}

void StelQuickView::timerEvent(QTimerEvent* event)
{
	update();
}

void StelQuickView::keyPressEvent(QKeyEvent* event)
{
	stelApp->handleKeys(event);
}

void StelQuickView::keyReleaseEvent(QKeyEvent* event)
{
	stelApp->handleKeys(event);
}

void StelQuickView::synchronize()
{
	if (initState == 0)
	{
		stelApp = new StelApp();
		StelApp::initStatic();
		stelApp->init(conf);
		StelPainter::initGLShaders();
		stelApp->glWindowHasBeenResized(0, 0, width(), height());
		setFlags(Qt::Window);
		initState++;
		emit initialized();
	}
}

void StelQuickView::showGui()
{
	Q_ASSERT(stelApp);
	rootContext()->setContextProperty("core", stelApp->getCore());
	rootContext()->setContextProperty("stelGui", stelApp->getGui());
	rootContext()->setContextProperty("stelApp", stelApp);
	rootContext()->setContextProperty("stelActionMgr", stelApp->getStelActionManager());
	rootContext()->setContextProperty("SkyCultureMgr", &stelApp->getSkyCultureMgr());
	rootContext()->setContextProperty("LocaleMgr", &stelApp->getLocaleMgr());

	// Set the global objects name, same as in the scripting module.
	StelModuleMgr* mmgr = &StelApp::getInstance().getModuleMgr();
	foreach (StelModule* m, mmgr->getAllModules())
	{
		rootContext()->setContextProperty(m->objectName(), m);
	}
	rootContext()->setContextProperty("StelSkyDrawer", stelApp->getCore()->getSkyDrawer());

	setSource(QUrl("qrc:/qml/main.qml"));
}

void StelQuickView::paint()
{
	if (stelApp==NULL)
		return;
	static double lastPaint = -1.;
	double newTime = StelApp::getTotalRunTime();
	if (lastPaint<0)
		lastPaint = newTime-0.01;
	stelApp->update(newTime-lastPaint);
	lastPaint = newTime;
	stelApp->draw();
}
