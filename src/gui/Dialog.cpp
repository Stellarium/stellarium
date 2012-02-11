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

#include <QDebug>

#include "Dialog.hpp"
#include "StelMainWindow.hpp"

void BarFrame::mousePressEvent(QMouseEvent *event)
{
	mousePos = event->pos();
	moving = true;
}

void BarFrame::mouseReleaseEvent(QMouseEvent *)
{
	moving = false;
}

void BarFrame::mouseMoveEvent(QMouseEvent *event)
{
	if (!moving) return;
	QPoint dpos = event->pos() - mousePos;
	QWidget* p = dynamic_cast<QWidget*>(QFrame::parent());
	QPoint targetPos = p->pos() + dpos;
	
	// Prevent the title bar from being dragged to an unreachable position.
	StelMainWindow& mainWindow = StelMainWindow::getInstance();
	int leftBoundX = 10 - width();
	int rightBoundX = mainWindow.width() - 10;
	if (targetPos.x() < leftBoundX)
		targetPos.setX(leftBoundX);
	else if (targetPos.x() > rightBoundX)
		targetPos.setX(rightBoundX);
	
	int lowerBoundY = mainWindow.height() - height();
	if (targetPos.y() < 0)
		targetPos.setY(0);
	else if (targetPos.y() > lowerBoundY)
		targetPos.setY(lowerBoundY);
	
	p->move(targetPos);
}

void ResizeFrame::mouseMoveEvent(QMouseEvent *event)
{
	QPoint dpos = event->pos() - mousePos;
	QWidget* p = dynamic_cast<QWidget*>(QFrame::parent()->parent());
	int w = p->size().width();
	int h = p->size().height();
	int minw;
	int minh;

	if (p->minimumSizeHint().isValid())
	{
		minw = p->minimumSizeHint().width();
		minh = p->minimumSizeHint().height();
	}
	else
	{
		minw = p->minimumWidth() > 0 ? p->minimumWidth() : 24;
		minh = p->minimumHeight() > 0 ? p->minimumHeight() : 24;
	}

	// The minimum size will only be enforced if the widget is being
	// shrunk, and its size is larger than its minimum size. (If, for some
	// reason, the widget's size is already *smaller* than its minimum
	// size, and the user is actually trying to *shrink* it, then it would
	// be rather odd to *enlarge* the widget to its minimum size.)
	if (w + dpos.x() >= minw)
		w += dpos.x();
	else if (w > minw && dpos.x() < 0)
		w = minw;
	if (h + dpos.y() >= minh)
		h += dpos.y();
	else if (h > minh && dpos.y() < 0)
		h = minh;

	p->setUpdatesEnabled(false);
	p->resize(w, h);
	p->setUpdatesEnabled(true);
}
