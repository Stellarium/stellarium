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


#include "StelDialog.hpp"
#include "StelMainGraphicsView.hpp"

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

StelDialog::StelDialog(QObject* parent) : QObject(parent), dialog(NULL)
{
}

StelDialog::~StelDialog()
{
}


void StelDialog::close()
{
	setVisible(false);
	StelMainGraphicsView::getInstance().scene()->setActiveWindow(0);
	((QGraphicsWidget*)StelMainGraphicsView::getInstance().getStelAppGraphicsWidget())->setFocus(Qt::OtherFocusReason);
}

bool StelDialog::visible() const
{
	return dialog!=NULL && dialog->isVisible();
}

void StelDialog::setVisible(bool v)
{
	if (v)
	{
		QSize screenSize = StelMainGraphicsView::getInstance().size();
		if (dialog)
		{
			dialog->show();
			StelMainGraphicsView::getInstance().scene()->setActiveWindow(proxy);
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
		dialog = new QDialog(NULL);
		//dialog->setAttribute(Qt::WA_OpaquePaintEvent, true);
		connect(dialog, SIGNAL(rejected()), this, SLOT(close()));
		createDialogContent();

		proxy = new CustomProxy(NULL, Qt::Tool);
		proxy->setWidget(dialog);
		QSizeF size = proxy->size();

		// centre with dialog according to current window size.
		int newX = (int)((screenSize.width() - size.width())/2);
		int newY = (int)((screenSize.height() - size.height())/2);
		// Make sure that the window's title bar is accessible
		if (newY <-0)
			newY = 0;
		proxy->setPos(newX, newY);
		StelMainGraphicsView::getInstance().scene()->addItem(proxy);
		proxy->setWindowFrameMargins(2,0,2,2);
		// (this also changes the bounding rectangle size)

		// The caching is buggy on all plateforms with Qt 4.5.2
		proxy->setCacheMode(QGraphicsItem::ItemCoordinateCache);

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
