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

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "Projector.hpp"
#include "fixx11h.h"
#include "StelGLWidget.hpp"
#include "ViewportDistorter.hpp"
#include <QtOpenGL>
#include "StelModuleMgr.hpp"

#include "stel_ui.h"
#include "stel_command_interface.h"
#include "image_mgr.h"
#include "script_mgr.h"

// Initialize static variables
StelGLWidget* StelGLWidget::singleton = NULL;

StelGLWidget::StelGLWidget(QWidget *parent) : QGLWidget(QGLFormat::defaultFormat(), parent), ui(NULL)
{
	setObjectName("StelGLWidget");
	
	// Can't create 2 StelGLWidget instances
	assert(!singleton);
	singleton = this;
	
	distorter = ViewportDistorter::create("none",width(),height(),NULL);
	lastEventTimeSec = StelApp::getInstance().getTotalRunTime();
	previousTime = lastEventTimeSec;
	setFocusPolicy(Qt::ClickFocus);
	setMouseTracking(true);
	// make openGL context current
	makeCurrent();
	setAutoBufferSwap(false);
	mainTimer = new QTimer(this);
	connect(mainTimer, SIGNAL(timeout()), this, SLOT(recompute()));
}

StelGLWidget::~StelGLWidget()
{
	if (distorter)
	{
		delete distorter;
		distorter=NULL;
	}
}

void StelGLWidget::init()
{
	setViewPortDistorterType(StelApp::getInstance().getSettings()->value("video/distorter","none").toString());
	
	
	// Everything below is going to disapear
	StelCommandInterface* commander = new StelCommandInterface(StelApp::getInstance().getCore(), &StelApp::getInstance());
	StelApp::getInstance().getModuleMgr().registerModule(commander);
	
	ScriptMgr* scripts = new ScriptMgr(commander);
	scripts->init();
	StelApp::getInstance().getModuleMgr().registerModule(scripts);
		
	ui = new StelUI(StelApp::getInstance().getCore(), &StelApp::getInstance());
	ui->init();
	StelApp::getInstance().getModuleMgr().registerModule(ui, true);
	
	ImageMgr* script_images = new ImageMgr();
	script_images->init();
	StelApp::getInstance().getModuleMgr().registerModule(script_images);
}

void StelGLWidget::initializeGL()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	swapBuffers();
	glClear(GL_COLOR_BUFFER_BIT);
}

void StelGLWidget::resizeGL(int w, int h)
{
	//cerr << "StelGLWidget::resizeGL(" << w << "x" << h << ")" << endl;
	// no resizing allowed in distortion mode
	if (!distorter || distorter && distorter->getType() == "none")
	{
		StelApp::getInstance().glWindowHasBeenResized(w, h);
	}
}

void StelGLWidget::paintGL()
{
	const double now = StelApp::getInstance().getTotalRunTime();
	double dt = now-previousTime;
	previousTime = now;
	
	if (dt<0)	// This fix the star scale bug!!
		return;
	StelApp::getInstance().update(dt);
	
	distorter->prepare();
	StelApp::getInstance().draw();
	distorter->distort();
	// Draw the Graphical ui
	if (ui)
		ui->drawGui();
	
	swapBuffers();
}

void StelGLWidget::recompute()
{
	update();
	double duration = 1./StelApp::getInstance().minfps;
	if (StelApp::getInstance().getTotalRunTime()-lastEventTimeSec<2.5)
		duration = 1./StelApp::getInstance().maxfps;
	mainTimer->start((int)(duration*1000));
}

void StelGLWidget::thereWasAnEvent()
{
	// Refresh screen ASAP
	mainTimer->start(0);
	lastEventTimeSec = StelApp::getInstance().getTotalRunTime();
}

void StelGLWidget::setViewPortDistorterType(const QString &type)
{
	if (type != getViewPortDistorterType())
	{
		if (type == "none")
		{
			setMaximumSize(10000,10000);
		}
		else
		{
			setFixedSize(size());
		}
	}
	if (distorter)
	{
		delete distorter;
		distorter = NULL;
	}
	distorter = ViewportDistorter::create(type.toStdString(),width(),height(),StelApp::getInstance().getCore()->getProjection());
}

QString StelGLWidget::getViewPortDistorterType() const
{
	if (distorter)
		return distorter->getType().c_str();
	return "none";
}

// Set mouse cursor display
void StelGLWidget::showCursor(bool b)
{
	setCursor(b ? Qt::ArrowCursor : Qt::BlankCursor);
}

// Start the main drawing loop
void StelGLWidget::startDrawingLoop()
{
	mainTimer->start(10);
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

void StelGLWidget::mousePressEvent(QMouseEvent* event)
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
	
	int x = event->x();
	int y = event->y();
	
	if (ui->handleClick(x, y, button, state, qtModToStelMod(event->modifiers())))
		return;
	
	y = height() - 1 - y;
	distorter->distortXY(x,y);
	
	QMouseEvent newEvent(event->type(), QPoint(x,y), event->button(), event->buttons(), event->modifiers());
	StelApp::getInstance().handleClick(&newEvent);
	
	// Refresh screen ASAP
	thereWasAnEvent();
}

void StelGLWidget::mouseReleaseEvent(QMouseEvent* event)
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
	
	int x = event->x();
	int y = event->y();
	
	if (ui->handleClick(x, y, button, state, qtModToStelMod(event->modifiers())))
		return;
	
	y = height() - 1 - y;
	distorter->distortXY(x,y);
	QMouseEvent newEvent(event->type(), QPoint(x,y), event->button(), event->buttons(), event->modifiers());
	StelApp::getInstance().handleClick(&newEvent);
	
	// Refresh screen ASAP
	thereWasAnEvent();
}

void StelGLWidget::mouseMoveEvent(QMouseEvent* event)
{
	int x = event->x();
	int y = event->y();
	const int ui_x = x;
	const int ui_y = y;
	y = height() - 1 - y;
	distorter->distortXY(x,y);
	QMouseEvent newEvent(event->type(), QPoint(x,y), event->button(), event->buttons(), event->modifiers());
	StelApp::getInstance().handleMove(&newEvent);
	
	ui->handleMouseMoves(ui_x,ui_y, qtModToStelMod(event->modifiers()));
	
	// Refresh screen ASAP
	thereWasAnEvent();
}

void StelGLWidget::wheelEvent(QWheelEvent* event)
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
	
	int x = event->x();
	int y = event->y();
	
	if (ui->handleClick(x, y, button, Stel_MOUSEBUTTONDOWN, qtModToStelMod(event->modifiers())))
		return;
	
	y = height() - 1 - y;
	distorter->distortXY(x,y);
	QWheelEvent newEvent(QPoint(x,y), event->delta(), event->buttons(), event->modifiers(), event->orientation());
	StelApp::getInstance().handleWheel(&newEvent);
	
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

void StelGLWidget::keyPressEvent(QKeyEvent* event)
{
	if (ui->handle_keys_tui(qtKeyToStelKey((Qt::Key)event->key()), Stel_KEYDOWN)) return;
	if (ui->handle_keysGUI(qtKeyToStelKey((Qt::Key)event->key()), qtModToStelMod(event->modifiers()), event->text().utf16()[0], Stel_KEYDOWN)) return;
	
	StelApp::getInstance().handleKeys(event);
	
	// Non widget key handling
	ui->handle_keys(qtKeyToStelKey((Qt::Key)event->key()), qtModToStelMod(event->modifiers()), event->text().utf16()[0], Stel_KEYDOWN);
	
	// Refresh screen ASAP
	thereWasAnEvent();
}

void StelGLWidget::keyReleaseEvent(QKeyEvent* event)
{
	if (ui->handle_keys_tui(qtKeyToStelKey((Qt::Key)event->key()), Stel_KEYUP)) return;
	if (ui->handle_keysGUI(qtKeyToStelKey((Qt::Key)event->key()), qtModToStelMod(event->modifiers()), event->text().utf16()[0], Stel_KEYUP)) return;
	
	StelApp::getInstance().handleKeys(event);
	
	// Non widget key handling
	ui->handle_keys(qtKeyToStelKey((Qt::Key)event->key()), qtModToStelMod(event->modifiers()), event->text().utf16()[0], Stel_KEYUP);
		
	// Refresh screen ASAP
	thereWasAnEvent();
}
