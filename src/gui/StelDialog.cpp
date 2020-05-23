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
#include "StelDialog_p.hpp"
#include "StelMainView.hpp"
#include "StelGui.hpp"
#include "StelActionMgr.hpp"
#include "StelPropertyMgr.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"

#include <QDebug>
#include <QAbstractButton>
#include <QComboBox>
#include <QDialog>
#include <QGraphicsSceneWheelEvent>
#include <QMetaProperty>
#include <QStyleOptionGraphicsItem>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QScroller>
#include <QToolButton>
#include <QColorDialog>

StelDialog::StelDialog(QString dialogName, QObject* parent)
	: QObject(parent)
	, dialog(Q_NULLPTR)
	, proxy(Q_NULLPTR)
	, dialogName(dialogName)	
{
	if (parent == Q_NULLPTR)
		setParent(StelMainView::getInstance().getGuiWidget());

	connect(&StelApp::getInstance(), SIGNAL(fontChanged(QFont)), this, SLOT(handleFontChanged()));
	connect(&StelApp::getInstance(), SIGNAL(guiFontSizeChanged(int)), this, SLOT(handleFontChanged()));
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
	return dialog!=Q_NULLPTR && dialog->isVisible();
}

void StelDialog::setVisible(bool v)
{
	if (v)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());		
		QSize screenSize = StelMainView::getInstance().size();
		QSize maxSize = 0.8*screenSize;
		if (dialog)
		{
			// reload stylesheet, in case size changed!
			if (gui)
				dialog->setStyleSheet(gui->getStelStyle().qtStyleSheet);
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
			QSizeF newSize = proxy->size();
			if (newSize.width() >= maxSize.width())
				newSize.setWidth(maxSize.width());
			if (newSize.height() >= maxSize.height())
				newSize.setHeight(maxSize.height());
			if(newSize != dialog->size())
				proxy->resize(newSize);
		}
		else
		{
			QGraphicsWidget* parent = qobject_cast<QGraphicsWidget*>(this->parent());
			dialog = new QDialog(Q_NULLPTR);
			// dialog->setParent(parent);
			//dialog->setAttribute(Qt::WA_OpaquePaintEvent, true);
			connect(dialog, SIGNAL(rejected()), this, SLOT(close()));
			createDialogContent();
			if (gui)
				dialog->setStyleSheet(gui->getStelStyle().qtStyleSheet);
			// Ensure that tooltip get rendered in red in night mode.
			connect(&StelApp::getInstance(), SIGNAL(visionNightModeChanged(bool)), this, SLOT(updateNightModeProperty()));
			updateNightModeProperty();

			proxy = new CustomProxy(parent, Qt::Tool);
			proxy->setWidget(dialog);
			QSizeF size = proxy->size();

			connect(proxy, SIGNAL(sizeChanged(QSizeF)), this, SLOT(handleDialogSizeChanged(QSizeF)));

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
			proxy->setPos(newX, newY);
			// Invisible frame around the window to make resizing easier
			// (this also changes the bounding rectangle size)
			proxy->setWindowFrameMargins(7,0,7,7);

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
			if ( (newX>=proxy->size().width()) || (newY>=proxy->size().height()) )
			{
				//qDebug() << confNameSize << ": resize to " << storedSizeString;
				proxy->resize(qMax(static_cast<qreal>(newX), proxy->size().width()), qMax(static_cast<qreal>(newY), proxy->size().height()));
			}
			if(proxy->size().width() > maxSize.width() || proxy->size().height() > maxSize.height())
			{
				proxy->resize(maxSize);
			}
			handleDialogSizeChanged(proxy->size()); // This may trigger internal updates in subclasses. E.g. LocationPanel location arrow.

			// The caching is buggy on all platforms with Qt 4.5.2
			// Disabled on mac for the moment (https://github.com/Stellarium/stellarium/issues/393)
			#ifndef Q_OS_MAC
			proxy->setCacheMode(QGraphicsItem::ItemCoordinateCache);
			#endif

			proxy->setZValue(100);
			StelMainView::getInstance().scene()->setActiveWindow(proxy);
		}
		proxy->setFocus();
	}
	else
	{
		dialog->hide();
		//proxy->clearFocus();
		StelMainView::getInstance().focusSky();
	}
	emit visibleChanged(v);
}

void StelDialog::handleFontChanged()
{
	if (dialog && dialog->isVisible())
	{
		// reload stylesheet, in case size or font changed!
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		if (gui)
			dialog->setStyleSheet(gui->getStelStyle().qtStyleSheet);
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

void StelDialog::connectIntProperty(QLineEdit* lineEdit, const QString& propName)
{
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propName);
	Q_ASSERT_X(prop,"StelDialog", "StelProperty does not exist");

	//use a proxy for the connection
	new QLineEditStelPropertyConnectionHelper(prop,lineEdit);
}

void StelDialog::connectIntProperty(QSpinBox *spinBox, const QString &propName)
{
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propName);
	Q_ASSERT_X(prop,"StelDialog", "StelProperty does not exist");

	//use a proxy for the connection
	new QSpinBoxStelPropertyConnectionHelper(prop,spinBox);
}

void StelDialog::connectIntProperty(QComboBox *comboBox, const QString &propName)
{
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propName);
	Q_ASSERT_X(prop,"StelDialog", "StelProperty does not exist");

	//use a proxy for the connection
	new QComboBoxStelPropertyConnectionHelper(prop,comboBox);
}

void StelDialog::connectIntProperty(QSlider *slider, const QString &propName,int minValue, int maxValue)
{
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propName);
	Q_ASSERT_X(prop,"StelDialog", "StelProperty does not exist");

	//The connection is handled by a helper class. It is automatically destroyed when the slider is destroyed.
	new QSliderStelPropertyConnectionHelper(prop,minValue,maxValue,slider);
}

void StelDialog::connectDoubleProperty(QDoubleSpinBox *spinBox, const QString &propName)
{
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propName);
	Q_ASSERT_X(prop,"StelDialog", "StelProperty does not exist");

	//use a proxy for the connection
	new QDoubleSpinBoxStelPropertyConnectionHelper(prop,spinBox);
}

void StelDialog::connectDoubleProperty(QSlider *slider, const QString &propName,double minValue, double maxValue)
{
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propName);
	Q_ASSERT_X(prop,"StelDialog", "StelProperty does not exist");

	//The connection is handled by a helper class. It is automatically destroyed when the slider is destroyed.
	new QSliderStelPropertyConnectionHelper(prop,minValue,maxValue,slider);
}

void StelDialog::connectStringProperty(QComboBox *comboBox, const QString &propName)
{
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propName);
	Q_ASSERT_X(prop,"StelDialog", "StelProperty does not exist");

	//use a proxy for the connection
	new QComboBoxStelStringPropertyConnectionHelper(prop,comboBox);
}

void StelDialog::connectBoolProperty(QAbstractButton *checkBox, const QString &propName)
{
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propName);
	Q_ASSERT_X(prop,"StelDialog", "StelProperty does not exist");

	new QAbstractButtonStelPropertyConnectionHelper(prop,checkBox);
}

void StelDialog::connectBoolProperty(QGroupBox *checkBox, const QString &propName)
{
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propName);
	Q_ASSERT_X(prop,"StelDialog", "StelProperty does not exist");

	new QGroupBoxStelPropertyConnectionHelper(prop,checkBox);
}

void StelDialog::connectColorButton(QToolButton *toolButton, QString propertyName, QString iniName, QString moduleName)
{
	toolButton->setProperty("propName", propertyName);
	toolButton->setProperty("iniName", iniName);
	toolButton->setProperty("moduleName", moduleName);
	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propertyName);
	QColor color=prop->getValue().value<Vec3f>().toQColor();
	// Use style sheet to create a nice button :)
	toolButton->setStyleSheet("QToolButton { background-color:" + color.name() + "; }");
	toolButton->setFixedSize(QSize(18, 18));

	connect(toolButton, SIGNAL(released()), this, SLOT(askColor()));
}

void StelDialog::askColor()
{
	// Only work if connected from a QToolButton
	if (!sender())
	{
		qWarning() << "askColor(): No sender? Ignoring.";
		Q_ASSERT(0);
		return;
	}
	if (QString(sender()->metaObject()->className()) != QString("QToolButton"))
	{
		qWarning() << "Sender not a QToolButton but a |" << sender()->metaObject()->className() << "|! ColorButton not set up properly! Ignoring.";
		Q_ASSERT(0);
		return;
	}
	QString propName=sender()->property("propName").toString();
	QString iniName=sender()->property("iniName").toString();
	QString moduleName=sender()->property("moduleName").toString(); // optional
	if ((propName.isEmpty()) || (iniName.isEmpty()))
	{
		qWarning() << "ColorButton not set up properly! Ignoring.";
		Q_ASSERT(0);
		return;
	}
	Vec3d vColor = StelApp::getInstance().getStelPropertyManager()->getProperty(propName)->getValue().value<Vec3f>().toVec3d();
	QColor color = vColor.toQColor();
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(static_cast<QToolButton*>(QObject::sender())->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3d(c.redF(), c.greenF(), c.blueF());
		StelApp::getInstance().getStelPropertyManager()->setStelPropertyValue(propName, QVariant::fromValue(Vec3f(c.redF(), c.greenF(), c.blueF())));
		if (moduleName.isEmpty())
			StelApp::getInstance().getSettings()->setValue(iniName, vColor.toStr());
		else
		{
			StelModule *module=StelApp::getInstance().getModuleMgr().getModule(moduleName);
			QSettings *settings=module->getSettings();
			Q_ASSERT(settings);
			settings->setValue(iniName, vColor.toStr());
		}
		static_cast<QToolButton*>(QObject::sender())->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void StelDialog::enableKineticScrolling(bool b)
{
	if (kineticScrollingList.length()==0) return;
	if (b)
	{
		for (auto* w : kineticScrollingList)
		{
			QScroller::grabGesture(w, QScroller::LeftMouseButtonGesture);
			QScroller::scroller(w); // WHAT DOES THIS DO? We don't use the return value.
		}
	}
	else
	{
		for (auto* w : kineticScrollingList)
		{
			QScroller::ungrabGesture(w);
			// QScroller::scroller(w);
		}
	}
}

void StelDialog::updateNightModeProperty()
{
	dialog->setProperty("nightMode", StelApp::getInstance().getVisionModeNight());
}

void StelDialog::handleMovedTo(QPoint newPos)
{
	QSettings *conf=StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	conf->setValue("DialogPositions/" + dialogName, QString("%1,%2").arg(newPos.x()).arg(newPos.y()));
}

void StelDialog::handleDialogSizeChanged(QSizeF size)
{
	QSettings *conf=StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	conf->setValue("DialogSizes/" + dialogName, QString("%1,%2").arg(static_cast<int>(size.width())).arg(static_cast<int>(size.height())));
}


//// --- Implementation of StelDialog_p.hpp classes follow ---

QAbstractButtonStelPropertyConnectionHelper::QAbstractButtonStelPropertyConnectionHelper(StelProperty *prop, QAbstractButton *button)
	:StelPropertyProxy(prop,button), button(button)
{
	QVariant val = prop->getValue();
	bool ok = val.canConvert<bool>();
	Q_ASSERT_X(ok,"QAbstractButtonStelPropertyConnectionHelper","Can not convert to bool datatype");
	Q_UNUSED(ok);
	onPropertyChanged(val);

	//in this direction, we can directly connect because Qt supports QVariant slots with the new syntax
	connect(button, &QAbstractButton::toggled, prop, &StelProperty::setValue);
}

void QAbstractButtonStelPropertyConnectionHelper::onPropertyChanged(const QVariant &value)
{
	//block signals to prevent sending the valueChanged signal, changing the property again
	bool b = button->blockSignals(true);
	button->setChecked(value.toBool());
	button->blockSignals(b);
}

QGroupBoxStelPropertyConnectionHelper::QGroupBoxStelPropertyConnectionHelper(StelProperty *prop, QGroupBox *box)
	:StelPropertyProxy(prop,box), box(box)
{
	QVariant val = prop->getValue();
	bool ok = val.canConvert<bool>();
	Q_ASSERT_X(ok,"QGroupBoxStelPropertyConnectionHelper","Can not convert to bool datatype");
	Q_UNUSED(ok);
	onPropertyChanged(val);

	//in this direction, we can directly connect because Qt supports QVariant slots with the new syntax
	connect(box, &QGroupBox::toggled, prop, &StelProperty::setValue);
}

void QGroupBoxStelPropertyConnectionHelper::onPropertyChanged(const QVariant &value)
{
	//block signals to prevent sending the valueChanged signal, changing the property again
	bool b = box->blockSignals(true);
	box->setChecked(value.toBool());
	box->blockSignals(b);
}

QComboBoxStelPropertyConnectionHelper::QComboBoxStelPropertyConnectionHelper(StelProperty *prop, QComboBox *combo)
	:StelPropertyProxy(prop,combo), combo(combo)
{
	QVariant val = prop->getValue();
	bool ok = val.canConvert<int>();
	Q_ASSERT_X(ok,"QComboBoxStelPropertyConnectionHelper","Can not convert to int datatype");
	Q_UNUSED(ok);
	onPropertyChanged(val);

	//in this direction, we can directly connect because Qt supports QVariant slots with the new syntax
	connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),prop,&StelProperty::setValue);
}

void QComboBoxStelPropertyConnectionHelper::onPropertyChanged(const QVariant &value)
{
	//block signals to prevent sending the valueChanged signal, changing the property again
	bool b = combo->blockSignals(true);
	combo->setCurrentIndex(value.toInt());
	combo->blockSignals(b);
}
QComboBoxStelStringPropertyConnectionHelper::QComboBoxStelStringPropertyConnectionHelper(StelProperty *prop, QComboBox *combo)
	:StelPropertyProxy(prop,combo), combo(combo)
{
	QVariant val = prop->getValue();
	bool ok = val.canConvert<QString>();
	Q_ASSERT_X(ok,"QComboBoxStelStringPropertyConnectionHelper","Can not convert to QString datatype");
	Q_UNUSED(ok);
	onPropertyChanged(val);

	//in this direction, we can directly connect because Qt supports QVariant slots with the new syntax
	connect(combo, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),prop,&StelProperty::setValue);
}

void QComboBoxStelStringPropertyConnectionHelper::onPropertyChanged(const QVariant &value)
{
	//block signals to prevent sending the valueChanged signal, changing the property again
	bool b = combo->blockSignals(true);
	combo->setCurrentText(value.toString());
	combo->blockSignals(b);
}

QLineEditStelPropertyConnectionHelper::QLineEditStelPropertyConnectionHelper(StelProperty *prop, QLineEdit *edit)
	:StelPropertyProxy(prop,edit), edit(edit)
{
	QVariant val = prop->getValue();
	bool ok = val.canConvert<int>();
	Q_ASSERT_X(ok,"QLineEditStelPropertyConnectionHelper","Can not convert to int datatype");
	Q_UNUSED(ok);
	onPropertyChanged(val);

	//in this direction, we can directly connect because Qt supports QVariant slots with the new syntax
	connect(edit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited),prop,&StelProperty::setValue);
}

void QLineEditStelPropertyConnectionHelper::onPropertyChanged(const QVariant &value)
{
	//block signals to prevent sending the valueChanged signal, changing the property again
	bool b = edit->blockSignals(true);
	edit->setText(value.toString());
	edit->blockSignals(b);
}

QSpinBoxStelPropertyConnectionHelper::QSpinBoxStelPropertyConnectionHelper(StelProperty *prop, QSpinBox *spin)
	:StelPropertyProxy(prop,spin), spin(spin)
{
	QVariant val = prop->getValue();
	bool ok = val.canConvert<int>();
	Q_ASSERT_X(ok,"QSpinBoxStelPropertyConnectionHelper","Can not convert to int datatype");
	Q_UNUSED(ok);
	onPropertyChanged(val);

	//in this direction, we can directly connect because Qt supports QVariant slots with the new syntax
	connect(spin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),prop,&StelProperty::setValue);
}

void QSpinBoxStelPropertyConnectionHelper::onPropertyChanged(const QVariant &value)
{
	//block signals to prevent sending the valueChanged signal, changing the property again
	bool b = spin->blockSignals(true);
	spin->setValue(value.toInt());
	spin->blockSignals(b);
}

QDoubleSpinBoxStelPropertyConnectionHelper::QDoubleSpinBoxStelPropertyConnectionHelper(StelProperty *prop, QDoubleSpinBox *spin)
	:StelPropertyProxy(prop,spin), spin(spin)
{
	QVariant val = prop->getValue();
	bool ok = val.canConvert<double>();
	Q_ASSERT_X(ok,"QDoubleSpinBoxStelPropertyConnectionHelper","Can not convert to double datatype");
	Q_UNUSED(ok);
	onPropertyChanged(val);

	//in this direction, we can directly connect because Qt supports QVariant slots with the new syntax
	connect(spin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),prop,&StelProperty::setValue);
}

void QDoubleSpinBoxStelPropertyConnectionHelper::onPropertyChanged(const QVariant &value)
{
	//block signals to prevent sending the valueChanged signal, changing the property again
	bool b = spin->blockSignals(true);
	spin->setValue(value.toDouble());
	spin->blockSignals(b);
}

QSliderStelPropertyConnectionHelper::QSliderStelPropertyConnectionHelper(StelProperty *prop, double minValue, double maxValue, QSlider *slider)
	: StelPropertyProxy(prop,slider),slider(slider),minValue(minValue),maxValue(maxValue)
{
	QVariant val = prop->getValue();
	bool ok = val.canConvert<double>();
	Q_ASSERT_X(ok,"QSliderStelPropertyConnectionHelper","Can not convert to double datatype");
	Q_UNUSED(ok);

	dRange = maxValue - minValue;
	onPropertyChanged(val);

	connect(slider,SIGNAL(valueChanged(int)),this,SLOT(sliderIntValueChanged(int)));
}

QSliderStelPropertyConnectionHelper::QSliderStelPropertyConnectionHelper(StelProperty *prop, int minValue, int maxValue, QSlider *slider)
	: StelPropertyProxy(prop,slider),slider(slider),minValue(minValue),maxValue(maxValue)
{
	QVariant val = prop->getValue();
	bool ok = val.canConvert<double>();
	Q_ASSERT_X(ok,"QSliderStelPropertyConnectionHelper","Can not convert to double datatype");
	Q_UNUSED(ok);

	dRange = maxValue - minValue;
	onPropertyChanged(val);

	connect(slider,SIGNAL(valueChanged(int)),this,SLOT(sliderIntValueChanged(int)));
}
void QSliderStelPropertyConnectionHelper::sliderIntValueChanged(int val)
{
	double dVal = ((val - slider->minimum()) / static_cast<double>(slider->maximum() - slider->minimum())) * dRange + minValue;
	prop->setValue(dVal);
}

void QSliderStelPropertyConnectionHelper::onPropertyChanged(const QVariant& val)
{
	double dVal = val.toDouble();
	int iRange = slider->maximum() - slider->minimum();
	int iVal = qRound(((dVal - minValue)/dRange) * iRange + slider->minimum());
	bool b = slider->blockSignals(true);
	slider->setValue(iVal);
	slider->blockSignals(b);
}
