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

#include <iostream>
#include "StelMainWindow.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "Projector.hpp"
#include "InitParser.hpp"
#include "fixx11h.h"

using namespace std;

#include <QtGui/QImage>
#include <QtOpenGL>
#include <QStringList>
#include <QCoreApplication>
#include <QRegExp>
#include <QDate>
#include <QTime>
#include <QDateTime>

StelMainWindow::StelMainWindow()
{
	setWindowTitle(StelApp::getApplicationName());
	setObjectName("stellariumMainWin");
	setMinimumSize(400,400);
}

StelMainWindow::~StelMainWindow()
{
}

// Get the width of the openGL screen
int StelMainWindow::getScreenW() const
{
	return findChild<GLWidget*>("stellariumOpenGLWidget")->width();
}
	
// Get the height of the openGL screen
int StelMainWindow::getScreenH() const
{
	return findChild<GLWidget*>("stellariumOpenGLWidget")->height();
}

// Set mouse cursor display
void StelMainWindow::showCursor(bool b)
{
	findChild<GLWidget*>("stellariumOpenGLWidget")->setCursor(b ? Qt::ArrowCursor : Qt::BlankCursor);
}

//! Swap GL buffer, should be called only for special condition
void StelMainWindow::swapGLBuffers()
{
	findChild<GLWidget*>("stellariumOpenGLWidget")->swapBuffers();
}


void StelMainWindow::saveScreenShot(const QString& filePrefix, const QString& saveDir) const
{
	QString shotDir;
	QImage im = findChild<GLWidget*>("stellariumOpenGLWidget")->grabFrameBuffer();

	if (saveDir == "")
	{
		try
		{
			shotDir = StelApp::getInstance().getFileMgr().getScreenshotDir();
			if (!StelApp::getInstance().getFileMgr().isWritable(shotDir))
			{
				cerr << "ERROR StelAppSdl::saveScreenShot: screenshot directory is not writable: " << qPrintable(shotDir) << endl;
				return;
			}
		}
		catch(exception& e)
		{
			cerr << "ERROR StelAppSdl::saveScreenShot: could not determine screenshot directory: " << e.what() << endl;
			return;
		}
	}
	else
	{
		shotDir = saveDir;
	}

	QString shotPath;
	for(int j=0; j<1000; ++j)
	{
		shotPath = shotDir+"/"+filePrefix + QString("%1").arg(j, 3, 10, QLatin1Char('0')) + ".bmp";
		if (!StelApp::getInstance().getFileMgr().exists(shotPath))
			break;
	}
	// TODO - if no more filenames available, don't just overwrite the last one
	// we should at least warn the user, perhaps prompt her, "do you want to overwrite?"
	
	cout << "Saving screenshot in file: " << qPrintable(shotPath) << endl;
	im.save(shotPath);
}

void StelMainWindow::setResizable(bool resizable)
{
	if (resizable)
	{
		setMaximumSize(10000,10000);
	}
	else
	{
		setFixedSize(size());
	}
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


bool StelMainWindow::getFullScreen() const
{
	return isFullScreen();
}

//////////////////////////////////////////////////////////////////////////
// GLWidget methods
//////////////////////////////////////////////////////////////////////////


GLWidget::GLWidget(QWidget *parent) : QGLWidget(QGLFormat::defaultFormat(), parent)
{
	setObjectName("stellariumOpenGLWidget");
	lastEventTimeSec = StelApp::getInstance().getTotalRunTime();
	previousTime = lastEventTimeSec;
	setFocusPolicy(Qt::ClickFocus);
	setMouseTracking(true);
	// make openGL context current
	makeCurrent();
	setAutoBufferSwap(false);
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
	StelApp::getInstance().glWindowHasBeenResized(w, h);
}

void GLWidget::paintGL()
{
	const double now = StelApp::getInstance().getTotalRunTime();
	double dt = now-previousTime;
	previousTime = now;
	
	if (dt<0)	// This fix the star scale bug!!
		return;
	StelApp::getInstance().update(dt);
	StelApp::getInstance().draw();
	swapBuffers();
}

void GLWidget::timerEvent(QTimerEvent *)
{
	update();
	killTimer(timerId);
	double duration = 1./StelApp::getInstance().minfps;
	if (StelApp::getInstance().getTotalRunTime()-lastEventTimeSec<2.5)
		duration = 1./StelApp::getInstance().maxfps;
	timerId = startTimer((int)(duration*1000));
}

void GLWidget::thereWasAnEvent()
{
	// Refresh screen ASAP
	killTimer(timerId);
	timerId = startTimer(0);
	lastEventTimeSec = StelApp::getInstance().getTotalRunTime();
}

StelMod qtModToStelMod(Qt::KeyboardModifiers m)
{
	StelMod out = StelMod_NONE;
	if (m.testFlag(Qt::ShiftModifier))
	{
		out = (StelMod)((int)out|(int)StelMod_SHIFT);
	}
	if (m.testFlag(Qt::ControlModifier))
	{
		out = (StelMod)((int)out|(int)COMPATIBLE_StelMod_CTRL);
	}
	if (m.testFlag(Qt::AltModifier))
	{
		out = (StelMod)((int)out|(int)StelMod_ALT);
	}
	if (m.testFlag(Qt::MetaModifier))
	{
		out = (StelMod)((int)out|(int)StelMod_META);
	}

	return out;
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
	
	StelApp::getInstance().handleClick(event->x(), event->y(), button, state, qtModToStelMod(event->modifiers()));
	
	// Refresh screen ASAP
	thereWasAnEvent();
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
	
	StelApp::getInstance().handleClick(event->x(), event->y(), button, state, qtModToStelMod(event->modifiers()));
	
	// Refresh screen ASAP
	thereWasAnEvent();
}

void GLWidget::mouseMoveEvent(QMouseEvent* event)
{
	StelApp::getInstance().handleMove(event->x(), event->y(), qtModToStelMod(event->modifiers()));
	
	// Refresh screen ASAP
	thereWasAnEvent();
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
		mod = COMPATIBLE_StelMod_CTRL;

	StelApp::getInstance().handleClick(event->x(), event->y(), button, Stel_MOUSEBUTTONDOWN,  mod);
	StelApp::getInstance().handleClick(event->x(), event->y(), button, Stel_MOUSEBUTTONUP,  mod);
	
	// Refresh screen ASAP
	thereWasAnEvent();
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
		StelApp::getInstance().getMainWindow()->toggleFullScreen();
	}
	StelApp::getInstance().handleKeys(qtKeyToStelKey((Qt::Key)event->key()), qtModToStelMod(event->modifiers()), event->text().utf16()[0], Stel_KEYDOWN);
	// Refresh screen ASAP
	findChild<GLWidget*>("stellariumOpenGLWidget")->thereWasAnEvent();
}

void StelMainWindow::keyReleaseEvent(QKeyEvent* event)
{
	StelApp::getInstance().handleKeys(qtKeyToStelKey((Qt::Key)event->key()), qtModToStelMod(event->modifiers()), event->text().utf16()[0], Stel_KEYUP);
	// Refresh screen ASAP
	findChild<GLWidget*>("stellariumOpenGLWidget")->thereWasAnEvent();
}
