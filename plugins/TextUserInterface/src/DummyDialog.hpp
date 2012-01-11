/*
 * Copyright (C) 2009 Matthew Gates
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
 
#ifndef _DUMMYDIALOG_HPP_
#define _DUMMYDIALOG_HPP_ 1

#include "StelModule.hpp"
#include <QObject>
#include <QString>

//! The TextUserInterface wants to intercept all key presses including those which
//! are assigned to glocal key bindings in the main GUI definition (i.e. keys
//! used for actions which are associated with toolbar buttons.
//!
//! This DummyDialog class allows the plugin to put the QT focus on the dialog
//! and thus prevent global action key bindings from being handled by the main GUI
//!
//! This is adapted from the StelDialog class.
class DummyDialog : public QObject
{
	Q_OBJECT
public:
	DummyDialog(StelModule* eventHandler);
	~DummyDialog();

public slots:
	void setVisible(bool);
	void close();

signals:
        void visibleChanged(bool);

protected:
	bool eventFilter(QObject *obj, QEvent *event);
	void createDialogContent();
	class CustomProxy* proxy;
	QWidget* dialog;
	StelModule* evtHandler;
};

#endif /* _DUMMYDIALOG_HPP_ */

