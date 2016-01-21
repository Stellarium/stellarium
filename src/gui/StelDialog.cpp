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
#include "StelMainView.hpp"
#include "StelGui.hpp"
#include "StelApp.hpp"

#include <QDebug>
#include <QDialog>
#include <QGraphicsProxyWidget>
#include <QStyleOptionGraphicsItem>
#include <QSettings>
#ifdef Q_OS_WIN
	#include <QScroller>
#endif

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
			if (StelApp::getInstance().getSettings()->value("gui/flag_use_window_transparency", true).toBool())
			{
				switch (event->type())
				{
					case QEvent::WindowDeactivate:
						widget()->setWindowOpacity(0.4);
						break;
					case QEvent::WindowActivate:
					case QEvent::GrabMouse:
						widget()->setWindowOpacity(0.9);
						break;
						// GZ Add moving events!
					case QEvent::Drop:
						// TODO: Store panel locations. HOW TO RETRIEVE DIALOG NAME?
						//widget()->
						qDebug() << "panel location changed to " << pos();
					default:
						break;
				}
			}
			return QGraphicsProxyWidget::event(event);
		}
};

StelDialog::StelDialog(QObject* parent)
	: QObject(parent)
	, dialog(NULL)
	, proxy(NULL)
{
	if (parent == NULL)
		setParent(StelMainView::getInstance().getGuiWidget());
}

StelDialog::~StelDialog()
{
}


void StelDialog::close()
{
	setVisible(false);
}

bool StelDialog::visible() const
{
	return dialog!=NULL && dialog->isVisible();
}

void StelDialog::setVisible(bool v)
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

		QGraphicsWidget* parent = qobject_cast<QGraphicsWidget*>(this->parent());
		dialog = new QDialog(NULL);
		// dialog->setParent(parent);
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		Q_ASSERT(gui);
		//dialog->setAttribute(Qt::WA_OpaquePaintEvent, true);
		connect(dialog, SIGNAL(rejected()), this, SLOT(close()));
		createDialogContent();
		dialog->setStyleSheet(gui->getStelStyle().qtStyleSheet);

		// Ensure that tooltip get rendered in red in night mode.
		connect(&StelApp::getInstance(), SIGNAL(visionNightModeChanged(bool)), this, SLOT(updateNightModeProperty()));
		updateNightModeProperty();

		proxy = new CustomProxy(parent, Qt::Tool);
		proxy->setWidget(dialog);
		QSizeF size = proxy->size();

		int newX, newY;

		// Retrieve panel locations from config.ini, but shift if required to a visible position.
		// else centre dialog according to current window size.
		QSettings *conf=StelApp::getInstance().getSettings();
		Q_ASSERT(conf);
		QString confNameX="DialogPositions/" + dialogName + "_x";
		QString confNameY="DialogPositions/" + dialogName + "_y";
		newX=conf->value(confNameX, (int)((screenSize.width()  - size.width() )/2)).toInt();
		newY=conf->value(confNameY, (int)((screenSize.height() - size.height())/2)).toInt();

		if (newX>=screenSize.width())
			newX= (screenSize.width()  - dialog->size().width());
		if (newY>=screenSize.height())
			newY= (screenSize.height() - dialog->size().height());

		// Make sure that the window's title bar is accessible
		if (newY <-0)
			newY = 0;
		proxy->setPos(newX, newY);
		proxy->setWindowFrameMargins(2,0,2,2);
		// (this also changes the bounding rectangle size)

		// The caching is buggy on all plateforms with Qt 4.5.2
		proxy->setCacheMode(QGraphicsItem::ItemCoordinateCache);

		proxy->setZValue(100);
		StelMainView::getInstance().scene()->setActiveWindow(proxy);
		proxy->setFocus();
	}
	else
	{
		dialog->hide();
		emit visibleChanged(false);
		//proxy->clearFocus();
		StelMainView::getInstance().focusSky();
	}
}

#ifdef Q_OS_WIN
void StelDialog::installKineticScrolling(QList<QWidget *> addscroll)
{
	return; // Temporary disable feature, bug in Qt: https://bugreports.qt-project.org/browse/QTBUG-41299

	if (StelApp::getInstance().getSettings()->value("gui/flag_enable_kinetic_scrolling", true).toBool() == false)
		return;

	foreach(QWidget * w, addscroll)
	{
		QScroller::grabGesture(w, QScroller::LeftMouseButtonGesture);
		QScroller::scroller(w);
	}
}
#endif


void StelDialog::updateNightModeProperty()
{
	dialog->setProperty("nightMode", StelApp::getInstance().getVisionModeNight());
}
