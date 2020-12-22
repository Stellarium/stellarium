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

#ifndef STELDIALOG_HPP
#define STELDIALOG_HPP

#include <QObject>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneResizeEvent>
#include <QSettings>
#include <QWidget>
#include "StelApp.hpp"

class QAbstractButton;
class QGroupBox;
class QComboBox;
class QSpinBox;
class QLineEdit;
class QDoubleSpinBox;
class QSlider;
class StelAction;
class QToolButton;

//! Base class for all the GUI windows in Stellarium.
//! 
//! Windows in Stellarium are actually basic QWidgets that have to be wrapped in
//! a QGraphicsProxyWidget (CustomProxy) to be displayed by StelMainView
//! (which is derived from QGraphicsView). See the %Qt documentation for details.
//! 
//! The base widget needs to be populated with controls in the implementation
//! of the createDialogContent() function. This can be done either manually, or
//! by using a .ui file. See the %Qt documentation on using %Qt Designer .ui files
//! for details.
//! 
//! The createDialogContent() function itself is called automatically the first
//! time setVisible() is called with "true".
//! 
//! Moving a window is done by dragging its title bar, defined in the BarFrame
//! class. Every derived window class needs a BarFrame object - it
//! has to be either included in a .ui file, or manually instantiated in
//! createDialogContent().
//!
//! The screen location of the StelDialog can be stored in config.ini. This requires
//! setting dialogName (must be a unique name, should be set in the constructor),
//! and setting a connect() from the BarFrame's movedTo() signal to handleMovedTo()
//! in createDialogContent().
//! If the dialog is called and the stored location is off-screen, the dialog is
//! shifted to become visible.
//!
//! ## StelProperty and StelAction
//! The StelDialog base class provides multiple helper functions that allow easy
//! two-way binding of widgets to specific StelAction or StelProperty instances. These functions are:
//! - \ref connectCheckBox to connect a StelAction to a QAbstractButton (includes QCheckBox)
//! - \ref connectIntProperty to connect a StelProperty to a QSpinBox or QComboBox
//! - \ref connectDoubleProperty to connect a StelProperty to a QDoubleSpinBox or QSlider
//! - \ref connectBoolProperty to connect a StelProperty to a QAbstractButton (includes QCheckBox)
//! Take care that a valid property name is used and it represents a property that can be converted to
//! the required data type, or the program will crash at runtime when the function is called
class StelDialog : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
public:
	StelDialog(QString dialogName="Default", QObject* parent=Q_NULLPTR);
	virtual ~StelDialog() Q_DECL_OVERRIDE;

	//! Notify that the application style changed
	virtual void styleChanged();

	//! Returns true if the dialog contents have been constructed and are currently shown
	bool visible() const;

public slots:
	//! Retranslate the content of the dialog.
	//! Needs to be connected to StelApp::languageChanged().
	//! At the very least, if the window is
	//! <a href="http://doc.qt.nokia.com/stable/designer-using-a-ui-file.html">
	//! based on a Qt Designer file (.ui)</a>, the implementation needs to call
	//! the generated class' retranslateUi() method, like this:
	//! \code
	//! if (dialog) 
	//! 	ui->retranslateUi(dialog);
	//! \endcode
	virtual void retranslate() = 0;
	//! On the first call with "true" populates the window contents.
	virtual void setVisible(bool);
	//! Closes the window (the window widget is not deleted, just not visible).
	virtual void close();
	//! Adds dialog location to config.ini; should be connected in createDialogContent()
	void handleMovedTo(QPoint newPos);
	//! Stores dialog sizes into config.ini; should be connected from the proxy.
	//! When a subclass needs a size-dependent update, implement such update in the subclass version,
	//! but call StelDialog::handleDialogSizeChanged() first.
	virtual void handleDialogSizeChanged(QSizeF size);
	QString getDialogName() const {return dialogName;}
signals:
	void visibleChanged(bool);

protected:
	//! Initialize the dialog widgets and connect the signals/slots.
	virtual void createDialogContent()=0;

	//! Helper function to connect a checkbox to the StelAction with the specified name
	static void connectCheckBox(QAbstractButton* checkBox,const QString& actionName);
	//! Helper function to connect a checkbox to the given StelAction
	static void connectCheckBox(QAbstractButton *checkBox, StelAction* action);

	//! Helper function to connect a QLineEdit to an integer StelProperty.
	//! @note This method also works with flag/enum types (TODO. Does it?)
	//! You should call something like setInputMask("009") for a 3-place numerical field.
	//! @warning If the action with \c propName is invalid/unregistered, or cannot be converted
	//! to the required datatype, the application will crash
	static void connectIntProperty(QLineEdit* lineEdit, const QString& propName);
	//! Helper function to connect a QSpinBox to an integer StelProperty.
	//! @note This method also works with flag/enum types
	//! @warning If the action with \c propName is invalid/unregistered, or cannot be converted
	//! to the required datatype, the application will crash
	static void connectIntProperty(QSpinBox* spinBox, const QString& propName);
	//! Helper function to connect a QComboBox to an integer StelProperty.
	//! The property is mapped to the selected index of the combobox.
	//! @note This method also works with flag/enum types
	//! @warning If the action with \c propName is invalid/unregistered, or cannot be converted
	//! to the required datatype, the application will crash
	static void connectIntProperty(QComboBox* comboBox, const QString& propName);
	//! Helper function to connect a QSlider to an double or float StelProperty
	//! @param slider The slider which should be connected
	//! @param propName The id of the StelProperty which should be connected
	//! @param minValue the int value associated with the minimal slider position
	//! @param maxValue the int value associated with the maximal slider position
	//! @warning If the action with \c propName is invalid/unregistered, or cannot be converted
	//! to the required datatype, the application will crash
	static void connectIntProperty(QSlider* slider, const QString& propName, int minValue, int maxValue);
	//! Helper function to connect a QDoubleSpinBox to an double or float StelProperty
	//! @warning If the action with \c propName is invalid/unregistered, or cannot be converted
	//! to the required datatype, the application will crash
	static void connectDoubleProperty(QDoubleSpinBox* spinBox, const QString& propName);
	//! Helper function to connect a QSlider to an double or float StelProperty
	//! @param slider The slider which should be connected
	//! @param propName The id of the StelProperty which should be connected
	//! @param minValue the double value associated with the minimal slider position
	//! @param maxValue the double value associated with the maximal slider position
	//! @warning If the action with \c propName is invalid/unregistered, or cannot be converted
	//! to the required datatype, the application will crash
	static void connectDoubleProperty(QSlider* slider, const QString& propName, double minValue, double maxValue);

	//! Helper function to connect a QComboBox to an QString StelProperty.
	//! The property is mapped to the selected string of the combobox.
	//! Make sure the string is available in the Combobox, else the first element may be chosen.
	//! @warning If the action with \c propName is invalid/unregistered, or cannot be converted
	//! to the required datatype, the application will crash
	static void connectStringProperty(QComboBox *comboBox, const QString &propName);

	//! Helper function to connect a checkbox to a bool StelProperty
	//! @warning If the action with \c propName is invalid/unregistered, or cannot be converted
	//! to the required datatype, the application will crash
	static void connectBoolProperty(QAbstractButton* checkBox, const QString& propName);
	//! Helper function to connect a groupbox to a bool StelProperty
	//! @warning If the action with \c propName is invalid/unregistered, or cannot be converted
	//! to the required datatype, the application will crash
	static void connectBoolProperty(QGroupBox *checkBox, const QString &propName);

	//! Prepare a QToolButton so that it can receive and handle askColor() connections properly.
	//! @param toolButton the QToolButton which shows the color
	//! @param propertyName a StelProperty name which must represent a color (coded as Vec3f)
	//! @param iniName the associated entry for config.ini, in the form group/name. Usually "color/some_feature_name_color".
	//! @param moduleName if the iniName is for a module (plugin)-specific ini file, add the module name here. The module needs an implementation of getSettings()
	//! @warning If the action with \c propName is invalid/unregistered, or cannot be converted
	//! to the required datatype, the application will crash
	void connectColorButton(QToolButton* button, QString propertyName, QString iniName, QString moduleName="");

	bool askConfirmation();

	//! The main dialog
	QWidget* dialog;
	class CustomProxy* proxy;
	//! The name should be set in derived classes' constructors and can be used to store and retrieve the panel locations.
	QString dialogName;

	//! A list of widgets where kinetic scrolling can be activated or deactivated
	//! The list must be filled once, in the constructor or init() of fillDialog() etc. functions.
	QList<QWidget *> kineticScrollingList;

protected slots:
	//! To be called by a connected QToolButton with a color background.
	//! This QToolButton needs properties "propName" and "iniName" which should be prepared using connectColorButton().
	void askColor();
	//! enable kinetic scrolling. This should be connected to StelApp's StelGui signal flagUseKineticScrollingChanged.
	void enableKineticScrolling(bool b);
	//! connect from StelApp to handle font and font size changes.
	void handleFontChanged();

private slots:
	void updateNightModeProperty();
};

class CustomProxy : public QGraphicsProxyWidget
{	private:
	Q_OBJECT
	public:
		CustomProxy(QGraphicsItem *parent = Q_NULLPTR, Qt::WindowFlags wFlags = Qt::Widget) : QGraphicsProxyWidget(parent, wFlags)
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
	signals: void sizeChanged(QSizeF);
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
		virtual void resizeEvent(QGraphicsSceneResizeEvent *event)
		{
			if (event->newSize() != event->oldSize())
			{
				emit sizeChanged(event->newSize());
			}
			QGraphicsProxyWidget::resizeEvent(event);
		}
};
#endif // STELDIALOG_HPP
