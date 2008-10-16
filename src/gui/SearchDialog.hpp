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
#include <QLabel>
#include "StelDialog.hpp"

// pre declaration of the ui class
class Ui_searchDialogForm;

//! @class CompletionLabel
//! used to display a few results matching the search string, and to
//! tab through those selections.
class CompletionLabel : public QLabel
{
	Q_OBJECT;

public:
	CompletionLabel(QWidget* parent=0);
	~CompletionLabel();

	QString getSelected(void);

public slots:
	void selectNext(void);
	void selectPrevious(void);
	void selectFirst(void);

private:
	void updateSelected(void);
	int selectedIdx;

};

//! @class SearchDialog
//! contains the search dialog widget 
class SearchDialog : public StelDialog
{
	Q_OBJECT;

public:
	SearchDialog();
	virtual ~SearchDialog();
	void languageChanged();
	//! Notify that the application style changed
	void styleChanged();
public slots:
	// Add auto focus of the edit line
	void setVisible(bool);
	void onTextChanged(const QString& text);
	void gotoObject();

private slots:
	void gotoPosition();

protected:
	Ui_searchDialogForm* ui;
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	bool eventFilter(QObject *object, QEvent *event);

};

#endif // _SEARCHDIALOG_HPP_
