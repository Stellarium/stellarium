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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#ifndef _DIALOG_HPP_
#define _DIALOG_HPP_

#include <QWidget>
#include <QFrame>
#include <QMouseEvent>


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
