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

//! @class StelDialog
//! Base class for all the GUI windows in Stellarium.
//! 
//! Windows in Stellarium are actually basic QWidgets that have to be wrapped in
//! a QGraphicsProxyWidget (CustomProxy) to be displayed by StelMainGraphicsView
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
class StelDialog : public QObject
{
	Q_OBJECT
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
signals:
	void visibleChanged(bool);

protected:
	//! Initialize the dialog widgets and connect the signals/slots.
	virtual void createDialogContent()=0;

	//! The main dialog
	QWidget* dialog;
	class CustomProxy* proxy;
};

#endif // _STELDIALOG_HPP_
