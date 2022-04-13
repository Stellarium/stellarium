/*
 * Stellarium OnlineQueries Plug-in
 *
 * Copyright (C) 2020-21 Georg Zotti
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

#include "StelWebEngineView.hpp"

#ifdef WITH_QTWEBENGINE

#include <QMouseEvent>
#include <QApplication>

StelWebEngineView::StelWebEngineView(QWidget *parent): QWebEngineView(parent)
{
    QApplication::instance()->installEventFilter(this);
    setContextMenuPolicy(Qt::NoContextMenu);
}

bool StelWebEngineView::eventFilter(QObject *object, QEvent *event) {
    if (object->parent() == this && event->type() == QEvent::MouseButtonRelease) {
	mouseReleaseEvent(static_cast<QMouseEvent *>(event));
    }

    return false;
}

void StelWebEngineView::mouseReleaseEvent(QMouseEvent *e)
{
	switch (e->button())
	{
		case Qt::BackButton:
		    qDebug() << "Back pressed";
		    back();
		    break;
		case Qt::ForwardButton:
		    qDebug() << "Forward pressed";
		    forward();
		    break;
		default:
		    QWidget::mouseReleaseEvent(e);
	}
}
#else
StelWebEngineView::StelWebEngineView(QWidget *parent): QTextBrowser(parent)
{
}
#endif
