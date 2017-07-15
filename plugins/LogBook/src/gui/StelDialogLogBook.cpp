/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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


#include "StelDialogLogBook.hpp"
#include "StelMainView.hpp"
#include "StelMainWindow.hpp"

#include <QDebug>
#include <QDialog>
#include <QGraphicsProxyWidget>
#include <QStyleOptionGraphicsItem>

class CustomProxy : public QGraphicsProxyWidget
{
	public:
		CustomProxy(QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0) : QGraphicsProxyWidget(parent, wFlags)
		{
			setFocusPolicy(Qt::StrongFocus);
		}
		//! Reimplement this method to add windows decorations. Currently there are invisible 2 px decorations
		void paintWindowFrame(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
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
};

StelDialogLogBook::StelDialogLogBook() : dialog(Q_NULLPTR)
{
}

StelDialogLogBook::~StelDialogLogBook()
{
}


void StelDialogLogBook::close()
{
	setVisible(false);
	StelMainView::getInstance().scene()->setActiveWindow(0);
	((QGraphicsWidget*)StelMainView::getInstance().getStelAppGraphicsWidget())->setFocus(Qt::OtherFocusReason);
	emit(dialogClosed(this));
}

void StelDialogLogBook::setVisible(bool v)
{
	if (v) 
	{
		QSize screenSize = StelMainWindow::getInstance().size();
		if (dialog)
		{
			dialog->show();
			StelMainView::getInstance().scene()->setActiveWindow(proxy);
			// If the main window has been resized, it is possible the dialog
			// will be off screen.  Check for this and move it to a visible
			// position if necessary
			QPointF newPos = proxy->pos();
			if (newPos.x()>=screenSize.width())
				newPos.setX(screenSize.width() - dialog->size().width());
			if (newPos.y()>=screenSize.height())
				newPos.setY(screenSize.height() - dialog->size().height());
			if (newPos != dialog->pos())
				proxy->setPos(newPos);
			
			proxy->setFocus();
			return;
		}
		dialog = new QDialog(Q_NULLPTR);
		connect(dialog, SIGNAL(rejected()), this, SLOT(close()));
		createDialogContent();
		
		proxy = new CustomProxy(Q_NULLPTR, Qt::Tool);
		proxy->setWidget(dialog);
		QRectF bound = proxy->boundingRect();
		
		// centre with dialog according to current window size.
		proxy->setPos((int)((screenSize.width()-bound.width())/2), (int)((screenSize.height()-bound.height())/2));
		StelMainView::getInstance().scene()->addItem(proxy);
		proxy->setWindowFrameMargins(2,0,2,2);

		// The caching is buggy on all plateforms with Qt 4.5.2

#if QT_VERSION==0x040502
		proxy->setCacheMode(QGraphicsItem::NoCache);
#else
		proxy->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
#endif

		proxy->setZValue(100);
		StelMainView::getInstance().scene()->setActiveWindow(proxy);
		proxy->setFocus();
	}
	else
	{
		dialog->hide();
		emit visibleChanged(false);
		//proxy->clearFocus();
		StelMainView::getInstance().scene()->setActiveWindow(0);
	}
}
