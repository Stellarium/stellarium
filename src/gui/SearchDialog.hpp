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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
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

struct stringLengthCompare
{
	bool operator()(const QString &s1, const QString &s2) const
	{
		return s1.length() < s2.length();
	}
};

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

QT_FORWARD_DECLARE_CLASS(QListWidgetItem)

//! @class SearchDialog
//! The sky object search dialog.
class SearchDialog : public StelDialog
{
	Q_OBJECT
	Q_ENUMS(CoordinateSystem)

public:
	//! @enum CoordinateSystem
	//! Available coordinate systems
	enum CoordinateSystem
	{
		equatorialJ2000,
		equatorial,
		horizontal,
		galactic,
		ecliptic,
		eclipticJ2000
	};

	SearchDialog(QObject* parent);
	virtual ~SearchDialog();
	//! Notify that the application style changed
	void styleChanged();
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void retranslate();
	//! This style only displays the text search field and the search button
	void setSimpleStyle();

	//! Set the current coordinate system
	void setCurrentCoordinateSystem(CoordinateSystem cs)
	{
		currentCoordinateSystem = cs;
	}
	//! Get the current coordinate system
	CoordinateSystem getCurrentCoordinateSystem() const
	{
		return currentCoordinateSystem;
	}
	//! Get the current coordinate system key
	QString getCurrentCoordinateSystemKey(void) const;
	//! Set the current coordinate system from its key
	void setCurrentCoordinateSystemKey(QString key);

	void setCoordinateSystem(int csID);
	void populateCoordinateSystemsList();
	void populateCoordinateAxis();

protected:
	Ui_searchDialogForm* ui;
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();

private slots:
	void greekLetterClicked();
	//! Called when the current simbad query status changes
	void onSimbadStatusChanged();
	//! Called when the user changed the input text
	void onSearchTextChanged(const QString& text);
	
	void gotoObject();
	void gotoObject(const QString& nameI18n);
	// for going from list views
	void gotoObject(QListWidgetItem* item);

	void searchListChanged(const QString& newText);
	
	//! Called when the user edit the manual position controls
	void manualPositionChanged();

	//! Whether to use SIMBAD for searches or not.
	void enableSimbadSearch(bool enable);

	//! Whether to use autofill for start of words or not.
	void enableStartOfWordsAutofill(bool enable);

	//! Set flagHasSelectedText as true, if search box has selected text
	void setHasSelectedFlag();

	//! Called when a SIMBAD server is selected in the list.
	void selectSimbadServer(int index);

	//! Called when new type of objects selected in list view tab
	void updateListWidget(int index);

	// retranslate/recreate tab
	void updateListTab();

	void showContextMenu(const QPoint &pt);

	void pasteAndGo();

private:
	class SimbadSearcher* simbadSearcher;
	class SimbadLookupReply* simbadReply;
	QMap<QString, Vec3d> simbadResults;
	class StelObjectMgr* objectMgr;
	class QSettings* conf;
	
	QString substituteGreek(const QString& keyString);
	QString getGreekLetterByName(const QString& potentialGreekLetterName);
	QHash<QString, QString> greekLetters;
	//! Used when substituting text with a Greek letter.
	bool flagHasSelectedText;

	bool useStartOfWords;
	bool useSimbad;
	//! URL of the server used for SIMBAD queries. 
	QString simbadServerUrl;
	void populateSimbadServerList();
	//! URL of the default SIMBAD server (Strasbourg).
	static const char* DEF_SIMBAD_URL;

	// The current coordinate system
	CoordinateSystem currentCoordinateSystem;

public:
	static QString extSearchText;

};

#endif // _SEARCHDIALOG_HPP_

