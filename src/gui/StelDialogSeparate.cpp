/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
 * Copyright (C) 2021 Georg Zotti
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


#include "StelDialogSeparate.hpp"
#include "StelMainView.hpp"
#include "StelGui.hpp"

//#include <QDebug>
#include <QWidget>
#include <QDialog>
#include <QSizeGrip>

class NightCover: public QWidget{
public:
    NightCover(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::FramelessWindowHint|Qt::WindowTransparentForInput)
	: QWidget(parent, f)
    {
	setAttribute(Qt::WA_TransparentForMouseEvents);
	// We cannot define this style in the global QSS file, as the background defined for QWidget gets inherited.
	setStyleSheet("QWidget{background: none;"
					 "color: rgba(80, 8, 0, 0.33);"
			      "background-color: rgba(80, 8, 0, 0.33);"
			       "selection-color: rgba(80, 8, 0, 0.33);"
		    "selection-background-color: rgba(80, 8, 0, 0.33);"
		    "alternate-background-color: rgba(80, 8, 0, 0.33);}");
    }

protected:
    virtual void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE
    {
	Q_UNUSED(event)
	QRect parentRect=reinterpret_cast<QWidget*>(parent())->geometry();
	setGeometry(0, 0, parentRect.width(), parentRect.height());
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setBrush(QBrush(QColor(128,16,0, 50)));
	painter.setPen(Qt::NoPen);
	painter.drawRect(rect());
    }
};


StelDialogSeparate::StelDialogSeparate(QString dialogName, QObject* parent)
	: StelDialog(dialogName, parent),
	nightCover(Q_NULLPTR)
{
}

StelDialogSeparate::~StelDialogSeparate()
{
    if (nightCover)
	delete nightCover;
}

void StelDialogSeparate::setVisible(bool v)
{
	if (v)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());		
		QSize screenSize = StelMainView::getInstance().size();
		// If dialog size is very large and we move to a computer with much smaller screen, this should create the dialog with reasonable better size.
		QSize maxSize = 0.95*screenSize;
		if (dialog)
		{
			// reload stylesheet, in case size changed!
			if (gui)
				dialog->setStyleSheet(gui->getStelStyle().qtStyleSheet);
			dialog->show();
		}
		else
		{
			dialog = new CustomDialog(&StelMainView::getInstance(), Qt::Tool | Qt::FramelessWindowHint);
			connect(dialog, SIGNAL(rejected()), this, SLOT(close()));
			createDialogContent();
			if (gui)
				dialog->setStyleSheet(gui->getStelStyle().qtStyleSheet);
			// Ensure that tooltip get rendered in red in night mode.
			connect(&StelApp::getInstance(), SIGNAL(visionNightModeChanged(bool)), this, SLOT(updateNightModeProperty(bool)));
			updateNightModeProperty(StelApp::getInstance().getVisionModeNight());

			reinterpret_cast<CustomDialog*>(dialog)->setSizeGripEnabled(true);
			QSizeF size = dialog->size();
			connect(reinterpret_cast<CustomDialog*>(dialog), SIGNAL(sizeChanged(QSizeF)), this, SLOT(handleDialogSizeChanged(QSizeF)));

			int newX, newY;
			// Retrieve panel locations from config.ini, but shift if required to a visible position.
			// else centre dialog according to current window size.
			QSettings *conf=StelApp::getInstance().getSettings();
			Q_ASSERT(conf);
			QString confNamePt="DialogPositions/" + dialogName;
			QString storedPosString=conf->value(confNamePt,
							    QString("%1,%2")
							    .arg(static_cast<int>((screenSize.width()  - size.width() )/2))
							    .arg(static_cast<int>((screenSize.height() - size.height())/2)))
					.toString();
			QStringList posList=storedPosString.split(",");
			if (posList.length()==2)
			{
				newX=posList.at(0).toInt();
				newY=posList.at(1).toInt();
			}
			else	// in case there is an invalid string?
			{
				newX=static_cast<int>((screenSize.width()  - size.width() )/2);
				newY=static_cast<int>((screenSize.height() - size.height())/2);
			}

			if (newX>=screenSize.width())
				newX= (screenSize.width()  - dialog->size().width());
			if (newY>=screenSize.height())
				newY= (screenSize.height() - dialog->size().height());

			// Make sure that the window's title bar is accessible
			if (newY <-0)
				newY = 0;
			dialog->move(newX, newY);

			// Retrieve stored panel sizes, scale panel up if it was stored larger than default.
			QString confNameSize="DialogSizes/" + dialogName;
			QString storedSizeString=conf->value(confNameSize, QString("0,0")).toString();
			QStringList sizeList=storedSizeString.split(",");
			if (sizeList.length()==2)
			{
				newX=sizeList.at(0).toInt();
				newY=sizeList.at(1).toInt();
			}
			else	// in case there is an invalid string?
			{
				newX=0;
				newY=0;
			}
			// resize only if number was valid and larger than default loaded size.
			if ( (newX>=dialog->size().width()) || (newY>=dialog->size().height()) )
			{
				//qDebug() << confNameSize << ": resize from" << proxy->size().width() << "x" << proxy->size().height() << "to " << storedSizeString;
				dialog->resize(qMax(newX, dialog->size().width()), qMax(newY, dialog->size().height()));
			}
			if(dialog->size().width() > maxSize.width() || dialog->size().height() > maxSize.height())
			{
				dialog->resize(qMin(maxSize.width(), dialog->size().width()), qMin(maxSize.height(), dialog->size().height()));
			}
			handleDialogSizeChanged(dialog->size()); // This may trigger internal updates in subclasses. E.g. LocationPanel location arrow.
			dialog->show();
		}
		dialog->setFocus();
		nightCover=new NightCover(dialog);
	}
	else
	{
		dialog->hide();
		StelMainView::getInstance().focusSky();
	}
	emit visibleChanged(v);
}

void StelDialogSeparate::updateNightModeProperty(bool n)
{
	StelDialog::updateNightModeProperty(n);
	// It seems we cannot code a nightMode property for the QSizeGrip in the global QSS sheet :-(
	QList<QSizeGrip*> grips=dialog->findChildren<QSizeGrip*>();
	for (const auto &g: grips)
	{
		if (n)
			g->setStyleSheet("QSizeGrip { background: rgb( 65,   0,   0);}");
		else
			g->setStyleSheet("QSizeGrip { background: rgb(101, 101, 101);}");
	}

	if (nightCover)
	{
	    if (n)
		nightCover->show();
	    else
		nightCover->hide();
	}
}
