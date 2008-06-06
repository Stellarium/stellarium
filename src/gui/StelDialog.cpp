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


#include "StelDialog.hpp"
#include "StelMainGraphicsView.hpp"

#include <QDebug>
#include <QDialog>
#include <QGraphicsProxyWidget>

class CustomProxy : public QGraphicsProxyWidget
{
	public:
		CustomProxy(QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0) : QGraphicsProxyWidget(parent, wFlags) {;}
		//! Reimplement this method to add windows decorations. Currently there are invisible 2 px decorations
		void paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
		{
/*			QStyleOptionTitleBar bar;
			initStyleOption(&bar);
			bar.subControls = QStyle::SC_TitleBarCloseButton;
			qWarning() << style()->subControlRect(QStyle::CC_TitleBar, &bar, QStyle::SC_TitleBarCloseButton);
			QGraphicsProxyWidget::paintWindowFrame(painter, option, widget);*/
		}
};

StelDialog::StelDialog() : dialog(NULL)
{
}

StelDialog::~StelDialog()
{
}


void StelDialog::close()
{
	emit closed();
}

void StelDialog::setVisible(bool v)
{
	if (v) 
	{
		if (dialog)
		{
			dialog->show();
			return;
		}
		dialog = new QDialog(&StelMainGraphicsView::getInstance());
		
		createDialogContent();
		
		CustomProxy* proxy = new CustomProxy(NULL, Qt::Tool);
		proxy->setWidget(dialog);
		StelMainGraphicsView::getInstance().scene()->addItem(proxy);
		proxy->setWindowFrameMargins(2,0,2,2);
		proxy->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
		proxy->setZValue(100);
		StelMainGraphicsView::getInstance().scene()->setActiveWindow(proxy);
	}
	else
	{
		dialog->hide();
	}
}
