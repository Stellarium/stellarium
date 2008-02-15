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

#include <QDebug>

#include "Dialog.hpp"

void BarFrame::mousePressEvent(QMouseEvent *event)
{
	mouse_pos = event->pos();
	moving = true;
}

void BarFrame::mouseReleaseEvent(QMouseEvent *event)
{
	moving = false;
}

void BarFrame::mouseMoveEvent(QMouseEvent *event)
{
	if (!moving) return;
	QPoint dpos = event->pos() - mouse_pos;
	QWidget* p = dynamic_cast<QWidget*>(QFrame::parent());
	p->move(p->pos() + dpos);
}

void ResizeFrame::mouseMoveEvent(QMouseEvent *event)
{
	QPoint dpos = event->pos() - mouse_pos;
	QWidget* p = dynamic_cast<QWidget*>(QFrame::parent()->parent());
	p->setUpdatesEnabled(false);
	p->resize(p->size().width() + dpos.x(), p->size().height() + dpos.y());
	p->setUpdatesEnabled(true);
}
