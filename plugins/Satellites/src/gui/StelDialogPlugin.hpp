/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau, 2009 Matthew Gates
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
 
#ifndef _STELDIALOGLOCAL_HPP_
#define _STELDIALOGLOCAL_HPP_

#include <QObject>

//! @class StelDialogPlugin
//! Base class for all the GUI windows in stellarium
class StelDialogPlugin : public QObject
{
	Q_OBJECT
public:
	StelDialogPlugin();
	virtual ~StelDialogPlugin();
	//! Retranslate the content of the dialog
	virtual void languageChanged()=0;
	
public slots:
	void setVisible(bool);
	void close();
signals:
	void visibleChanged(bool);
	
protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent()=0;
	
	//! The main dialog
	QWidget* dialog;
	class CustomProxy* proxy;
};

#endif // _STELDIALOGLOCAL_HPP_

