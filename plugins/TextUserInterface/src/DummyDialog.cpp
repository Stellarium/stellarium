/*
 * Stellarium
 * Copyright (C) 2009 Matthew Gates
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


#include "DummyDialog.hpp"
#include "StelMainView.hpp"

#include <QDebug>
#include <QDialog>
#include <QGraphicsProxyWidget>
#include <QStyleOptionGraphicsItem>
#include <QLineEdit>

// this is adapted from DummyDialog.cpp
class CustomProxy : public QGraphicsProxyWidget
{
	public:
		CustomProxy(QGraphicsItem *parent = Q_NULLPTR, Qt::WindowFlags wFlags = Qt::WindowFlags()) : QGraphicsProxyWidget(parent, wFlags)
		{
			setFocusPolicy(Qt::StrongFocus);
		}
		//! Reimplement this method to add windows decorations. Currently there are invisible 2 px decorations
		void paintWindowFrame(QPainter*, const QStyleOptionGraphicsItem *, QWidget *)
		{
		}

	protected:
		virtual bool event(QEvent* event)
		{
			if (event->type()==QEvent::WindowDeactivate)
			{
				widget()->setWindowOpacity(0.4);
			}
			if (event->type()==QEvent::WindowActivate)
			{
				widget()->setWindowOpacity(0.9);
			}
			return QGraphicsProxyWidget::event(event);
		}
		
		// Avoid blocking the program when hovering over an inactive window
		virtual bool sceneEvent(QEvent* event)
		{
			if (!(isActiveWindow() || event->type()==QEvent::WindowActivate || event->type()==QEvent::GraphicsSceneMousePress))
			{
				event->setAccepted(false);
				return false;
			}
			return QGraphicsProxyWidget::sceneEvent(event);
		}
};

DummyDialog::DummyDialog(StelModule* eventHandler)
	: proxy(Q_NULLPTR)
	, dialog(Q_NULLPTR)
	, evtHandler(eventHandler)
{
}

DummyDialog::~DummyDialog()
{
}


void DummyDialog::close()
{
	emit visibleChanged(false);
	StelMainView::getInstance().scene()->setActiveWindow(Q_NULLPTR);
}

void DummyDialog::setVisible(bool v)
{
	if (v) 
	{
		if (dialog)
		{
			dialog->show();
			StelMainView::getInstance().scene()->setActiveWindow(proxy);
			proxy->setFocus();
			return;
		}
		dialog = new QDialog(Q_NULLPTR);
		connect(dialog, SIGNAL(rejected()), this, SLOT(close()));
		createDialogContent();
		
		proxy = new CustomProxy(Q_NULLPTR, Qt::Tool);
		proxy->setWidget(dialog);
		StelMainView::getInstance().scene()->addItem(proxy);
		// Invisible frame around the window to make resizing easier
		// (this also changes the bounding rectangle size)
		proxy->setWindowFrameMargins(7,0,7,7);
		proxy->setCacheMode(QGraphicsItem::DeviceCoordinateCache); 
		proxy->setZValue(100);
		StelMainView::getInstance().scene()->setActiveWindow(proxy);
		proxy->setFocus();
	}
	else
	{
		dialog->hide();
		emit visibleChanged(false);
		//proxy->clearFocus();
		StelMainView::getInstance().scene()->setActiveWindow(Q_NULLPTR);
	}
}

bool DummyDialog::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		evtHandler->handleKeys(keyEvent);
		return keyEvent->isAccepted();
	} else {
		// standard event processing
		return QObject::eventFilter(obj, event);
	}
}

void DummyDialog::createDialogContent()
{
	QLineEdit* le = new QLineEdit(dialog);
	le->show();
	le->setFocus();
	le->installEventFilter(this);
	dialog->resize(20,20);
	dialog->move(-30,-30);
}

