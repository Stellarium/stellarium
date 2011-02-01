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
#include <QMap>
#include <QHash>
#include "StelDialog.hpp"
#include "VecMath.hpp"

// pre declaration of the ui class
class Ui_searchDialogForm;

//! @class CompletionLabel
//! Display a list of results matching the search string, and allow to
//! tab through those selections.
class CompletionLabel : public QLabel
{
	Q_OBJECT

public:
	CompletionLabel(QWidget* parent=0);
	~CompletionLabel();

	QString getSelected(void);
	void setValues(const QStringList&);
	bool isEmpty() const {return values.isEmpty();}
	void appendValues(const QStringList&);
	void clearValues();
	
public slots:
	void selectNext();
	void selectPrevious();
	void selectFirst();

private:
	void updateText();
	int selectedIdx;
	QStringList values;
};

//! @class SearchDialog
//! The sky object search dialog.
class SearchDialog : public StelDialog
{
	Q_OBJECT

public:
	SearchDialog();
	virtual ~SearchDialog();
	//! Notify that the application style changed
	void styleChanged();
	bool eventFilter(QObject *object, QEvent *event);
	void setSimpleStyle(bool b);
	
public slots:
	void languageChanged();
	//! Add auto focus of the edit line
	void setVisible(bool);

protected:
	Ui_searchDialogForm* ui;
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();

private slots:
	//! Called when the current simbad query status changes
	void onSimbadStatusChanged();
	//! Called when the user changed the input text
	void onTextChanged(const QString& text);
	
	void gotoObject();
	
	//! Called when the user edit the manual position controls
	void manualPositionChanged();
	
private:
	class SimbadSearcher* simbadSearcher;
	class SimbadLookupReply* simbadReply;
	QMap<QString, Vec3d> simbadResults;
	class StelObjectMgr* objectMgr;
	
	QString substituteGreek(const QString& keyString);
	QString getGreekLetterByName(const QString& potentialGreekLetterName);
	QHash<QString, QString> greekLetters;
};

#endif // _SEARCHDIALOG_HPP_

