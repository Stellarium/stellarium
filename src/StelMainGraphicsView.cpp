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

#include "StelMainGraphicsView.hpp"
#include "StelAppGraphicsScene.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "Projector.hpp"
#include "StelModuleMgr.hpp"

#include <QGLFormat>
#include <QGraphicsView>
#include <QGLWidget>
#include <QResizeEvent>
#include <QSettings>
#include <QCoreApplication>
#include <QDebug>
#include <QThread>
#include <QAction>

#include "gui/StelGui.hpp"

// Initialize static variables
StelMainGraphicsView* StelMainGraphicsView::singleton = NULL;

StelMainGraphicsView::StelMainGraphicsView(QWidget* parent, int argc, char** argv)  : QGraphicsView(parent), wasDeinit(false)
{
	setScene(new QGraphicsScene(this));
	// Can't create 2 StelMainWindow instances
	assert(!singleton);
	singleton = this;

	setObjectName("Mainview");
	
	// Avoid white background at init
	setStyleSheet(QString("QGraphicsView {background: #000;}"));
	setFrameShape(QFrame::NoFrame);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	
	// Create an openGL viewport
 	QGLFormat glFormat(QGL::StencilBuffer);
	//glFormat.setSamples(16);
	//glFormat.setSampleBuffers(true);
	//glFormat.setDirectRendering(false);
	glWidget = new QGLWidget(glFormat, NULL);
	setViewport(glWidget);
	
	setFocusPolicy(Qt::ClickFocus);
	
	// Create the main instance of stellarium
	stelApp = new StelApp(argc, argv);
	setScene(new StelAppGraphicsScene());
}

StelMainGraphicsView::~StelMainGraphicsView()
{
}

void StelMainGraphicsView::resizeEvent(QResizeEvent* event)
{
	if (scene())
		scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
	QGraphicsView::resizeEvent(event);
}

void StelMainGraphicsView::init()
{
	//setAutoFillBackground(false);
	
	// This apparently useless line fixes the scrolling bug
	// I suspect a Qt 4.4 bug -> Fixed with Qt 4.4.1
#if QT_VERSION == 0x040400
	setMatrix(QMatrix(1,0,0,1,0.00000001,0));
#endif
	
	// Antialiasing works only with SampleBuffer, but it's much slower
	// setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
	//setRenderHint(QPainter::TextAntialiasing, false);
			
	QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	stelApp->init();
	static_cast<StelAppGraphicsScene*>(scene())->init();
	setFocus();
	
	// Allows for precise FPS control
	setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
	
	//setOptimizationFlags(QGraphicsView::DontClipPainter|QGraphicsView::DontSavePainterState|QGraphicsView::DontAdjustForAntialiasing);
	
	StelGui* newGui = new StelGui();
	newGui->init();
	StelApp::getInstance().getModuleMgr().registerModule(newGui, true);
	
	stelApp->initPlugIns();
	
	QThread::currentThread()->setPriority(QThread::HighestPriority);
}

//! Delete openGL textures (to call before the GLContext disappears)
void StelMainGraphicsView::deinitGL()
{
	// Can be called only once
	if (wasDeinit==true)
		return;
	wasDeinit = true;
	StelApp::getInstance().getModuleMgr().removeModule("StelGui");
	QCoreApplication::processEvents();
	StelApp* stelApp = &StelApp::getInstance();
	delete stelApp;
}

void StelMainGraphicsView::saveScreenShot(const QString& filePrefix, const QString& saveDir) const
{
	QString shotDir;
	QImage im = glWidget->grabFrameBuffer();

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

// Add a new progress bar in the lower right corner of the screen.
class QProgressBar* StelMainGraphicsView::addProgessBar()
{
	StelGui* gui = (StelGui*)GETSTELMODULE("StelGui");
	return gui->addProgessBar();
}

//! Activate all the QActions associated to the widget
void StelMainGraphicsView::activateKeyActions(bool b)
{
	QList<QAction*> aList = findChildren<QAction*>("actionQuit");
	QAction* actionQuit = aList.size() > 0 ? aList.at(0) : NULL;
	if (b==false)
	{
		foreach (QAction* a, actions())
		{
			// Special case for the Quit action shortcut which is always valid
			if (a!=actionQuit)
				removeAction(a);
		}
	}
	else
	{
		foreach (QAction* a, findChildren<QAction*>())
		{
			// Special case for the Quit action shortcut which is always valid
			if (a!=actionQuit)
				addAction(a);
		}
	}
}
