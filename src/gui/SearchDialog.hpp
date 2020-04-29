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
 
#ifndef SEARCHDIALOG_HPP
#define SEARCHDIALOG_HPP

#include <QObject>
#include <QLabel>
#include <QMap>
#include <QHash>
#include "StelDialog.hpp"
#include "VecMath.hpp"

// pre declaration of the ui class
class Ui_searchDialogForm;
class QSortFilterProxyModel;
class QStringListModel;

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
	CompletionLabel(QWidget* parent=Q_NULLPTR);
	~CompletionLabel();

	QString getSelected(void) const;
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
	Q_PROPERTY(bool useSimbad       READ simbadSearchEnabled WRITE enableSimbadSearch  NOTIFY simbadUseChanged)
	Q_PROPERTY(int  simbadDist      READ getSimbadQueryDist  WRITE setSimbadQueryDist  NOTIFY simbadQueryDistChanged)
	Q_PROPERTY(int  simbadCount     READ getSimbadQueryCount WRITE setSimbadQueryCount NOTIFY simbadQueryCountChanged)
	Q_PROPERTY(bool simbadGetIds    READ getSimbadGetsIds    WRITE setSimbadGetsIds    NOTIFY simbadGetsIdsChanged)
	Q_PROPERTY(bool simbadGetSpec   READ getSimbadGetsSpec   WRITE setSimbadGetsSpec   NOTIFY simbadGetsSpecChanged)
	Q_PROPERTY(bool simbadGetMorpho READ getSimbadGetsMorpho WRITE setSimbadGetsMorpho NOTIFY simbadGetsMorphoChanged)
	Q_PROPERTY(bool simbadGetTypes  READ getSimbadGetsTypes  WRITE setSimbadGetsTypes  NOTIFY simbadGetsTypesChanged)
	Q_PROPERTY(bool simbadGetDims   READ getSimbadGetsDims   WRITE setSimbadGetsDims   NOTIFY simbadGetsDimsChanged)

	Q_ENUMS(CoordinateSystem)

public:
	//! Available coordinate systems
	enum CoordinateSystem
	{
		equatorialJ2000,
		equatorial,
		horizontal,
		galactic,
		supergalactic,
		ecliptic,
		eclipticJ2000
	};

	SearchDialog(QObject* parent);
	virtual ~SearchDialog();
	//! Notify that the application style changed
	void styleChanged();
	bool eventFilter(QObject *object, QEvent *event);


	//! Replaces all occurences of substrings describing Greek letters (i.e. "alpha", "beta", ...)
	//! with the actual Greek unicode characters.
	static QString substituteGreek(const QString& keyString);
	//! Returns the Greek unicode character for the specified letter string (i.e. "alpha", "beta", ...)
	static QString getGreekLetterByName(const QString& potentialGreekLetterName);
	//! URL of the default SIMBAD server (Strasbourg).
	static const char* DEF_SIMBAD_URL;

signals:
	void simbadUseChanged(bool use);
	void simbadQueryDistChanged(int dist);
	void simbadQueryCountChanged(int count);
	void simbadGetsIdsChanged(bool b);
	void simbadGetsSpecChanged(bool b);
	void simbadGetsDistChanged(bool b);
	void simbadGetsMorphoChanged(bool b);
	void simbadGetsTypesChanged(bool b);
	void simbadGetsDimsChanged(bool b);

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
	//! Query SIMBAD for data at the J2000.0 coordinates of the selected object.
	void lookupCoordinates();
	//! Signal handler
	void clearSimbadText(StelModule::StelModuleSelectAction);
	//! Called when the current simbad query status changes
	void onSimbadStatusChanged();
	//! Called when the user changed the input text
	void onSearchTextChanged(const QString& text);
	
	void gotoObject();
	void gotoObject(const QString& nameI18n);
	// for going from list views
	void gotoObject(const QModelIndex &modelIndex);

	void searchListClear();
	
	//! Called when the user edit the manual position controls
	void manualPositionChanged();

	//! Whether to use SIMBAD for searches or not.
	void enableSimbadSearch(bool enable);
	bool simbadSearchEnabled() const {return useSimbad;}

	//! Whether to use autofill for start of words or not.
	void enableStartOfWordsAutofill(bool enable);

	//! Whether to use lock position when coordinates are used or not.
	void enableLockPosition(bool enable);

	//! Whether to show FOV center marker when coordinates are used or not.
	void enableFOVCenterMarker(bool enable);

	//! Set flagHasSelectedText as true, if search box has selected text
	void setHasSelectedFlag();

	//! Called when a SIMBAD server is selected in the list.
	void selectSimbadServer(int index);

	//! Called when new type of objects selected in list view tab
	void updateListView(int index);

	// retranslate/recreate tab
	void updateListTab();

	void showContextMenu(const QPoint &pt);

	void pasteAndGo();

	void changeTab(int index);

	int  getSimbadQueryDist () const { return simbadDist;}
	int  getSimbadQueryCount() const { return simbadCount;}
	bool getSimbadGetsIds   () const { return simbadGetIds;}
	bool getSimbadGetsSpec  () const { return simbadGetSpec;}
	bool getSimbadGetsMorpho() const { return simbadGetMorpho;}
	bool getSimbadGetsTypes () const { return simbadGetTypes;}
	bool getSimbadGetsDims  () const { return simbadGetDims;}

	void setSimbadQueryDist(int dist);
	void setSimbadQueryCount(int count);
	void setSimbadGetsIds(bool b);
	void setSimbadGetsSpec(bool b);
	void setSimbadGetsMorpho(bool b);
	void setSimbadGetsTypes(bool b);
	void setSimbadGetsDims(bool b);

private:
	class SearchDialogStaticData
	{
	public:
		//! Greek letters and strings
		QHash<QString, QString> greekLetters;

		SearchDialogStaticData()
		{
			greekLetters.insert("alpha", QString(QChar(0x03B1)));
			greekLetters.insert("beta", QString(QChar(0x03B2)));
			greekLetters.insert("gamma", QString(QChar(0x03B3)));
			greekLetters.insert("delta", QString(QChar(0x03B4)));
			greekLetters.insert("epsilon", QString(QChar(0x03B5)));

			greekLetters.insert("zeta", QString(QChar(0x03B6)));
			greekLetters.insert("eta", QString(QChar(0x03B7)));
			greekLetters.insert("theta", QString(QChar(0x03B8)));
			greekLetters.insert("iota", QString(QChar(0x03B9)));
			greekLetters.insert("kappa", QString(QChar(0x03BA)));

			greekLetters.insert("lambda", QString(QChar(0x03BB)));
			greekLetters.insert("mu", QString(QChar(0x03BC)));
			greekLetters.insert("nu", QString(QChar(0x03BD)));
			greekLetters.insert("xi", QString(QChar(0x03BE)));
			greekLetters.insert("omicron", QString(QChar(0x03BF)));

			greekLetters.insert("pi", QString(QChar(0x03C0)));
			greekLetters.insert("rho", QString(QChar(0x03C1)));
			greekLetters.insert("sigma", QString(QChar(0x03C3))); // second lower-case sigma shouldn't affect anything
			greekLetters.insert("tau", QString(QChar(0x03C4)));
			greekLetters.insert("upsilon", QString(QChar(0x03C5)));

			greekLetters.insert("phi", QString(QChar(0x03C6)));
			greekLetters.insert("chi", QString(QChar(0x03C7)));
			greekLetters.insert("psi", QString(QChar(0x03C8)));
			greekLetters.insert("omega", QString(QChar(0x03C9)));
		}
	};
	static SearchDialogStaticData staticData;

	class SimbadSearcher* simbadSearcher;
	class SimbadLookupReply* simbadReply;
	QMap<QString, Vec3d> simbadResults;
	class StelObjectMgr* objectMgr;
	class QSettings* conf;
	QStringListModel* listModel;
	QSortFilterProxyModel *proxyModel;

	//! Used when substituting text with a Greek letter.
	bool flagHasSelectedText;

	bool useStartOfWords;
	bool useLockPosition;
	bool useSimbad;
	bool useFOVCenterMarker;
	bool fovCenterMarkerState;
	//! URL of the server used for SIMBAD queries. 
	QString simbadServerUrl;

	//! Properties for SIMBAD position query
	int  simbadDist;      //!< Distance from queried coordinates
	int  simbadCount;     //!< At max, retrieve so many results
	bool simbadGetIds;    //!< get all IDs for object
	bool simbadGetSpec;   //!< get spectral data
	bool simbadGetMorpho; //!< get morphological description
	bool simbadGetTypes;  //!< get object types
	bool simbadGetDims;   //!< get dimensions

	void populateSimbadServerList();

	// The current coordinate system
	CoordinateSystem currentCoordinateSystem;

public:
	static QString extSearchText;
};

#endif // _SEARCHDIALOG_HPP

