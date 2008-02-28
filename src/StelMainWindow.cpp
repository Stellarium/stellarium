/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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

#include <config.h>

#include "GLee.h"
#include "fixx11h.h"

#include "StelMainWindow.hpp"
#include "StelAppGraphicsItem.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "Projector.hpp"
#include "StelModuleMgr.hpp"

#include <QGLWidget>
#include <QSettings>
#include <QCoreApplication>
#include <QDebug>
#include <QGraphicsView>
#include <QResizeEvent>
#include <QDebug>

#include "gui/NewGui.hpp"

class StelQGraphicsView : public QGraphicsView
{
public:
	StelQGraphicsView(QGraphicsScene* ascene, QWidget* parent) : QGraphicsView(ascene, parent) {;}
protected:	
	virtual void resizeEvent(QResizeEvent* event)
	{
		setSceneRect(0,0,event->size().width(), event->size().height());
		StelAppGraphicsItem::getInstance().glWindowHasBeenResized(event->size().width(), event->size().height());
	}
};

// Initialize static variables
StelMainWindow* StelMainWindow::singleton = NULL;
		 
StelMainWindow::StelMainWindow()
{
	setObjectName("stellariumMainWin");
	
	// Can't create 2 StelMainWindow instances
	assert(!singleton);
	singleton = this;
	show();
}

void StelMainWindow::init(int argc, char** argv)
{
	// Create the main instance of stellarium
	StelApp* stelApp = new StelApp(argc, argv, this);
	
	// Init the main window. It must be done here because it is not the responsability of StelApp to do that
	QSettings* settings = stelApp->getSettings();
	resize(settings->value("video/screen_w", 800).toInt(), settings->value("video/screen_h", 600).toInt());
	if (settings->value("video/fullscreen", true).toBool())
	{
		showFullScreen();
	}
	
	// Create a graphicScene and a GraphicView drawing in an openGL widget
	QGraphicsScene* scene = new QGraphicsScene(this);
	StelAppGraphicsItem* mainItem = new StelAppGraphicsItem();
	scene->addItem(mainItem);
	mainItem->setFocus();
	view = new StelQGraphicsView(scene, this);
	view->setFrameShape(QFrame::NoFrame);
	view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	view->setFocusPolicy(Qt::ClickFocus);

	QGLFormat glFormat(QGL::StencilBuffer);
	//glFormat.setSamples(1);
	glWidget = new QGLWidget(glFormat, this);
	glWidget->setObjectName("StelGLWidget");
	glWidget->setAutoFillBackground(false);
	view->setAutoFillBackground(false);
	view->setViewport(glWidget);
 	view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
	
	//view->setCacheMode(QGraphicsView::CacheBackground);
	// Antialiasing works only with SampleBuffer, but it's much slower
	view->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
	
	// Use it as a central widget
	setCentralWidget(view);
	
	QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	stelApp->init();
	mainItem->init();
	view->setFocus();
	
	NewGui* newGui = new NewGui();
	newGui->init();
	StelApp::getInstance().getModuleMgr().registerModule(newGui, true);
	
	stelApp->initPlugIns();
	
	mainItem->startDrawingLoop();
}

StelMainWindow::~StelMainWindow()
{
}

void StelMainWindow::saveScreenShot(const QString& filePrefix, const QString& saveDir) const
{
	QString shotDir;
	QImage im = findChild<QGLWidget*>("StelGLWidget")->grabFrameBuffer();

	if (saveDir == "")
	{
		try
		{
			shotDir = StelApp::getInstance().getFileMgr().getScreenshotDir();
			if (!StelApp::getInstance().getFileMgr().isWritable(shotDir))
			{
				qWarning() << "ERROR StelAppSdl::saveScreenShot: screenshot directory is not writable: " << qPrintable(shotDir);
				return;
			}
		}
		catch(std::exception& e)
		{
			qWarning() << "ERROR StelAppSdl::saveScreenShot: could not determine screenshot directory: " << e.what();
			return;
		}
	}
	else
	{
		shotDir = saveDir;
	}

	QString shotPath;
	for (int j=0; j<100000; ++j)
	{
		shotPath = shotDir+"/"+filePrefix + QString("%1").arg(j, 3, 10, QLatin1Char('0')) + ".png";
		if (!StelApp::getInstance().getFileMgr().exists(shotPath))
			break;
	}
	
	qDebug() << "Saving screenshot in file: " << shotPath;
	im.save(shotPath);
}


/*************************************************************************
 Alternate fullscreen mode/windowed mode if possible
*************************************************************************/
void StelMainWindow::toggleFullScreen()
{
	// Toggle full screen
	if (!isFullScreen())
	{
		showFullScreen();
	}
	else
	{
		showNormal();
	}
}

/*************************************************************************
 Get whether fullscreen is activated or not
*************************************************************************/
bool StelMainWindow::getFullScreen() const
{
	return isFullScreen();
}

/*************************************************************************
 Set whether fullscreen is activated or not
 *************************************************************************/
void StelMainWindow::setFullScreen(bool b)
{
	if (b)
		showFullScreen();
	else
		showNormal();
}
