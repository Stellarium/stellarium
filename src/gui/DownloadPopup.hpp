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
 
#ifndef _DOWNLOADPOPUP_HPP_
#define _DOWNLOADPOPUP_HPP_

#include <QObject>
#include "StelDialog.hpp"

class Ui_downloadPopupForm;

class DownloadPopup : public StelDialog
{
	Q_OBJECT;
public:
	DownloadPopup();
	virtual ~DownloadPopup();
	void languageChanged();
	//! Notify that the application style changed
	void styleChanged();
	
	void setText(const QString&);
protected:
	Ui_downloadPopupForm* ui;
	
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();

public slots:
	void cancel(void);
	void cont(void);

signals:
	void cancelClicked();
	void continueClicked();
};

#endif // _DOWNLOADPOPUP_HPP_
