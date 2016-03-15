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

#ifndef _STELDIALOG_HPP_
#define _STELDIALOG_HPP_

#include <QObject>

class QAbstractButton;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QSlider;
class StelAction;

//! @class StelDialog
//! Base class for all the GUI windows in Stellarium.
//! 
//! Windows in Stellarium are actually basic QWidgets that have to be wrapped in
//! a QGraphicsProxyWidget (CustomProxy) to be displayed by StelMainView
//! (which is derived from QGraphicsView). See the Qt documentation for details.
//! 
//! The base widget needs to be populated with controls in the implementation
//! of the createDialogContent() function. This can be done either manually, or
//! by using a .ui file. See the Qt documentation on using Qt Designer .ui files
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
class StelDialog : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
public:
	StelDialog(QObject* parent=NULL);
	virtual ~StelDialog();

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
	void setVisible(bool);
	//! Closes the window (the window widget is not deleted, just not visible).
	void close();
	//! Adds dialog location to config.ini; should be connected in createDialogContent()
	void handleMovedTo(QPoint newPos);
signals:
	void visibleChanged(bool);

protected:
	//! Initialize the dialog widgets and connect the signals/slots.
	virtual void createDialogContent()=0;

	//! Helper function to connect a checkbox to the StelAction with the specified name
	static void connectCheckBox(QAbstractButton* checkBox,const QString& actionName);
	//! Helper function to connect a checkbox to the given StelAction
	static void connectCheckBox(QAbstractButton *checkBox, StelAction* action);

	//! Helper function to connect a QSpinBox to an integer StelProperty.
	//! @note This method also works with flag/enum types
	static void connectIntProperty(QSpinBox* spinBox, const QString& propName);
	//! Helper function to connect a QComboBox to an integer StelProperty.
	//! The property is mapped to the selected index of the combobox.
	//! @note This method also works with flag/enum types
	static void connectIntProperty(QComboBox* comboBox, const QString& propName);
	//! Helper function to connect a QDoubleSpinBox to an double or float StelProperty
	static void connectDoubleProperty(QDoubleSpinBox* spinBox, const QString& propName);
	//! Helper function to connect a QSlider to an double or float StelProperty
	//! @param minValue the double value associated with the minimal slider position
	//! @param maxValue the double value associated with the maximal slider position
	static void connectDoubleProperty(QSlider* slider, const QString& propName, double minValue, double maxValue);
	//! Helper function to connect a checkbox to a bool StelProperty
	static void connectBoolProperty(QAbstractButton* checkBox, const QString& propName);

	//! The main dialog
	QWidget* dialog;
	class CustomProxy* proxy;
	//! The name should be set in derived classes' constructors and can be used to store and retrieve the panel locations.
	QString dialogName;

#ifdef Q_OS_WIN
	//! Kinetic scrolling for lists.
	void installKineticScrolling(QList<QWidget *> addscroll);
#endif

private slots:
	void updateNightModeProperty();

};

#endif // _STELDIALOG_HPP_
