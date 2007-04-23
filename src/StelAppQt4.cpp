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

#ifdef USE_QT4

#include <iostream>

#include "StelAppQt4.hpp"
#include "StelCore.hpp"
#include "Projector.hpp"
#include "fixx11h.h"

using namespace std;

#include <QtGui/QImage>
#include <QtGui/QApplication>
#include <QtOpenGL>
#include <QMainWindow>

class StelMainWindow : public QMainWindow
{
public:
	StelMainWindow(class StelAppQt4* app) : stelApp(app) {;}
	void keyPressEvent(QKeyEvent*);
	void keyReleaseEvent(QKeyEvent*);
private:
	class StelAppQt4* stelApp;
};

class GLWidget : public QGLWidget
{
public:
    GLWidget(QWidget *parent, class StelAppQt4* app);
    ~GLWidget();
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();
    void timerEvent(QTimerEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
	void mouseMoveEvent(QMouseEvent*);
	void wheelEvent(QWheelEvent*);
	QTime qtime;
private:
    int timerId;
    class StelAppQt4* stelApp;
    int previousTime;
};

StelAppQt4::StelAppQt4(const string& configDir, const string& localeDir, const string& dataRootDir, int argc, char **argv) :
	StelApp(configDir, localeDir, dataRootDir)
{
}

StelAppQt4::~StelAppQt4()
{
	deInit();
}


void StelAppQt4::initOpenGL(int w, int h, int bbpMode, bool fullScreen, string iconFile)
{
	mainWindow->setWindowIcon(QIcon(iconFile.c_str()));
	mainWindow->resize(w, h);
	if (fullScreen)
	{
		mainWindow->showFullScreen();
		QDesktopWidget* desktop = QApplication::desktop();
		mainWindow->resize(desktop->screenGeometry(mainWindow).width(),desktop->screenGeometry(mainWindow).height());
	}
}

// Get the width of the openGL screen
int StelAppQt4::getScreenW() const
{
	return winOpenGL->width();
}
	
// Get the height of the openGL screen
int StelAppQt4::getScreenH() const
{
	return winOpenGL->height();
}

string StelAppQt4::getVideoModeList(void) const
{
	return "";
}


// Terminate the application
void StelAppQt4::terminateApplication(void)
{
	exit(0);
}

// Set mouse cursor display
void StelAppQt4::showCursor(bool b)
{;}

// DeInit SDL related stuff
void StelAppQt4::deInit()
{;}

//! Swap GL buffer, should be called only for special condition
void StelAppQt4::swapGLBuffers()
{
	winOpenGL->swapBuffers();
}

//! Return the time since when stellarium is running in second
double StelAppQt4::getTotalRunTime() const
{
	return (double)(winOpenGL->qtime.elapsed())/1000;
}

void StelAppQt4::startMainLoop()
{
	int argc = 0;
	char **argv = NULL;
	QApplication app(argc, argv);
	if (!QGLFormat::hasOpenGL())
	{
		QMessageBox::information(0, "Stellarium", "This system does not support OpenGL.");
	}
	
	StelMainWindow mainWin(this);
	mainWindow = &mainWin;
	mainWindow->setMinimumSize(400,300);
	
	GLWidget openGLWin(&mainWin, this);
	winOpenGL = &openGLWin;
	winOpenGL->setObjectName(QString::fromUtf8("stellariumOpenGLWin"));
	mainWin.setCentralWidget(&openGLWin);
	mainWin.show();
	openGLWin.show();

	StelApp::init();
	// Update GL screen size because the last time it was called, the Projector was not yet properly initialized
	openGLWin.resizeGL(getScreenW(), getScreenH());
	
	app.exec();
}

void StelAppQt4::saveScreenShot() const
{
	cerr << "Broken" << endl;
}

void StelAppQt4::setResizable(bool resizable)
{
	if (resizable)
	{
		mainWindow->setMaximumSize(10000,10000);
	}
	else
	{
		mainWindow->setFixedSize(mainWindow->size());
	}
}

/*************************************************************************
 Alternate fullscreen mode/windowed mode if possible
*************************************************************************/
void StelAppQt4::toggleFullScreen()
{
	// Toggle full screen
	if (!mainWindow->isFullScreen())
	{
		mainWindow->showFullScreen();
	}
	else
	{
		mainWindow->showNormal();
	}
}


bool StelAppQt4::getFullScreen() const
{
	return mainWindow->isFullScreen();
}

GLWidget::GLWidget(QWidget *parent, StelAppQt4* stapp) : QGLWidget(QGLFormat::defaultFormat(), parent), stelApp(stapp)
{
	setFocusPolicy(Qt::ClickFocus);
	setMouseTracking(true);
	// make openGL context current
	makeCurrent();
	setAutoBufferSwap(false);
	timerId = startTimer(10);
	qtime.start();
}

GLWidget::~GLWidget()
{
}

void GLWidget::initializeGL()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	swapBuffers();
	glClear(GL_COLOR_BUFFER_BIT);	
}

void GLWidget::resizeGL(int w, int h)
{
	//cerr << "GLWidget::resizeGL(" << w << "x" << h << ")" << endl;
	stelApp->glWindowHasBeenResized(w, h);
}

void GLWidget::paintGL()
{
	int dt = qtime.elapsed()-previousTime;
	previousTime = qtime.elapsed();
	
	if (dt<0)	// This fix the star scale bug!!
		return;
	stelApp->update(dt);
	stelApp->draw(dt);
	swapBuffers();
}

void GLWidget::timerEvent(QTimerEvent *)
{
	update();
	killTimer(timerId);
	timerId = startTimer(15);
}

StelMod qtModToStelMod(Qt::KeyboardModifiers m)
{
	switch (m)
	{
	case Qt::ShiftModifier : return StelMod_SHIFT;
	case Qt::ControlModifier : return StelMod_CTRL;
	case Qt::AltModifier : return StelMod_ALT;
	case Qt::MetaModifier : return StelMod_META;
	default: return StelMod_NONE;
	}
}

void GLWidget::mousePressEvent(QMouseEvent* event)
{
	Uint8 button = Stel_NOEVENT;
	if (event->button() == Qt::LeftButton)
		button = Stel_BUTTON_LEFT;
	if (event->button() == Qt::RightButton)
		button = Stel_BUTTON_RIGHT;
	if (event->button() == Qt::MidButton)
		button = Stel_BUTTON_MIDDLE;		

	Uint8 state = Stel_NOEVENT;
	if (event->type() == QEvent::MouseButtonPress)
		state = Stel_MOUSEBUTTONDOWN;
	if (event->type() == QEvent::MouseButtonRelease)
		state = Stel_MOUSEBUTTONUP;
	
	stelApp->handleClick(event->x(), event->y(), button, state, qtModToStelMod(event->modifiers()));
}

void GLWidget::mouseReleaseEvent(QMouseEvent* event)
{
	Uint8 button = Stel_NOEVENT;
	if (event->button() == Qt::LeftButton)
		button = Stel_BUTTON_LEFT;
	if (event->button() == Qt::RightButton)
		button = Stel_BUTTON_RIGHT;
	if (event->button() == Qt::MidButton)
		button = Stel_BUTTON_MIDDLE;		

	Uint8 state = Stel_NOEVENT;
	if (event->type() == QEvent::MouseButtonPress)
		state = Stel_MOUSEBUTTONDOWN;
	if (event->type() == QEvent::MouseButtonRelease)
		state = Stel_MOUSEBUTTONUP;
	
	stelApp->handleClick(event->x(), event->y(), button, state, qtModToStelMod(event->modifiers()));
}

void GLWidget::mouseMoveEvent(QMouseEvent* event)
{
	stelApp->handleMove(event->x(), event->y(), qtModToStelMod(event->modifiers()));
}

void GLWidget::wheelEvent(QWheelEvent* event)
{
	Uint8 button;
	if (event->delta()>0)
		button = Stel_BUTTON_WHEELUP;
	else
		button = Stel_BUTTON_WHEELDOWN;
	
	StelMod mod = StelMod_NONE;
	if (event->modifiers() == Qt::ShiftModifier)
		mod = StelMod_SHIFT;
	if (event->modifiers() == Qt::ControlModifier)
		mod = StelMod_CTRL;

	stelApp->handleClick(event->x(), event->y(), button, Stel_MOUSEBUTTONDOWN,  mod);
	stelApp->handleClick(event->x(), event->y(), button, Stel_MOUSEBUTTONUP,  mod);
}

StelKey qtKeyToStelKey(Qt::Key k)
{
	switch (k)
	{
	case Qt::Key_Escape: return StelKey_ESCAPE ; 
	case Qt::Key_Tab : 	return StelKey_TAB ;
	case Qt::Key_Backspace : 	return StelKey_BACKSPACE ;
	case Qt::Key_Return : 	return StelKey_RETURN ;
	//case Qt::Key_Enter : 	return StelKey_ENTER ;
	case Qt::Key_Insert : 	return StelKey_INSERT ;
	case Qt::Key_Delete : 	return StelKey_DELETE ;
	case Qt::Key_Pause : 	return StelKey_PAUSE ;
	case Qt::Key_Print : 	return StelKey_PRINT ;
	case Qt::Key_Clear : 	return StelKey_CLEAR ;
	case Qt::Key_Home : 	return StelKey_HOME ;
	case Qt::Key_End : 	return StelKey_END ;
	case Qt::Key_Left : 	return StelKey_LEFT ;
	case Qt::Key_Up : 	return StelKey_UP ;
	case Qt::Key_Right : 	return StelKey_RIGHT ;
	case Qt::Key_Down : 	return StelKey_DOWN ;
	case Qt::Key_PageUp : 	return StelKey_PAGEUP ;
	case Qt::Key_PageDown : 	return StelKey_PAGEDOWN ;
	case Qt::Key_Shift : 	return StelKey_LSHIFT ;
	case Qt::Key_Control : 	return StelKey_LCTRL ;
	case Qt::Key_Meta : 	return StelKey_LMETA ;
	case Qt::Key_Alt : 	return StelKey_LALT ;
	case Qt::Key_AltGr : 	return StelKey_RALT ;
	case Qt::Key_CapsLock : 	return StelKey_CAPSLOCK ;
	case Qt::Key_NumLock : 	return StelKey_NUMLOCK ;
	case Qt::Key_ScrollLock : 	return StelKey_SCROLLOCK ;
	case Qt::Key_F1 : 	return StelKey_F1 ;
	case Qt::Key_F2 : 	return StelKey_F2 ;
	case Qt::Key_F3 : 	return StelKey_F3 ;
	case Qt::Key_F4 : 	return StelKey_F4 ;
	case Qt::Key_F5 : 	return StelKey_F5 ;
	case Qt::Key_F6 : 	return StelKey_F6 ;
	case Qt::Key_F7 : 	return StelKey_F7 ;
	case Qt::Key_F8 : 	return StelKey_F8 ;
	case Qt::Key_F9 : 	return StelKey_F9 ;
	case Qt::Key_F10 : 	return StelKey_F10 ;
	case Qt::Key_F11 : 	return StelKey_F11 ;
	case Qt::Key_F12 : 	return StelKey_F12 ;
	case Qt::Key_F13 : 	return StelKey_F13 ;
	case Qt::Key_F14 : 	return StelKey_F14 ;
	case Qt::Key_F15 : 	return StelKey_F15 ;
	case Qt::Key_Space : 	return StelKey_SPACE ;
	case Qt::Key_Exclam : 	return StelKey_EXCLAIM ;
	case Qt::Key_QuoteDbl : 	return StelKey_QUOTEDBL ;
	case Qt::Key_NumberSign : 	return StelKey_HASH ;
	case Qt::Key_Dollar : 	return StelKey_DOLLAR ;
	case Qt::Key_Ampersand : 	return StelKey_AMPERSAND ;
	case Qt::Key_Apostrophe : 	return StelKey_QUOTE ;
	case Qt::Key_ParenLeft : 	return StelKey_LEFTPAREN ;
	case Qt::Key_ParenRight : 	return StelKey_RIGHTPAREN ;
	case Qt::Key_Asterisk : 	return StelKey_ASTERISK ;
	case Qt::Key_Plus : 	return StelKey_PLUS ;
	case Qt::Key_Comma : 	return StelKey_COMMA ;
	case Qt::Key_Minus : 	return StelKey_MINUS ;
	case Qt::Key_Period : 	return StelKey_PERIOD ;
	case Qt::Key_Slash : 	return StelKey_SLASH ;
	case Qt::Key_0 : 	return StelKey_0 ;
	case Qt::Key_1 : 	return StelKey_1 ;
	case Qt::Key_2 : 	return StelKey_2 ;
	case Qt::Key_3 : 	return StelKey_3 ;
	case Qt::Key_4 : 	return StelKey_4 ;
	case Qt::Key_5 : 	return StelKey_5 ;
	case Qt::Key_6 : 	return StelKey_6 ;
	case Qt::Key_7 : 	return StelKey_7 ;
	case Qt::Key_8 : 	return StelKey_8 ;
	case Qt::Key_9 : 	return StelKey_9 ;
	case Qt::Key_Colon : 	return StelKey_COLON ;
	case Qt::Key_Semicolon : 	return StelKey_SEMICOLON ;
	case Qt::Key_Less : 	return StelKey_LESS ;
	case Qt::Key_Equal : 	return StelKey_EQUALS ;
	case Qt::Key_Greater : 	return StelKey_GREATER ;
	case Qt::Key_Question : 	return StelKey_QUESTION ;
	case Qt::Key_At : 	return StelKey_AT ;
	case Qt::Key_A : 	return StelKey_a ;
	case Qt::Key_B : 	return StelKey_b ;
	case Qt::Key_C : 	return StelKey_c ;
	case Qt::Key_D : 	return StelKey_d ;
	case Qt::Key_E : 	return StelKey_e ;
	case Qt::Key_F : 	return StelKey_f ;
	case Qt::Key_G : 	return StelKey_g ;
	case Qt::Key_H : 	return StelKey_h ;
	case Qt::Key_I : 	return StelKey_i ;
	case Qt::Key_J : 	return StelKey_j ;
	case Qt::Key_K : 	return StelKey_k ;
	case Qt::Key_L : 	return StelKey_l ;
	case Qt::Key_M : 	return StelKey_m ;
	case Qt::Key_N : 	return StelKey_n ;
	case Qt::Key_O : 	return StelKey_o ;
	case Qt::Key_P : 	return StelKey_p ;
	case Qt::Key_Q : 	return StelKey_q ;
	case Qt::Key_R : 	return StelKey_r ;
	case Qt::Key_S : 	return StelKey_s ;
	case Qt::Key_T : 	return StelKey_t ;
	case Qt::Key_U : 	return StelKey_u ;
	case Qt::Key_V : 	return StelKey_v ;
	case Qt::Key_W : 	return StelKey_w ;
	case Qt::Key_X : 	return StelKey_x ;
	case Qt::Key_Y : 	return StelKey_y ;
	case Qt::Key_Z : 	return StelKey_z ;
	case Qt::Key_BracketLeft : 	return StelKey_LEFTBRACKET ;
	case Qt::Key_Backslash : 	return StelKey_BACKSLASH ;
	case Qt::Key_BracketRight : 	return StelKey_RIGHTBRACKET ;
	case Qt::Key_AsciiCircum : 	return StelKey_CARET ;
	case Qt::Key_Underscore : 	return StelKey_UNDERSCORE ;
	case Qt::Key_QuoteLeft : 	return StelKey_BACKQUOTE ;
	default: return StelKey_UNKNOWN ;
	}
	return StelKey_UNKNOWN; 
}

void StelMainWindow::keyPressEvent(QKeyEvent* event)
{
	if ((Qt::Key)event->key()==Qt::Key_F1)
	{
		stelApp->toggleFullScreen();
	}
	stelApp->handleKeys(qtKeyToStelKey((Qt::Key)event->key()), qtModToStelMod(event->modifiers()), event->text().utf16()[0], Stel_KEYDOWN);
}

void StelMainWindow::keyReleaseEvent(QKeyEvent* event)
{
	stelApp->handleKeys(qtKeyToStelKey((Qt::Key)event->key()), qtModToStelMod(event->modifiers()), event->text().utf16()[0], Stel_KEYUP);
}


#endif
