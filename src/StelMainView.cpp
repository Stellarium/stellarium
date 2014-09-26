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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelMainView.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelProjector.hpp"
#include "StelModuleMgr.hpp"
#include "StelPainter.hpp"
#include "StelGuiBase.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "StelActionMgr.hpp"
#include "StelOpenGL.hpp"

#include <QDeclarativeItem>
#include <QDebug>
#include <QDir>
#include <QGLWidget>
#include <QGuiApplication>
#include <QFileInfo>
#include <QIcon>
#include <QMoveEvent>
#include <QPluginLoader>
#include <QScreen>
#include <QSettings>
#include <QtPlugin>
#include <QThread>
#include <QTimer>
#include <QWidget>
#include <QWindow>
#include <QDeclarativeContext>
#include <QPinchGesture>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>

#include <clocale>

// Initialize static variables
StelMainView* StelMainView::singleton = NULL;

//! Render Stellarium sky. 
class StelSkyItem : public QDeclarativeItem
{
public:
	StelSkyItem(QDeclarativeItem* parent = NULL);
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
protected:
	void mousePressEvent(QGraphicsSceneMouseEvent* event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	void wheelEvent(QGraphicsSceneWheelEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	bool event(QEvent * e);
private:
	double previousPaintTime;
	void onSizeChanged();
	void pinchTriggered(QPinchGesture *gesture);
	bool gestureEvent(QGestureEvent *event);
};

//! Initialize and render Stellarium gui.
class StelGuiItem : public QDeclarativeItem
{
public:
	StelGuiItem(QDeclarativeItem* parent = NULL);
private:
	QGraphicsWidget *widget;
	void onSizeChanged();
};

StelSkyItem::StelSkyItem(QDeclarativeItem* parent)
{
	Q_UNUSED(parent);
	setObjectName("SkyItem");
	setFlag(QGraphicsItem::ItemHasNoContents, false);
	setAcceptHoverEvents(true);
	setAcceptTouchEvents(true);
	grabGesture(Qt::PinchGesture);
	setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
	connect(this, &StelSkyItem::widthChanged, this, &StelSkyItem::onSizeChanged);
	connect(this, &StelSkyItem::heightChanged, this, &StelSkyItem::onSizeChanged);
	previousPaintTime = StelApp::getTotalRunTime();
	StelMainView::getInstance().skyItem = this;
	setFocus(true);
}

void StelSkyItem::onSizeChanged()
{
	StelApp::getInstance().glWindowHasBeenResized(x(), y(), width(), height());
}

void StelSkyItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	const double now = StelApp::getTotalRunTime();
	double dt = now - previousPaintTime;
	previousPaintTime = now;

	painter->beginNativePainting();
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	StelApp::getInstance().update(dt);
	StelApp::getInstance().draw();

	painter->endNativePainting();
}

void StelSkyItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	//To get back the focus from dialogs
	this->setFocus(true);
	QPointF pos = event->scenePos();
	pos.setY(height() - 1 - pos.y());
	QMouseEvent newEvent(QEvent::MouseButtonPress, QPoint(pos.x(), pos.y()), event->button(), event->buttons(), event->modifiers());
	StelApp::getInstance().handleClick(&newEvent);
}

void StelSkyItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	QPointF pos = event->scenePos();
	pos.setY(height() - 1 - pos.y());
	QMouseEvent newEvent(QEvent::MouseButtonRelease, QPoint(pos.x(), pos.y()), event->button(), event->buttons(), event->modifiers());
	StelApp::getInstance().handleClick(&newEvent);
}

void StelSkyItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	QPointF pos = event->scenePos();
	pos.setY(height() - 1 - pos.y());
	StelApp::getInstance().handleMove(pos.x(), pos.y(), event->buttons());
}

void StelSkyItem::wheelEvent(QGraphicsSceneWheelEvent *event)
{
	QPointF pos = event->scenePos();
	pos.setY(height() - 1 - pos.y());
	QWheelEvent newEvent(QPoint(pos.x(),pos.y()), event->delta(), event->buttons(), event->modifiers(), event->orientation());
	StelApp::getInstance().handleWheel(&newEvent);
}

bool StelSkyItem::event(QEvent * e)
{
	switch (e->type()){
	case QEvent::TouchBegin:
	case QEvent::TouchUpdate:
	case QEvent::TouchEnd:
	{
		QTouchEvent *touchEvent = static_cast<QTouchEvent *>(e);
		QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();

		if (touchPoints.count() == 1)
			setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);

		return true;
	}
		break;

	case QEvent::Gesture:
		setAcceptedMouseButtons(0);
		return gestureEvent(static_cast<QGestureEvent*>(e));
		break;

	default:
		return false;
	}
}

bool StelSkyItem::gestureEvent(QGestureEvent *event)
{
	if (QGesture *pinch = event->gesture(Qt::PinchGesture))
		pinchTriggered(static_cast<QPinchGesture *>(pinch));

	return true;
}

void StelSkyItem::pinchTriggered(QPinchGesture *gesture)
{
	QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();
	if (changeFlags & QPinchGesture::ScaleFactorChanged) {
		qreal zoom = gesture->scaleFactor();

		if (zoom < 2 && zoom > 0.5){
			StelApp::getInstance().handlePinch(zoom, true);
		}
	}
}

void StelSkyItem::keyPressEvent(QKeyEvent* event)
{
	StelApp::getInstance().handleKeys(event);
}

void StelSkyItem::keyReleaseEvent(QKeyEvent* event)
{
	StelApp::getInstance().handleKeys(event);
}


StelGuiItem::StelGuiItem(QDeclarativeItem* parent) : QDeclarativeItem(parent)
{
	connect(this, &StelGuiItem::widthChanged, this, &StelGuiItem::onSizeChanged);
	connect(this, &StelGuiItem::heightChanged, this, &StelGuiItem::onSizeChanged);
	widget = new QGraphicsWidget(this);
	StelMainView::getInstance().guiWidget = widget;
	StelApp::getInstance().getGui()->init(widget);
}

void StelGuiItem::onSizeChanged()
{
	// I whish I could find a way to let Qt automatically resize the widget
	// when the QDeclarativeItem size changes.
	widget->setGeometry(0, 0, width(), height());
	StelApp::getInstance().getGui()->forceRefreshGui();
}

class StelQGLWidget : public QGLWidget
{
public:
	StelQGLWidget(QGLContext* ctx, QWidget* parent) : QGLWidget(ctx, parent)
	{
		setAttribute(Qt::WA_PaintOnScreen);
		setAttribute(Qt::WA_NoSystemBackground);
		setAttribute(Qt::WA_OpaquePaintEvent);
	}

protected:
	virtual void initializeGL()
	{
		qDebug() << "It appears this is never called?";
		Q_ASSERT(0);
		qDebug() << "OpenGL supported version: " << QString((char*)glGetString(GL_VERSION));

		QGLWidget::initializeGL();

		qDebug() << "Current Context: " << this->format();
		if (!format().stencil())
			qWarning("Could not get stencil buffer; results will be suboptimal");
		if (!format().depth())
			qWarning("Could not get depth buffer; results will be suboptimal");
		if (!format().doubleBuffer())
			qWarning("Could not get double buffer; results will be suboptimal");
	}

};

StelMainView::StelMainView(QWidget* parent)
	: QDeclarativeView(parent), gui(NULL),
	  flagInvertScreenShotColors(false),
	  screenShotPrefix("stellarium-"),
	  screenShotDir(""),
	  cursorTimeout(-1.f), flagCursorTimeout(false), minFpsTimer(NULL), maxfps(10000.f)
{
	StelApp::initStatic();
	
	// Can't create 2 StelMainView instances
	Q_ASSERT(!singleton);
	singleton = this;

	setWindowIcon(QIcon(":/mainWindow/icon.bmp"));
	initTitleI18n();
	setObjectName("Mainview");

	// Allows for precise FPS control
	setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
	setFrameShape(QFrame::NoFrame);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setFocusPolicy(Qt::StrongFocus);
	connect(this, SIGNAL(screenshotRequested()), this, SLOT(doScreenshot()));

	lastEventTimeSec = 0;

	// Create an openGL viewport
	QGLFormat glFormat(QGL::StencilBuffer | QGL::DepthBuffer | QGL::DoubleBuffer);
	glWidget = new StelQGLWidget(new QGLContext(glFormat), this);
	setViewport(glWidget);

	// Workaround (see Bug #940638) Although we have already explicitly set
	// LC_NUMERIC to "C" in main.cpp there seems to be a bug in OpenGL where
	// it will silently reset LC_NUMERIC to the value of LC_ALL during OpenGL
	// initialization. This has been observed on Ubuntu 11.10 under certain
	// circumstances, so here we set it again just to be on the safe side.
	setlocale(LC_NUMERIC, "C");
	// End workaround
}

void StelMainView::focusSky() {
	StelMainView::getInstance().scene()->setActiveWindow(0);
	QGraphicsObject* skyItem = rootObject()->findChild<QGraphicsObject*>("SkyItem");
	Q_ASSERT(skyItem);
	skyItem->setFocus();
}

StelMainView::~StelMainView()
{
	StelApp::deinitStatic();
}

void StelMainView::init(QSettings* conf)
{
	// Look for a static GUI plugins.
	foreach (QObject *plugin, QPluginLoader::staticInstances())
	{
		StelGuiPluginInterface* pluginInterface = qobject_cast<StelGuiPluginInterface*>(plugin);
		if (pluginInterface)
		{
			gui = pluginInterface->getStelGuiBase();
		}
		break;
	}
	Q_ASSERT(gui);

	Q_ASSERT(glWidget->isValid());
	glWidget->makeCurrent();

	// Debug info about supported version of OpenGL and vendor/renderer
	qDebug() << "OpenGL versions supported:" << getSupportedOpenGLVersion();
	qDebug() << "Driver version string:" << QString(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
	qDebug() << "GL vendor is" << QString(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	qDebug() << "GL renderer is" << QString(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	qDebug() << "GL Shading Language version is" << QString(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
	
	// Only give extended info if called on command line, for diagnostic.
	if (qApp->property("dump_OpenGL_details").toBool())
		dumpOpenGLdiagnostics();

	stelApp= new StelApp();
	stelApp->setGui(gui);
	stelApp->init(conf);
	StelActionMgr *actionMgr = stelApp->getStelActionManager();
	actionMgr->addAction("actionSave_Screenshot_Global", N_("Miscellaneous"), N_("Save screenshot"), this, "saveScreenShot()", "Ctrl+S");
	actionMgr->addAction("actionSet_Full_Screen_Global", N_("Display Options"), N_("Full-screen mode"), this, "fullScreen", "F11");
	
	StelPainter::initGLShaders();

	setResizeMode(QDeclarativeView::SizeRootObjectToView);
	qmlRegisterType<StelSkyItem>("Stellarium", 1, 0, "StelSky");
	qmlRegisterType<StelGuiItem>("Stellarium", 1, 0, "StelGui");
	rootContext()->setContextProperty("stelApp", stelApp);	
	setSource(QUrl("qrc:/qml/main.qml"));
	
	QSize size = glWidget->windowHandle()->screen()->size();
	size = QSize(conf->value("video/screen_w", size.width()).toInt(),
		     conf->value("video/screen_h", size.height()).toInt());

	bool fullscreen = conf->value("video/fullscreen", true).toBool();

	// Without this, the screen is not shown on a Mac + we should use resize() for correct work of fullscreen/windowed mode switch. --AW WTF???
	resize(size);

	if (fullscreen)
	{
		setFullScreen(true);
	}
	else
	{
		setFullScreen(false);
		int x = conf->value("video/screen_x", 0).toInt();
		int y = conf->value("video/screen_y", 0).toInt();
		move(x, y);	
	}

	flagInvertScreenShotColors = conf->value("main/invert_screenshots_colors", false).toBool();
	setFlagCursorTimeout(conf->value("gui/flag_mouse_cursor_timeout", false).toBool());
	setCursorTimeout(conf->value("gui/mouse_cursor_timeout", 10.f).toFloat());
	maxfps = conf->value("video/maximum_fps",10000.f).toFloat();
	minfps = conf->value("video/minimum_fps",10000.f).toFloat();
	flagMaxFpsUpdatePending = false;

	// XXX: This should be done in StelApp::init(), unfortunately for the moment we need init the gui before the
	// plugins, because the gui create the QActions needed by some plugins.
	StelApp::getInstance().initPlugIns();

	QThread::currentThread()->setPriority(QThread::HighestPriority);
	startMainLoop();
}

QString StelMainView::getSupportedOpenGLVersion() const
{
	int version = QGLFormat::openGLVersionFlags();
	QStringList ver;

	if (version&QGLFormat::OpenGL_Version_1_1)
		ver << "1.1";
	if (version&QGLFormat::OpenGL_Version_1_2)
		ver << "1.2";
	if (version&QGLFormat::OpenGL_Version_1_3)
		ver << "1.3";
	if (version&QGLFormat::OpenGL_Version_1_4)
		ver << "1.4";
	if (version&QGLFormat::OpenGL_Version_1_5)
		ver << "1.5";
	if (version&QGLFormat::OpenGL_Version_2_0)
		ver << "2.0";
	if (version&QGLFormat::OpenGL_Version_2_1)
		ver << "2.1";
	if (version&QGLFormat::OpenGL_Version_3_0)
		ver << "3.0";
	if (version&QGLFormat::OpenGL_Version_3_1)
		ver << "3.1";
	if (version&QGLFormat::OpenGL_Version_3_2)
		ver << "3.2";
	if (version&QGLFormat::OpenGL_Version_3_3)
		ver << "3.3";
	if (version&QGLFormat::OpenGL_Version_4_0)
		ver << "4.0";
	if (version&QGLFormat::OpenGL_Version_4_1)
		ver << "4.1";
	if (version&QGLFormat::OpenGL_Version_4_2)
		ver << "4.2";
	if (version&QGLFormat::OpenGL_Version_4_3)
		ver << "4.3";
	if (version&QGLFormat::OpenGL_ES_CommonLite_Version_1_0)
		ver << "1.0 (ES CL)";
	if (version&QGLFormat::OpenGL_ES_CommonLite_Version_1_1)
		ver << "1.1 (ES CL)";
	if (version&QGLFormat::OpenGL_ES_Common_Version_1_0)
		ver << "1.0 (ES C)";
	if (version&QGLFormat::OpenGL_ES_Common_Version_1_1)
		ver << "1.1 (ES C)";
	if (version&QGLFormat::OpenGL_ES_Version_2_0)
		ver << "2.0 (ES)";

	return ver.join(", ");
}

void StelMainView::dumpOpenGLdiagnostics() const
{
	// GZ: Debug info about OpenGL capabilities.
	QOpenGLContext *context = QOpenGLContext::currentContext();
	if (context)
	{
		context->functions()->initializeOpenGLFunctions();
		qDebug() << "initializeOpenGLFunctions()...";
	}
	else
		qDebug() << "No OpenGL context";

	QOpenGLFunctions::OpenGLFeatures oglFeatures=context->functions()->openGLFeatures();
	qDebug() << "OpenGL Features:";
	qDebug() << " - glActiveTexture() function" << (oglFeatures&QOpenGLFunctions::Multitexture ? "is" : "is NOT") << "available.";
	qDebug() << " - Shader functions" << (oglFeatures&QOpenGLFunctions::Shaders ? "are" : "are NOT ") << "available.";
	qDebug() << " - Vertex and index buffer functions" << (oglFeatures&QOpenGLFunctions::Buffers ? "are" : "are NOT") << "available.";
	qDebug() << " - Framebuffer object functions" << (oglFeatures&QOpenGLFunctions::Framebuffers ? "are" : "are NOT") << "available.";
	qDebug() << " - glBlendColor()" << (oglFeatures&QOpenGLFunctions::BlendColor ? "is" : "is NOT") << "available.";
	qDebug() << " - glBlendEquation()" << (oglFeatures&QOpenGLFunctions::BlendEquation ? "is" : "is NOT") << "available.";
	qDebug() << " - glBlendEquationSeparate()" << (oglFeatures&QOpenGLFunctions::BlendEquationSeparate ? "is" : "is NOT") << "available.";
	qDebug() << " - glBlendFuncSeparate()" << (oglFeatures&QOpenGLFunctions::BlendFuncSeparate ? "is" : "is NOT") << "available.";
	qDebug() << " - Blend subtract mode" << (oglFeatures&QOpenGLFunctions::BlendSubtract ? "is" : "is NOT") << "available.";
	qDebug() << " - Compressed texture functions" << (oglFeatures&QOpenGLFunctions::CompressedTextures ? "are" : "are NOT") << "available.";
	qDebug() << " - glSampleCoverage() function" << (oglFeatures&QOpenGLFunctions::Multisample ? "is" : "is NOT") << "available.";
	qDebug() << " - Separate stencil functions" << (oglFeatures&QOpenGLFunctions::StencilSeparate ? "are" : "are NOT") << "available.";
	qDebug() << " - Non power of two textures" << (oglFeatures&QOpenGLFunctions::NPOTTextures ? "are" : "are NOT") << "available.";
	qDebug() << " - Non power of two textures" << (oglFeatures&QOpenGLFunctions::NPOTTextureRepeat ? "can" : "CANNOT") << "use GL_REPEAT as wrap parameter.";
	qDebug() << " - The fixed function pipeline" << (oglFeatures&QOpenGLFunctions::FixedFunctionPipeline ? "is" : "is NOT") << "available.";

	qDebug() << "OpenGL shader capabilities and details:";
	qDebug() << " - Vertex Shader:" << (QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Vertex, context) ? "YES" : "NO");
	qDebug() << " - Fragment Shader:" << (QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Fragment, context) ? "YES" : "NO");
	qDebug() << " - Geometry Shader:" << (QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry, context) ? "YES" : "NO");
	qDebug() << " - TessellationControl Shader:" << (QOpenGLShader::hasOpenGLShaders(QOpenGLShader::TessellationControl, context) ? "YES" : "NO");
	qDebug() << " - TessellationEvaluation Shader:" << (QOpenGLShader::hasOpenGLShaders(QOpenGLShader::TessellationEvaluation, context) ? "YES" : "NO");
	qDebug() << " - Compute Shader:" << (QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Compute, context) ? "YES" : "NO");

	// GZ: List available extensions. Not sure if this is in any way useful?
	QSet<QByteArray> extensionSet=context->extensions();
	qDebug() << "We have" << extensionSet.count() << "OpenGL extensions:";
	QMap<QString, QString> extensionMap;
	QSetIterator<QByteArray> iter(extensionSet);
	while (iter.hasNext())
	{
		if (!iter.peekNext().isEmpty()) // Don't insert empty lines
			extensionMap.insert(QString(iter.peekNext()), QString(iter.peekNext()));
		iter.next();
	}
	QMapIterator<QString, QString> iter2(extensionMap);
	while (iter2.hasNext())
		qDebug() << " -" << iter2.next().key();

	QFunctionPointer programParameterPtr =context->getProcAddress("glProgramParameteri");
	if (programParameterPtr == 0)
		qDebug() << "glProgramParameteri cannot be resolved here. BAD!";
	//else
	//	qDebug() << "glProgramParameteri can be resolved. GOOD!";
	programParameterPtr =context->getProcAddress("glProgramParameteriEXT");
	if (programParameterPtr == 0)
		qDebug() << "glProgramParameteriEXT cannot be resolved here. BAD!";
	//else
	//	qDebug() << "glProgramParameteriEXT can be resolved here. GOOD!";

}


void StelMainView::deinit()
{
	deinitGL();
	delete stelApp;
	stelApp = NULL;
}

// Update the translated title
void StelMainView::initTitleI18n()
{
	QString appNameI18n = q_("Stellarium %1").arg(StelUtils::getApplicationVersion());
	setWindowTitle(appNameI18n);
}

void StelMainView::setFullScreen(bool b)
{
	if (b)
		showFullScreen();
	else
		showNormal();
}

void StelMainView::updateScene() {
	// For some reason the skyItem is not updated when the night mode shader is on.
	// To fix this we manually do it here.
	skyItem->update();
	scene()->update();
}

void StelMainView::thereWasAnEvent()
{
	lastEventTimeSec = StelApp::getTotalRunTime();
}

void StelMainView::maxFpsSceneUpdate()
{
	updateScene();
	flagMaxFpsUpdatePending = false;
}

void StelMainView::drawBackground(QPainter*, const QRectF&)
{
	const double now = StelApp::getTotalRunTime();
	const double JD_SECOND=0.000011574074074074074074;

	// Determines when the next display will need to be triggered
	// The current policy is that after an event, the FPS is maximum for 2.5 seconds
	// after that, it switches back to the default minfps value to save power.
	// The fps is also kept to max if the timerate is higher than normal speed.
	const float timeRate = StelApp::getInstance().getCore()->getTimeRate();
	const bool needMaxFps = (now - lastEventTimeSec < 2.5) || fabs(timeRate) > JD_SECOND;
	if (needMaxFps)
	{
		if (!flagMaxFpsUpdatePending)
		{
			double duration = 1./getMaxFps();
			int dur = (int)(duration*1000);

			if (minFpsTimer!=NULL)
			{
				disconnect(minFpsTimer, SIGNAL(timeout()), 0, 0);
				delete minFpsTimer;
				minFpsTimer = NULL;
			}
			flagMaxFpsUpdatePending = true;
			QTimer::singleShot(dur<5 ? 5 : dur, this, SLOT(maxFpsSceneUpdate()));
		}
	} else if (minFpsTimer == NULL) {
		// Restart the minfps timer
		minFpsChanged();
	}

	// Manage cursor timeout
	if (cursorTimeout>0.f && (now-lastEventTimeSec>cursorTimeout) && flagCursorTimeout)
	{
		if (QGuiApplication::overrideCursor()==0)
			QGuiApplication::setOverrideCursor(Qt::BlankCursor);
	}
	else
	{
		if (QGuiApplication::overrideCursor()!=0)
			QGuiApplication::restoreOverrideCursor();
	}
}

void StelMainView::startMainLoop()
{
	// Set a timer refreshing for every minfps frames
	minFpsChanged();
}

void StelMainView::minFpsChanged()
{
	if (minFpsTimer!=NULL)
	{
		disconnect(minFpsTimer, SIGNAL(timeout()), 0, 0);
		delete minFpsTimer;
		minFpsTimer = NULL;
	}

	minFpsTimer = new QTimer(this);
	connect(minFpsTimer, SIGNAL(timeout()), this, SLOT(updateScene()));

	minFpsTimer->start((int)(1./getMinFps()*1000.));
}



void StelMainView::mouseMoveEvent(QMouseEvent* event)
{
	if (event->buttons())
		thereWasAnEvent(); // Refresh screen ASAP
	QDeclarativeView::mouseMoveEvent(event);
}

void StelMainView::mousePressEvent(QMouseEvent* event)
{
	thereWasAnEvent(); // Refresh screen ASAP
	QDeclarativeView::mousePressEvent(event);
}

void StelMainView::mouseReleaseEvent(QMouseEvent* event)
{
	thereWasAnEvent(); // Refresh screen ASAP
	QDeclarativeView::mouseReleaseEvent(event);
}

void StelMainView::wheelEvent(QWheelEvent* event)
{
	thereWasAnEvent(); // Refresh screen ASAP
	QDeclarativeView::wheelEvent(event);
}

void StelMainView::moveEvent(QMoveEvent * event)
{
	Q_UNUSED(event);

	// We use the glWidget instead of the even, as we want the screen that shows most of the widget.
	StelApp::getInstance().setDevicePixelsPerPixel(glWidget->windowHandle()->devicePixelRatio());
}

void StelMainView::closeEvent(QCloseEvent* event)
{
	Q_UNUSED(event);
	StelApp::getInstance().quit();
}

void StelMainView::keyPressEvent(QKeyEvent* event)
{
	thereWasAnEvent(); // Refresh screen ASAP
	// Try to trigger a gobal shortcut.
	StelActionMgr* actionMgr = StelApp::getInstance().getStelActionManager();
	if (actionMgr->pushKey(event->key() + event->modifiers(), true)) {
		event->setAccepted(true);
		return;
	}
	QDeclarativeView::keyPressEvent(event);
}

void StelMainView::keyReleaseEvent(QKeyEvent* event)
{
	thereWasAnEvent(); // Refresh screen ASAP
	QDeclarativeView::keyReleaseEvent(event);
}


//! Delete openGL textures (to call before the GLContext disappears)
void StelMainView::deinitGL()
{
	StelApp::getInstance().deinit();
	delete gui;
	gui = NULL;
}

void StelMainView::saveScreenShot(const QString& filePrefix, const QString& saveDir)
{
	screenShotPrefix = filePrefix;
	screenShotDir = saveDir;
	emit(screenshotRequested());
}

void StelMainView::doScreenshot(void)
{
	QFileInfo shotDir;
	QImage im = glWidget->grabFrameBuffer();
	if (flagInvertScreenShotColors)
		im.invertPixels();

	if (screenShotDir == "")
		shotDir = QFileInfo(StelFileMgr::getScreenshotDir());
	else
		shotDir = QFileInfo(screenShotDir);

	if (!shotDir.isDir())
	{
		qWarning() << "ERROR requested screenshot directory is not a directory: " << QDir::toNativeSeparators(shotDir.filePath());
		return;
	}
	else if (!shotDir.isWritable())
	{
		qWarning() << "ERROR requested screenshot directory is not writable: " << QDir::toNativeSeparators(shotDir.filePath());
		return;
	}

	QFileInfo shotPath;
	for (int j=0; j<100000; ++j)
	{
		shotPath = QFileInfo(shotDir.filePath() + "/" + screenShotPrefix + QString("%1").arg(j, 3, 10, QLatin1Char('0')) + ".png");
		if (!shotPath.exists())
			break;
	}

	qDebug() << "INFO Saving screenshot in file: " << QDir::toNativeSeparators(shotPath.filePath());
	if (!im.save(shotPath.filePath())) {
		qWarning() << "WARNING failed to write screenshot to: " << QDir::toNativeSeparators(shotPath.filePath());
	}
}
