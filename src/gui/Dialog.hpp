/*
 * Stellarium
 * Copyright (C) 2008 Guillaume Chereau
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


#ifndef _DIALOG_HPP_
#define _DIALOG_HPP_

#include <QWidget>
#include <QFrame>
#include <QMouseEvent>

//! \class BarFrame
//! A title bar control used in windows derived from StelDialog.
//! 
//! As window classes derived from StelDialog are basic QWidgets, they have no
//! title bar. A BarFrame control needs to be used in each window's design
//! to allow the user to move them.
//! 
//! Typically, the frame should contain a centered label displaying the window's
//! title and a button for closing the window (connected to the 
//! StelDialog::close() slot).
//! 
//! To use the default Stellarium style for title bars, the BarFrame object of a
//! given window should be named "TitleBar". See the normalStyle.css file for
//! the style sheet description.
class BarFrame : public QFrame
{
Q_OBJECT
public:
	QPoint mousePos;
  
	BarFrame(QWidget* parent) : QFrame(parent), moving(false) {}
  
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
protected:
	bool moving;
};

class ResizeFrame : public QFrame
{
Q_OBJECT
public:
	QPoint mousePos;
  
	ResizeFrame(QWidget* parent) : QFrame(parent) {}
  
	virtual void mousePressEvent(QMouseEvent *event) {
		mousePos = event->pos();
	}
	virtual void mouseMoveEvent(QMouseEvent *event);
};


#endif // _DIALOG_HPP_
