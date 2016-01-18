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
#include "StelActionMgr.hpp"
#include "StelPropertyMgr.hpp"

#include <QDebug>
#include <QAbstractButton>
#include <QDialog>
#include <QGraphicsProxyWidget>
#include <QMetaProperty>
#include <QStyleOptionGraphicsItem>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
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

		// centre with dialog according to current window size.
		int newX = (int)((screenSize.width() - size.width())/2);
		int newY = (int)((screenSize.height() - size.height())/2);
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

void StelDialog::connectCheckBox(QAbstractButton *checkBox, const QString &actionName)
{
	StelAction* action = StelApp::getInstance().getStelActionManager()->findAction(actionName);
	connectCheckBox(checkBox,action);
}

void StelDialog::connectCheckBox(QAbstractButton *checkBox, StelAction *action)
{
	Q_ASSERT(action);
	checkBox->setChecked(action->isChecked());
	connect(action, SIGNAL(toggled(bool)), checkBox, SLOT(setChecked(bool)));
	connect(checkBox, SIGNAL(toggled(bool)), action, SLOT(setChecked(bool)));
}

void StelDialog::connectIntProperty(QSpinBox *spinBox, const QString &propName)
{
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propName);
	Q_ASSERT_X(prop,"StelDialog", "StelProperty does not exist");
	bool ok;
	spinBox->setValue(prop->getValue().toInt(&ok));
	Q_ASSERT_X(ok,"StelDialog","Can not convert to required datatype");
	StelPropertyIntProxy* prox = qobject_cast<StelPropertyIntProxy*>(prop->getProxy());
	Q_ASSERT_X(prox,"StelDialog", "No valid StelPropertyProxy defined for this property");
	//in this direction, we require a proxy
	connect(prox, &StelPropertyIntProxy::propertyChanged, spinBox, &QSpinBox::setValue);
	//in this direction, we can directly connect because Qt supports QVariant slots with the new syntax
	connect(spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),prop,&StelProperty::setValue);
}

void StelDialog::connectDoubleProperty(QDoubleSpinBox *spinBox, const QString &propName)
{
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propName);
	Q_ASSERT_X(prop,"StelDialog", "StelProperty does not exist");
	bool ok;
	spinBox->setValue(prop->getValue().toDouble(&ok));
	Q_ASSERT_X(ok,"StelDialog","Can not convert to required datatype");
	StelPropertyDoubleProxy* prox = qobject_cast<StelPropertyDoubleProxy*>(prop->getProxy());
	Q_ASSERT_X(prox,"StelDialog", "No valid StelPropertyProxy defined for this property");
	//in this direction, we require a proxy
	connect(prox, &StelPropertyDoubleProxy::propertyChanged, spinBox, &QDoubleSpinBox::setValue);
	//in this direction, we can directly connect because Qt supports QVariant slots with the new syntax
	connect(spinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),prop,&StelProperty::setValue);
}

void StelDialog::connectDoubleProperty(QSlider *slider, const QString &propName,double minValue, double maxValue)
{
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propName);
	Q_ASSERT_X(prop,"StelDialog", "StelProperty does not exist");

	//The connection is handled by a helper class. It is automatically destroyed when the slider is destroyed.
	new QSliderStelPropertyConnectionHelper(slider,prop,minValue,maxValue,slider);
}

void StelDialog::connectBoolProperty(QAbstractButton *checkBox, const QString &propName)
{
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propName);
	Q_ASSERT_X(prop,"StelDialog", "StelProperty does not exist");
	checkBox->setChecked(prop->getValue().toBool());
	StelPropertyBoolProxy* prox = qobject_cast<StelPropertyBoolProxy*>(prop->getProxy());
	Q_ASSERT_X(prox,"StelDialog", "No valid StelPropertyProxy defined for this property");
	//in this direction, we require a proxy
	connect(prox, &StelPropertyBoolProxy::propertyChanged, checkBox, &QAbstractButton::setChecked);
	//in this direction, we can directly connect because Qt supports QVariant slots with the new syntax
	connect(checkBox, &QAbstractButton::toggled, prop, &StelProperty::setValue);
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

QSliderStelPropertyConnectionHelper::QSliderStelPropertyConnectionHelper(QSlider *slider, StelProperty *prop, double minValue, double maxValue, QObject *parent)
	: QObject(parent),slider(slider),prop(prop),minValue(minValue),maxValue(maxValue)
{
	bool ok;
	prop->getValue().toDouble(&ok);
	Q_ASSERT_X(ok,"QSliderStelPropertyConnectionHelper","Can not convert to required datatype");

	dRange = maxValue - minValue;
	propertyValueChanged(prop->getValue());

	connect(slider,SIGNAL(valueChanged(int)),this,SLOT(sliderIntValueChanged(int)));
	connect(prop,SIGNAL(changed(QVariant)), this, SLOT(propertyValueChanged(QVariant)));
}

void QSliderStelPropertyConnectionHelper::sliderIntValueChanged(int val)
{
	double dVal = ((val - slider->minimum()) / (double)(slider->maximum() - slider->minimum())) * dRange + minValue;
	prop->setValue(dVal);
}

void QSliderStelPropertyConnectionHelper::propertyValueChanged(const QVariant& val)
{
	double dVal = val.toDouble();
	int iRange = slider->maximum() - slider->minimum();
	int iVal = qRound(((dVal - minValue)/dRange) * iRange + slider->minimum());
	slider->setValue(iVal);
}
