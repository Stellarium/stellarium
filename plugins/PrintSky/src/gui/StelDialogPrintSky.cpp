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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#include "StelDialogPrintSky.hpp"
#include "StelMainView.hpp"
//#include "StelMainWindow.hpp"

#include <QDebug>
#include <QDialog>
#include <QGraphicsProxyWidget>
#include <QStyleOptionGraphicsItem>

class CustomProxy : public QGraphicsProxyWidget
{
	public:
		CustomProxy(QGraphicsItem *parent = Q_NULLPTR, Qt::WindowFlags wFlags = Qt::Widget) : QGraphicsProxyWidget(parent, wFlags)
		{
			setFocusPolicy(Qt::StrongFocus);
		}
		//! Reimplement this method to add windows decorations. Currently there are invisible 2 px decorations
		void paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
		{
			Q_UNUSED(painter)
			Q_UNUSED(option)
			Q_UNUSED(widget)
/*			QStyleOptionTitleBar bar;
			initStyleOption(&bar);
			bar.subControls = QStyle::SC_TitleBarCloseButton;
			qWarning() << style()->subControlRect(QStyle::CC_TitleBar, &bar, QStyle::SC_TitleBarCloseButton);
			QGraphicsProxyWidget::paintWindowFrame(painter, option, widget);*/
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

StelDialogPrintSky::StelDialogPrintSky() : dialog(Q_NULLPTR)
{
}

StelDialogPrintSky::~StelDialogPrintSky()
{
}


void StelDialogPrintSky::close()
{
	setVisible(false);
}

void StelDialogPrintSky::setVisible(bool v)
{
	if (v)
	{
		QSize screenSize = StelMainView::getInstance().size();
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
		proxy->setPos(static_cast<int>((screenSize.width()-bound.width())/2), static_cast<int>((screenSize.height()-bound.height())/2));
		StelMainView::getInstance().scene()->addItem(proxy);
		proxy->setWindowFrameMargins(2,0,2,2);
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
