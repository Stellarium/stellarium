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
#include "StelMainGraphicsView.hpp"

#include <QDebug>
#include <QDialog>
#include <QGraphicsProxyWidget>
#include <QStyleOptionGraphicsItem>
#include <QLineEdit>

// this is adapted from DummyDialog.cpp
class CustomProxy : public QGraphicsProxyWidget
{
	public:
		CustomProxy(QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0) : QGraphicsProxyWidget(parent, wFlags)
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

DummyDialog::DummyDialog(StelModule* eventHandler) : dialog(NULL)
{
	evtHandler = eventHandler;
}

DummyDialog::~DummyDialog()
{
}


void DummyDialog::close()
{
	emit visibleChanged(false);
	StelMainGraphicsView::getInstance().scene()->setActiveWindow(0);
}

void DummyDialog::setVisible(bool v)
{
	if (v) 
	{
		if (dialog)
		{
			dialog->show();
			StelMainGraphicsView::getInstance().scene()->setActiveWindow(proxy);
			proxy->setFocus();
			return;
		}
		dialog = new QDialog(NULL);
		connect(dialog, SIGNAL(rejected()), this, SLOT(close()));
		createDialogContent();
		
		proxy = new CustomProxy(NULL, Qt::Tool);
		proxy->setWidget(dialog);
		StelMainGraphicsView::getInstance().scene()->addItem(proxy);
		proxy->setWindowFrameMargins(2,0,2,2);
		proxy->setCacheMode(QGraphicsItem::DeviceCoordinateCache); 
		proxy->setZValue(100);
		StelMainGraphicsView::getInstance().scene()->setActiveWindow(proxy);
		proxy->setFocus();
	}
	else
	{
		dialog->hide();
		emit visibleChanged(false);
		//proxy->clearFocus();
		StelMainGraphicsView::getInstance().scene()->setActiveWindow(0);
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

