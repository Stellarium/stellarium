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


#include "Dialog.hpp"

#include <iostream>

DialogFrame::DialogFrame(QWidget *parent) : 
      QFrame(parent), border(4),
      followLeft(false), followRight(false),
      followTop(false), followBottom(false)
{
  setMouseTracking(true);
  setMinimumSize(100,100);
}

void DialogFrame::mouseMoveEvent(QMouseEvent *event)
{
  if (onLeft(event->pos()) || onRight(event->pos()))
  {
    setCursor(Qt::SizeHorCursor);
  }
  else if (onTop(event->pos()) || onBottom(event->pos()))
  {
    setCursor(Qt::SizeVerCursor);
  }
  else {unsetCursor();}
  
  QPoint dpos = event->pos() - mouse_pos;
  if (followLeft)
  {
    QSize old_size = size();
    resize(size().width() - dpos.x(), size().height());
    move(pos().x() + (old_size - size()).width(), pos().y());
  }
  if (followRight)
  {
    resize(size().width() + dpos.x(), size().height());
    mouse_pos += QPoint(dpos.x(), 0);
  }
  if (followTop)
  {
    QSize old_size = size();
    resize(size().width(), size().height() - dpos.y());
    move(pos().x(), pos().y() + (old_size - size()).height());
  }
  if (followBottom)
  {
    resize(size().width(), size().height() + dpos.y());
    mouse_pos += QPoint(0, dpos.y());
  }
}

void DialogFrame::mousePressEvent(QMouseEvent *event)
{
  followLeft = onLeft(event->pos());
  followRight = onRight(event->pos());
  followTop = onTop(event->pos());
  followBottom = onBottom(event->pos());
  mouse_pos = event->pos();
}

void DialogFrame::mouseReleaseEvent(QMouseEvent *event)
{
  followLeft = false;
  followRight = false;
  followTop = false;
  followBottom = false;
  
  unsetCursor();
}

void BarFrame::mouseMoveEvent(QMouseEvent *event)
{
  QPoint dpos = event->pos() - mouse_pos;
  QWidget* p = dynamic_cast<QWidget*>(QFrame::parent());
  p->move(p->pos() + dpos);
}
