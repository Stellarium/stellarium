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
 
#ifndef _STELDIALOGLOGBOOK_HPP_
#define _STELDIALOGLOGBOOK_HPP_

#include <QObject>

//! @class StelDialog
//! A local copy of StelDialog, the base class for all the GUI windows in Stellarium, included to allow the plug-in to be 
//! loaded dynamically on Windows. (An "Invalid access to memory location" error is thrown otherwise.)
class StelDialogLogBook : public QObject
{
	Q_OBJECT
public:
	StelDialogLogBook();
	virtual ~StelDialogLogBook();
	//! Retranslate the content of the dialog
	virtual void languageChanged()=0;
	
public slots:
	void setVisible(bool);
	void close();
signals:
	void visibleChanged(bool);
	void dialogClosed(StelDialogLogBook *);
	
protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent()=0;
	
	//! The main dialog
	QWidget* dialog;
	class CustomProxy* proxy;
};

#endif // _STELDIALOGLOGBOOK_HPP_
