/*
 * Stellarium
 * Copyright (C) 2008 Guillaume Chereau
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
 
#ifndef _SEARCHDIALOG_HPP_
#define _SEARCHDIALOG_HPP_

#include <QObject>

// pre declaratio of the ui class
class Ui_searchDialogForm;

/**
	@class SearchDialog
	
	contains the search dialog widget 
*/
class SearchDialog : public QObject
{
Q_OBJECT
public:
	SearchDialog();
	virtual ~SearchDialog();
public slots:
	void setVisible(bool);
	void close();
	void onTextChanged(const QString& text);
	void gotoObject();
signals:
	void closed();
protected:
	QWidget* dialog;
	Ui_searchDialogForm* ui;
};

#endif // _SEARCHDIALOG_HPP_
