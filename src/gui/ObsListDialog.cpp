/*
 * Stellarium
 * Copyright (C) 2020 Jocelyn GIROD
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QUuid>

#include "StelTranslator.hpp"
#include "StelApp.hpp"
#include "StelObjectMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelLocationMgr.hpp"
#include "LandscapeMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelPropertyMgr.hpp"
#include "StelMovementMgr.hpp"
#include "CustomObjectMgr.hpp"
#include "LandscapeMgr.hpp"
#include "HighlightMgr.hpp"
#include "StarMgr.hpp"
#include "NebulaMgr.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelUtils.hpp"
#include "ObsListDialog.hpp"
#include "LabelMgr.hpp"

#include "ui_obsListDialog.h"

ObsListDialog::ObsListDialog(QObject *parent) :
	StelDialog("ObservingList", parent),
	ui(new Ui_obsListDialogForm()),
	core(StelApp::getInstance().getCore()),
	objectMgr(GETSTELMODULE(StelObjectMgr)),
	landscapeMgr(GETSTELMODULE(LandscapeMgr)),
	labelMgr(GETSTELMODULE(LabelMgr)),
	itemModel(new QStandardItemModel(0, ColumnCount)),
	observingListJsonPath(StelFileMgr::findFile("data", static_cast<StelFileMgr::Flags>(StelFileMgr::Directory | StelFileMgr::Writable)) + "/" + JSON_FILE_NAME),
	bookmarksJsonPath(    StelFileMgr::findFile("data", static_cast<StelFileMgr::Flags>(StelFileMgr::Directory | StelFileMgr::Writable)) + "/" + JSON_BOOKMARKS_FILE_NAME),
	tainted(false),
	isEditMode(false),
	isCreationMode(false)
{
	setObjectName("ObsListDialog");
	StelApp::getInstance().getStelPropertyManager()->registerObject(this);
	//Initialize the list of observing lists
	setObservingListHeaderNames();

	QSettings* conf  = StelApp::getInstance().getSettings();
	flagUseJD        = conf->value("bookmarks/useJD", false).toBool();
	flagUseLandscape = conf->value("bookmarks/useLandscape", false).toBool();
	flagUseLocation  = conf->value("bookmarks/useLocation", false).toBool();
	flagUseFov       = conf->value("bookmarks/useFOV", false).toBool();
}

ObsListDialog::~ObsListDialog() {
	// Only on exit we may need to write
	if (tainted)
	{
		// At this point we have added our lists to the observingLists map. Now update the jsonMap and store to file.
		QFile jsonFile(observingListJsonPath);
		if (!jsonFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
			qWarning() << "[ObservingList] bookmarks list can not be saved. A file can not be open for reading and writing:"
				   << QDir::toNativeSeparators(observingListJsonPath);
			messageBox(q_("Error"), q_("Cannot open observingLists.json to write"));
			return;
		}
		// Update the jsonMap and store
		jsonMap.insert(KEY_OBSERVING_LISTS, observingLists);
		jsonFile.resize(0);
		StelJsonParser::write(jsonMap, &jsonFile);
		jsonFile.flush();
		jsonFile.close();
	}

	delete ui;
	delete itemModel;
	ui = nullptr;
	itemModel = nullptr;
}

/*
 * Initialize the dialog widgets and connect the signals/slots.
*/
void ObsListDialog::createDialogContent()
{
	ui->setupUi(dialog);

	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	// Standard mode buttons: NewList/EditList/DeleteList
	connect(ui->newListButton,        SIGNAL(clicked()), this, SLOT(   newListButtonPressed()));
	connect(ui->editListButton,       SIGNAL(clicked()), this, SLOT(  editListButtonPressed()));
	connect(ui->deleteListButton,     SIGNAL(clicked()), this, SLOT(deleteListButtonPressed()));
	connect(ui->importListButton,     SIGNAL(clicked()), this, SLOT(importListButtonPressed()));
	connect(ui->exportListButton,     SIGNAL(clicked()), this, SLOT(exportListButtonPressed()));
	// Mark all objects of currentList in the sky
	connect(ui->highlightAllButton,   SIGNAL(clicked()), this, SLOT(highlightAll()));
	connect(ui->clearHighlightButton, SIGNAL(clicked()), this, SLOT(clearHighlights()));
	// Edits
	connect(ui->addObjectButton,      SIGNAL(clicked()), this, SLOT(   addObjectButtonPressed()));
	connect(ui->removeObjectButton,   SIGNAL(clicked()), this, SLOT(removeObjectButtonPressed()));
	connect(ui->saveButton,           SIGNAL(clicked()), this, SLOT(  saveButtonPressed()));
	connect(ui->cancelButton,         SIGNAL(clicked()), this, SLOT(cancelButtonPressed()));

	connect(ui->defaultListCheckBox, SIGNAL(clicked(bool)), this, SLOT(defaultClicked(bool)));

	// Allow loading one list entry
	connect(ui->treeView,           SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectAndGoToObject(QModelIndex)));

	switchEditMode(false, false); // start with view mode

	connectBoolProperty(ui->jdCheckBox,        "ObsListDialog.flagUseJD");
	connectBoolProperty(ui->locationCheckBox,  "ObsListDialog.flagUseLocation");
	connectBoolProperty(ui->landscapeCheckBox, "ObsListDialog.flagUseLandscape");
	connectBoolProperty(ui->fovCheckBox,       "ObsListDialog.flagUseFov");

	//obsListCombo settings: A change in the combobox loads the list.
	connect(ui->obsListComboBox, SIGNAL(activated(int)), this, SLOT(loadSelectedObservingList(int)));

	ui->treeView->setModel(itemModel);
	ui->treeView->header()->setSectionsMovable(false);
	ui->treeView->hideColumn(ColumnUUID);
	for (int c=ColumnDesignation; c<=ColumnLandscapeID; c++)
		ui->treeView->header()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
	ui->treeView->header()->setStretchLastSection(true);
	//Enable the sort for columns
	ui->treeView->setSortingEnabled(true);

	// Load all observing lists from JSON.
	// We need to load the global list only once!
	QFile jsonFile(observingListJsonPath);
	if (!jsonFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
		qWarning() << "[ObservingList] JSON list file can not be opened for reading and writing:"
			   << QDir::toNativeSeparators(observingListJsonPath);
		return;
	}
	if (jsonFile.size() > 0) {
		jsonMap = StelJsonParser::parse(jsonFile.readAll()).toMap();
		observingLists = jsonMap.value(KEY_OBSERVING_LISTS).toMap();
	}
	else
	{
		// begin with empty maps
		const QString olud=QUuid::createUuid().toString();
		jsonMap = { {KEY_SHORT_NAME       , SHORT_NAME_VALUE},
			    {KEY_VERSION          , FILE_VERSION},
			    {KEY_DEFAULT_LIST_OLUD, olud}};
		observingLists = QMap<QString, QVariant>();
		// Create one default empty list
		// Creation date
		const double JD = StelUtils::getJDFromSystem();
		const QString listCreationDate = StelUtils::julianDayToISO8601String(JD + core->getUTCOffset(JD) / 24.).replace("T", " ");
		QVariantMap emptyList = {
			// Name, description, current date for the list, current sorting
			{KEY_NAME,          qc_("new list", "default name for observing list if none is available")},
			{KEY_DESCRIPTION,   QString()},
			{KEY_SORTING,       QString()},
			{KEY_CREATION_DATE, listCreationDate },
			{KEY_LAST_EDIT,     listCreationDate }};

		observingLists.insert(olud, emptyList);
		jsonMap.insert(KEY_OBSERVING_LISTS, observingLists);
	}

	// For no regression we must take into account the legacy bookmarks file
	QFile jsonBookmarksFile(bookmarksJsonPath);
	if (jsonBookmarksFile.exists())
	{
		//qDebug() << "Old Bookmarks found: Try if we need to process/import them";
		if (!checkIfBookmarksListExists())
		{
			//qDebug() << "No bookmark list so far. Importing...";
			QHash<QString, observingListItem> bookmarksForImport=loadBookmarksFile(jsonBookmarksFile);
			saveBookmarksHashInObservingLists(bookmarksForImport);
			jsonMap.insert(KEY_OBSERVING_LISTS, observingLists); // Update the global map
		}
		//else
		//	qDebug() << "Bookmark list exists. We can skip the import.";

		jsonFile.resize(0);
		StelJsonParser::write(jsonMap, &jsonFile);
		jsonFile.flush();
		jsonFile.close();
		tainted=false;
	}
	// Now we certainly have a json file and have parsed everything that exists.
	defaultOlud = jsonMap.value(KEY_DEFAULT_LIST_OLUD).toString();
	loadListNames(); // also populate Combobox and make sure at least some defaultOlud exists.
	loadDefaultList();
}

/*
 * Check if bookmarks list already exists in observing list file,
 * in fact if the file of bookmarks has already be loaded.
*/
bool ObsListDialog::checkIfBookmarksListExists()
{
	QMapIterator<QString,QVariant>it(observingLists);
	while(it.hasNext())
	{
		it.next();
		QVariantMap map = it.value().toMap();
		QString listName = map.value(KEY_NAME).toString();

		if (BOOKMARKS_LIST_NAME == listName)
			return true;
	}
	return false;
}

/*
 * Retranslate dialog
*/
void ObsListDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setObservingListHeaderNames();
	}
}

/*
 * Set the header for the observing list table (obsListTreeView)
*/
void ObsListDialog::setObservingListHeaderNames()
{
	const QStringList headerStrings = {
		"UUID",                     // Hidden column
		q_("Object designation"),   // English name and/or catalog number
		q_("Object name"),          // Localized name
		q_("Type"),                 // Localized type description (not just class name)
		q_("Right ascension"),      // J2000.0 RA
		q_("Declination"),          // J2000.0 DE
		q_("Magnitude"),            // visual magnitude
		q_("Constellation"),        // IAU constellation code
		q_("Date"),                 // date string (if stored)
		q_("Location"),             // location (if stored)
		q_("Landscape")             // landscape ID (if stored)
	};
	Q_ASSERT(headerStrings.length()==ColumnCount);
	itemModel->setHorizontalHeaderLabels(headerStrings);
}

/*
 * Add row in the obsListListModel
*/
void ObsListDialog::addModelRow(const QString &olud, const QString &designation, const QString &nameI18n,
				const QString &typeI18n, const QString &ra,
                                const QString &dec, const QString &magnitude, const QString &constellation,
				const QString &date, const QString &location, const QString &landscapeID)
{
	const int number=itemModel->rowCount();
	QStandardItem *item = nullptr;

	item = new QStandardItem(olud);
	item->setEditable(false);
	itemModel->setItem(number, ColumnUUID, item);

	item = new QStandardItem(designation);
	item->setEditable(false);
	itemModel->setItem(number, ColumnDesignation, item);

	item = new QStandardItem(nameI18n);
	item->setEditable(false);
	itemModel->setItem(number, ColumnNameI18n, item);

	item = new QStandardItem(typeI18n);
	item->setEditable(false);
	itemModel->setItem(number, ColumnType, item);

	item = new QStandardItem(ra);
	item->setEditable(false);
	itemModel->setItem(number, ColumnRa, item);

	item = new QStandardItem(dec);
	item->setEditable(false);
	itemModel->setItem(number, ColumnDec, item);

	item = new QStandardItem(magnitude);
	item->setEditable(false);
	itemModel->setItem(number, ColumnMagnitude, item);

	item = new QStandardItem(constellation);
	item->setEditable(false);
	itemModel->setItem(number, ColumnConstellation, item);

	item = new QStandardItem(date);
	item->setEditable(false);
	itemModel->setItem(number, ColumnDate, item);

	item = new QStandardItem(location);
	item->setEditable(false);
	itemModel->setItem(number, ColumnLocation, item);

	item = new QStandardItem(landscapeID);
	item->setEditable(false);
	itemModel->setItem(number, ColumnLandscapeID, item);

	for (int i = 0; i < ColumnCount; ++i)
		ui->treeView->resizeColumnToContents(i);
}

/*
 * Load the lists names from jsonMap,
 * Populate the list names into combo box and extract defaultOlud
*/
void ObsListDialog::loadListNames()
{
	listNames.clear();
	QVariantMap::iterator i;
	for (i = observingLists.begin(); i != observingLists.end(); ++i) {
		if (i.value().canConvert<QVariantMap>())
		{
			QVariant var = i.value();
			QVariantMap data = var.value<QVariantMap>();
			QString listName = data.value(KEY_NAME).toString();
			listNames.append(listName);
		}
	}
	listNames.sort(Qt::CaseInsensitive);
	ui->obsListComboBox->clear();
	ui->obsListComboBox->addItems(listNames);

	// Now add the item data into the ComboBox
	for (i = observingLists.begin(); i != observingLists.end(); ++i) {
		const QString &listUuid = i.key();
		if (i.value().canConvert<QVariantMap>()) // if this looks like an actual obsList?
		{
			QVariant var = i.value();
			auto data = var.value<QVariantMap>();
			QString listName = data.value(KEY_NAME).toString();
			int idx=ui->obsListComboBox->findText(listName);
			Q_ASSERT(idx!=-1);
			ui->obsListComboBox->setItemData(idx, listUuid);
		}
	}
	// If defaultOlud list not found, set first list as default.
	if (!observingLists.contains(defaultOlud))
	{
		qDebug() << "populateListNameInComboBox: Cannot find defaultListOlud" << defaultOlud << ". Setting to first list.";
		defaultOlud = observingLists.firstKey();
		jsonMap.insert(KEY_DEFAULT_LIST_OLUD, defaultOlud);
		tainted=true;
	}
}

/*
 * Load the default list
*/
void ObsListDialog::loadDefaultList()
{
	if (defaultOlud != "")
	{
		int index = ui->obsListComboBox->findData(defaultOlud);
		if (index != -1)
		{
			ui->obsListComboBox->setCurrentIndex(index);
			selectedOlud = defaultOlud;
		}
	}
	else
	{
		// If there is no default list we load the current, or the first list in the combo box
		int currentIndex = ui->obsListComboBox->currentIndex();
		if (currentIndex != -1)
		{
			currentIndex=0;
			ui->obsListComboBox->setCurrentIndex(0);
		}
		selectedOlud = ui->obsListComboBox->itemData(currentIndex).toString();
		defaultOlud = selectedOlud;
		jsonMap.insert(KEY_DEFAULT_LIST_OLUD, defaultOlud);
		tainted=true;
	}
	loadSelectedList();
}

/*
 * Load the selected observing list (selectedOlud) into the dialog.
*/
void ObsListDialog::loadSelectedList()
{
	// At this point selectedOlud must be set
	Q_ASSERT(selectedOlud.length()>0);

	// We must keep selection for the user. It is not enough to store/restore the existingSelection.
	// The QList<StelObjectP> objects are apparently volatile. We must retrieve the actual object.
	const QList<StelObjectP>&existingSelection = objectMgr->getSelectedObject();
	QList<StelObjectP> existingSelectionToRestore;
	if (existingSelection.length()>0)
	{
		existingSelectionToRestore.append(existingSelection.at(0));
	}

	// Check or not default list checkbox information
	ui->defaultListCheckBox->setChecked(selectedOlud == defaultOlud);

	QVariantMap observingListMap = observingLists.value(selectedOlud).toMap();
	QVariantList listOfObjects;

	// Display description and creation date
	currentListName=observingListMap.value(KEY_NAME).toString();
	ui->listNameLineEdit->setText(currentListName);
	ui->descriptionLineEdit->setText(observingListMap.value(KEY_DESCRIPTION).toString());
	ui->creationDateLineEdit->setText(observingListMap.value(KEY_CREATION_DATE).toString());
	ui->lastEditLineEdit->setText(observingListMap.value(KEY_LAST_EDIT, observingListMap.value(KEY_CREATION_DATE)).toString());

	if (observingListMap.value(KEY_OBJECTS).canConvert<QVariantList>())
	{
		QVariant data = observingListMap.value(KEY_OBJECTS);
		listOfObjects = data.value<QVariantList>();
	}
	else
		qCritical() << "[ObservingList] conversion error";

	// Clear model
	itemModel->removeRows(0, itemModel->rowCount()); // don't use clear() here!
	currentItemCollection.clear();

	if (!listOfObjects.isEmpty())
	{
		for (const QVariant &object: listOfObjects)
		{
			if (object.canConvert<QVariantMap>())
			{
				QVariantMap objectMap = object.value<QVariantMap>();

				observingListItem item;
				const QString objectUUID = QUuid::createUuid().toString();
				item.designation = objectMap.value(KEY_DESIGNATION).toString();  // This is the common name or catalog number (with catalog ID)
				item.name = objectMap.value(KEY_NAME).toString();          // Preliminary: Do not rely on name in the JSON file. It may have changed in Stellarium's name lists! Retrieve name from the actual object later.
				item.nameI18n = objectMap.value(KEY_NAME_I18N).toString(); // Preliminary: Do not rely on translated name in the JSON file. It may be in the wrong language or may have changed in Stellarium's name lists! Retrieve translated name from the actual object later.
				item.objClass = objectMap.value(KEY_TYPE).toString();
				item.objTypeI18n  = objectMap.value(KEY_OBJECTS_TYPE).toString(); // Preliminary: Do not rely on this translated string! Re-retrieve later
				item.ra  = objectMap.value(KEY_RA).toString();
				item.dec = objectMap.value(KEY_DEC).toString();

				if (objectMgr->findAndSelect(item.designation, item.objClass) && !objectMgr->getSelectedObject().isEmpty())
				{
					//qDebug() << "ObsList: found an object of objClass" << item.objClass << "for" << item.designation;
					const QList<StelObjectP> &selectedObject = objectMgr->getSelectedObject();
					double ra, dec;
					StelUtils::rectToSphe(&ra, &dec, selectedObject[0]->getJ2000EquatorialPos(core));

					if (item.ra.isEmpty())
						item.ra = StelUtils::radToHmsStr(ra, false).trimmed();
					if (item.dec.isEmpty())
						item.dec = StelUtils::radToDmsStr(dec, false).trimmed();
					item.objTypeI18n = selectedObject[0]->getObjectTypeI18n();
					item.name=selectedObject[0]->getEnglishName();
					item.nameI18n=selectedObject[0]->getNameI18n();
				}
				else // THEREFORE: repeat the same code with findAndSelect with any type.
					if (objectMgr->findAndSelect(item.designation) && !objectMgr->getSelectedObject().isEmpty())
				{
					const QList<StelObjectP> &selectedObject = objectMgr->getSelectedObject();
					double ra, dec;
					StelUtils::rectToSphe(&ra, &dec, selectedObject[0]->getJ2000EquatorialPos(core));

					if (item.ra.isEmpty())
						item.ra = StelUtils::radToHmsStr(ra, false).trimmed();
					if (item.dec.isEmpty())
						item.dec = StelUtils::radToDmsStr(dec, false).trimmed();
					//qDebug() << "Changing item.objClass " << item.objClass << "to" << selectedObject[0]->getType();
					item.objClass = selectedObject[0]->getType();
					item.objTypeI18n = selectedObject[0]->getObjectTypeI18n();
					item.name=selectedObject[0]->getEnglishName();
					item.nameI18n=selectedObject[0]->getNameI18n();
				}
				else
				{
					qWarning() << "[ObservingList] object: " << item.designation << " not found or empty. Cannot set item.objClass !";
					qWarning() << "item.objType given as:" << item.objTypeI18n;
				}

				item.magnitude = objectMap.value(KEY_MAGNITUDE).toString();
				item.constellation = objectMap.value(KEY_CONSTELLATION).toString();

				// Julian Day / Date
				item.jd = objectMap.value(KEY_JD).toDouble();
				QString dateStr;
				if (item.jd != 0.)
					dateStr = StelUtils::julianDayToISO8601String(item.jd + core->getUTCOffset(item.jd) / 24.).replace("T", " ");

				// Location, landscapeID, FoV (may be empty)
				item.location = objectMap.value(KEY_LOCATION).toString();
				item.landscapeID = objectMap.value(KEY_LANDSCAPE_ID).toString();
				item.fov = objectMap.value(KEY_FOV).toDouble();

				// Visible flag
				item.isVisibleMarker = objectMap.value(KEY_IS_VISIBLE_MARKER).toBool();

				QString LocationStr;
				if (!item.location.isEmpty())
				{
					StelLocation loc=StelApp::getInstance().getLocationMgr().locationForString(item.location);
					LocationStr=loc.name;
				}

				addModelRow(objectUUID,
					    item.designation,
					    item.nameI18n,
					    item.objTypeI18n,
					    item.ra,
					    item.dec,
					    item.magnitude,
					    item.constellation,
					    dateStr,
					    LocationStr,
					    item.landscapeID);

				currentItemCollection.insert(objectUUID, item);
			}
			else
			{
				qCritical() << "[ObservingList] conversion error";
				return;
			}
		}
	}

	// Sorting for the objects list.
	QString sortingBy = observingListMap.value(KEY_SORTING).toString();
	if (!sortingBy.isEmpty())
		sortObsListTreeViewByColumnName(sortingBy);

	// Restore selection that was active before calling this
	if (existingSelectionToRestore.length()>0)
		objectMgr->setSelectedObject(existingSelectionToRestore, StelModule::ReplaceSelection);
	else
		objectMgr->unSelect();
}

/*
 * Load the bookmarks of bookmarks.json file into observing lists file
*/
QHash<QString, ObsListDialog::observingListItem> ObsListDialog::loadBookmarksFile(QFile &file)
{
	//qDebug() << "LOADING OLD BOOKMARKS...";

	QHash<QString, observingListItem> bookmarksItemHash;

	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "[ObservingList] cannot open" << QDir::toNativeSeparators(bookmarksJsonPath);
	}
	else
	{
		const double currentJD=core->getJDOfLastJDUpdate();// Restore at end
		const qint64 millis = core->getMilliSecondsOfLastJDUpdate();

		// We must keep selection for the user!
		const QList<StelObjectP>&existingSelection = objectMgr->getSelectedObject();
		QList<StelObjectP> existingSelectionToRestore;
		if (existingSelection.length()>0)
		{
			existingSelectionToRestore.append(existingSelection.at(0));
		}

		try {
			QVariantMap map = StelJsonParser::parse(file.readAll()).toMap();
			file.close();
			QVariantMap bookmarksMap = map.value(KEY_BOOKMARKS).toMap();

			QMapIterator<QString,QVariant>it(bookmarksMap);
			while(it.hasNext())
			{
				it.next();

				QVariantMap bookmarkMap = it.value().toMap();
				observingListItem item;

				// Name
				item.designation = bookmarkMap.value(KEY_NAME).toString();
				QString nameI18n = bookmarkMap.value(KEY_NAME_I18N).toString();
				item.nameI18n = nameI18n.isEmpty() ? DASH : nameI18n;

				// We need to select the object to add additional information that is not in the Bookmark file
				if (objectMgr->findAndSelect(item.designation) && !objectMgr->getSelectedObject().isEmpty()) {
					const QList<StelObjectP> &selectedObject = objectMgr->getSelectedObject();

					item.objClass = selectedObject[0]->getType(); // Assign class name
					item.objTypeI18n = selectedObject[0]->getObjectTypeI18n(); // Assign a detailed object type description
					item.name = selectedObject[0]->getEnglishName();
					item.nameI18n = selectedObject[0]->getNameI18n();
					item.jd = bookmarkMap.value(KEY_JD).toDouble();
					if (item.jd!=0.)
					{
						core->setJD(item.jd);
						core->update(0.); // Force position updates
					}

					// Ra & Dec - ra and dec are not empty in case of Custom Object
					item.ra = bookmarkMap.value(KEY_RA).toString();
					item.dec = bookmarkMap.value(KEY_DEC).toString();
					if (item.ra.isEmpty() || item.dec.isEmpty()) {
						double ra, dec;
						StelUtils::rectToSphe(&ra, &dec, selectedObject[0]->getJ2000EquatorialPos(core));
						item.ra = StelUtils::radToHmsStr(ra, false).trimmed();
						item.dec = StelUtils::radToDmsStr(dec, false).trimmed();
					}
					item.magnitude = getMagnitude(selectedObject, core);
					// Several data items were not part of the original bookmarks, so we have no entry.
					const Vec3d posNow = selectedObject[0]->getEquinoxEquatorialPos(core);
					item.constellation = core->getIAUConstellation(posNow);
					item.location = bookmarkMap.value(KEY_LOCATION).toString();
					// No landscape in original bookmarks. Just leave empty.
					//item.landscapeID = bookmarkData.value(KEY_LANDSCAPE_ID).toString();
					double bmFov = bookmarkMap.value(KEY_FOV).toDouble();
					if (bmFov>1.e-10) // FoV may look like 3.121251e-310, which is bogus.
						item.fov=bmFov;
					item.isVisibleMarker = bookmarkMap.value(KEY_IS_VISIBLE_MARKER, false).toBool();

					bookmarksItemHash.insert(it.key(), item);
				}
			}
		}
		catch (std::runtime_error &e)
		{
			qWarning() << "[ObservingList] Load bookmarks in observing list: File format is wrong! Error: " << e.what();
		}
		// Restore selection that was active before calling this
		if (existingSelectionToRestore.length()>0)
			objectMgr->setSelectedObject(existingSelectionToRestore, StelModule::ReplaceSelection);
		else
			objectMgr->unSelect();

		core->setJD(currentJD);
		core->setMilliSecondsOfLastJDUpdate(millis); // restore millis.
		core->update(0); // enforce update to the previous positions
	}
	return bookmarksItemHash;
}

/*
 * Save the bookmarks into observingLists QVariantMap
*/
QString ObsListDialog::saveBookmarksHashInObservingLists(const QHash<QString, observingListItem> &bookmarksHash)
{
	// Creation date
	double JD = StelUtils::getJDFromSystem(); // Mark with current system time
	QString listCreationDate = StelUtils::julianDayToISO8601String(JD + core->getUTCOffset(JD) / 24.).replace("T", " ");

	QVariantMap bookmarksObsList = {
		{KEY_NAME,          BOOKMARKS_LIST_NAME},
		{KEY_DESCRIPTION,   BOOKMARKS_LIST_DESCRIPTION},
		{KEY_CREATION_DATE, listCreationDate},
		{KEY_LAST_EDIT,     listCreationDate},
		{KEY_SORTING,       SORTING_BY_NAME}};

	// Add actual list of (former) bookmark entries
	QVariantList objects;
	QHashIterator<QString, observingListItem> it(bookmarksHash);
	while (it.hasNext())
	{
		it.next();
		observingListItem item = it.value();
		objects.push_back(item.toVariantMap());
	}
	bookmarksObsList.insert(KEY_OBJECTS, objects);

	QList<QString> keys = bookmarksHash.keys();
	QString bookmarkListOlud= (keys.empty() ? QUuid::createUuid().toString() : keys.at(0));

	observingLists.insert(bookmarkListOlud, bookmarksObsList);
	return bookmarkListOlud;
}

/*
 * Select and go to object
*/
void ObsListDialog::selectAndGoToObject(QModelIndex index)
{
	QStandardItem *selectedItem = itemModel->itemFromIndex(index);
	int rowNumber = selectedItem->row();

	QStandardItem *uuidItem = itemModel->item(rowNumber, ColumnUUID);
	QString itemUuid = uuidItem->text();
	observingListItem item = currentItemCollection.value(itemUuid);

	// Load landscape/location before dealing with the object: It could be a view from another planet!
	// We load stored landscape/location if the respective checkbox is checked.
	if (getFlagUseLandscape() && !item.landscapeID.isEmpty())
		GETSTELMODULE(LandscapeMgr)->setCurrentLandscapeID(item.landscapeID, 0);
	if (getFlagUseLocation() && !item.location.isEmpty())
	{
		StelLocation loc=StelApp::getInstance().getLocationMgr().locationForString(item.location);
		if (loc.isValid())
			core->moveObserverTo(loc);
		else
			qWarning() << "ObservingLists: Cannot retrieve valid location for" << item.location;
	}
	// We also use stored jd only if the checkbox JD is checked.
	if (getFlagUseJD() && item.jd != 0.0)
		core->setJD(item.jd);

	StelMovementMgr *mvmgr = GETSTELMODULE(StelMovementMgr);
	//objectMgr->unSelect();

	bool objectFound = objectMgr->findAndSelect(item.designation); // TODO: We should prefer to use findAndSelect(item.designation, item.objClass). But what are the implications for markers?
	if (!item.ra.isEmpty() && !item.dec.isEmpty() && (!objectFound || item.designation.contains("marker", Qt::CaseInsensitive))) {
		Vec3d pos;
		StelUtils::spheToRect(StelUtils::getDecAngle(item.ra.trimmed()), StelUtils::getDecAngle(item.dec.trimmed()), pos);
		if (item.designation.contains("marker", Qt::CaseInsensitive))
		{
			// Add a custom object on the sky
			GETSTELMODULE(CustomObjectMgr)->addCustomObject(item.designation, pos, item.isVisibleMarker);
			objectFound = objectMgr->findAndSelect(item.designation);
		}
		else
		{
			// The unnamed stars
			StelObjectP sobj=nullptr;
			const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
			double fov = (item.fov > 0.0 ? item.fov : 5.0);

			mvmgr->zoomTo(fov, 0.0);
			mvmgr->moveToJ2000(pos, mvmgr->mountFrameToJ2000(Vec3d(0., 0., 1.)), 0.0);

			QList<StelObjectP> candidates = GETSTELMODULE(StarMgr)->searchAround(pos, 0.5, core);
			if (candidates.empty()) { // The FOV is too big, let's reduce it to see dimmer objects.
				mvmgr->zoomTo(0.5 * fov, 0.0);
				candidates = GETSTELMODULE(StarMgr)->searchAround(pos, 0.5, core);
			}

			Vec3d winpos;
			prj->project(pos, winpos);
			double xpos = winpos[0];
			double ypos = winpos[1];
			double bestCandDistSq = 1.e6;
			for (const auto &obj: candidates)
			{
				prj->project(obj->getJ2000EquatorialPos(core), winpos);
				double sqrDistance = (xpos - winpos[0]) * (xpos - winpos[0]) + (ypos - winpos[1]) * (ypos - winpos[1]);
				if (sqrDistance < bestCandDistSq)
				{
					bestCandDistSq = sqrDistance;
					sobj = obj;
				}
			}
			if (sobj)
				objectFound = objectMgr->setSelectedObject(sobj);
		}
	}

	if (objectFound) {
		const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
		if (!newSelected.empty())
		{
			const float amd = mvmgr->getAutoMoveDuration();
			mvmgr->moveToObject(newSelected[0], amd);
			mvmgr->setFlagTracking(true);
			// We load stored FoV if the FoV checkbox is checked.
			if (getFlagUseFov() && item.fov>1e-10) {
				GETSTELMODULE(StelMovementMgr)->zoomTo(item.fov, amd);
			}
		}
	}
}

/*
 * Method called when a list name is selected in the combobox
*/
void ObsListDialog::loadSelectedObservingList(int selectedIndex)
{
	//ui->editListButton->setEnabled(true);
	//ui->deleteListButton->setEnabled(true);
	selectedOlud = ui->obsListComboBox->itemData(selectedIndex).toString();
	loadSelectedList();
}

/*
 * Slot for button highlightAllButton: show all objects of active list.
*/
void ObsListDialog::highlightAll()
{
	// We must keep selection for the user. It is not enough to store/restore the existingSelection.
	// The QList<StelObjectP> objects are apparently volatile. We must retrieve the actual object.
	const QList<StelObjectP>&existingSelection = objectMgr->getSelectedObject();
	QList<StelObjectP> existingSelectionToRestore;
	if (existingSelection.length()>0)
	{
		existingSelectionToRestore.append(existingSelection.at(0));
	}

	QList<Vec3d> highlights;
	clearHighlights(); // Enable fool protection
	const int fontSize = StelApp::getInstance().getScreenFontSize();
	HighlightMgr *hlMgr = GETSTELMODULE(HighlightMgr);
	const QString color = hlMgr->getColor().toHtmlColor();
	float distance = hlMgr->getMarkersSize();

	for (const auto &item: qAsConst(currentItemCollection)) {
		const QString name = item.designation;
		const QString raStr = item.ra.trimmed();
		const QString decStr = item.dec.trimmed();

		Vec3d pos;
		bool usablePosition;
		if (!raStr.isEmpty() && !decStr.isEmpty())
		{
			StelUtils::spheToRect(StelUtils::getDecAngle(raStr), StelUtils::getDecAngle(decStr), pos);
			usablePosition = true;
		}
		else
		{
			usablePosition = objectMgr->findAndSelect(name);
			if (usablePosition) {
				const QList<StelObjectP> &selected = objectMgr->getSelectedObject();
				pos = selected[0]->getJ2000EquatorialPos(core);
			}
		}
		if (usablePosition)
			highlights.append(pos);

		// Add labels for named highlights (name in top right corner)
		if (!raStr.isEmpty() && !decStr.isEmpty()) // We may have a position for a timestamped event
			highlightLabelIDs.append(labelMgr->labelEquatorial(name, raStr, decStr, true, fontSize, color, "NE", distance));
		else
			highlightLabelIDs.append(labelMgr->labelObject(name, name, true, fontSize, color, "NE", distance));
	}

	hlMgr->fillHighlightList(highlights);

	// Restore selection that was active before calling this
	if (existingSelectionToRestore.length()>0)
		objectMgr->setSelectedObject(existingSelectionToRestore, StelModule::ReplaceSelection);
	else
		objectMgr->unSelect();
}

/*
 * Clear highlights
*/
void ObsListDialog::clearHighlights()
{
	GETSTELMODULE(HighlightMgr)->cleanHighlightList();
	// Clear labels
	for (int l: highlightLabelIDs) {
		labelMgr->deleteLabel(l);
	}
	highlightLabelIDs.clear();
}

/*
 * Slot for button newListButton
*/
void ObsListDialog::newListButtonPressed()
{
	selectedOlud=QUuid::createUuid().toString();
	itemModel->removeRows(0, itemModel->rowCount()); // don't use clear() here!
	currentItemCollection.clear();

	//ui->treeView->clearSelection(); ???
	switchEditMode(true, true);

	ui->listNameLineEdit->setText(q_("New Observation List"));
	ui->stelWindowTitle->setText(q_("Observing list creation mode"));
}
/*
 * Slot for editButton
*/
void ObsListDialog::editListButtonPressed()
{
	Q_ASSERT(!selectedOlud.isEmpty());

	if (!selectedOlud.isEmpty())
	{
		switchEditMode(true, false);

		ui->stelWindowTitle->setText(q_("Observing list editor mode"));
	}
	else
	{
		qCritical() << "ObsListDialog::editListButtonPressed(): selectedOlud is empty";
		messageBox(q_("Error"), q_("selectedOlud empty. This is a bug"));
	}
}

/*
 * Slot for button obsListExportListButton
 */
void ObsListDialog::exportListButtonPressed()
{
	static const QString filter = "Stellarium Single Observing List (*.sol);;Stellarium Observing List (*.ol)";
	const QString destinationDir=StelApp::getInstance().getSettings()->value("main/observinglists_dir", QDir::homePath()).toString();
	QString selectedFilter = "Stellarium Single Observing List (*.sol)";
	QString exportListJsonPath = QFileDialog::getSaveFileName(nullptr, q_("Export observing list as..."),
							      destinationDir + "/" + JSON_FILE_BASENAME + "_" + currentListName + ".sol", filter, &selectedFilter);

	QFile jsonFile(exportListJsonPath);
	if (!jsonFile.open(QIODevice::ReadWrite | QIODevice::Text))
	{
		qWarning() << "[ObservingList Creation/Edition] Error exporting observing list. "
			   << "File cannot be opened for reading and writing:"
			   << QDir::toNativeSeparators(exportListJsonPath);
		messageBox(q_("Error"), q_("Cannot export. See logfile for details."));
		return;
	}

	jsonFile.resize(0);
	QFileInfo fi(exportListJsonPath);
	if (fi.suffix()=="sol")
	{
		// Prepare a new json-able map
		QVariantMap exportJsonMap={
			{KEY_DEFAULT_LIST_OLUD, ""}, // Do not set a default in this list!
			{KEY_SHORT_NAME, SHORT_NAME_VALUE},
			{KEY_VERSION, FILE_VERSION}};

		QVariantMap currentListMap={{selectedOlud, observingLists.value(selectedOlud).toMap()}};
		//QVariantMap oneListMap={{KEY_OBSERVING_LISTS, currentListMap}};
		//exportJsonMap.insert(KEY_OBSERVING_LISTS, oneListMap);
		exportJsonMap.insert(KEY_OBSERVING_LISTS, currentListMap);
		StelJsonParser::write(exportJsonMap, &jsonFile);
	}
	else
	{
		// just export complete map.
		StelJsonParser::write(jsonMap, &jsonFile);
	}

	jsonFile.flush();
	jsonFile.close();
}

/*
 * Slot for button obsListImportListButton
 */
void ObsListDialog::importListButtonPressed()
{
	static const QString filter = "Stellarium Single Observing List (*.sol);;Stellarium Observing List (*.ol);;Stellarium Legacy JSON Observing List or Bookmarks (*.json)";
	const QString destinationDir=StelApp::getInstance().getSettings()->value("main/observinglists_dir", QDir::homePath()).toString();
	QString fileToImportJsonPath = QFileDialog::getOpenFileName(nullptr, q_("Import observing list"),
								    destinationDir,
								    filter);
	QVariantMap map;
	QFile jsonFile(fileToImportJsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "[ObservingList Import] cannot open"
			   << QDir::toNativeSeparators(jsonFile.fileName());
		messageBox(q_("Error"), q_("Cannot open selected file for import"));
		return;
	}
	else
	{
		try {
			map = StelJsonParser::parse(jsonFile.readAll()).toMap();
			jsonFile.close();

			if (map.contains(KEY_OBSERVING_LISTS))
			{
				// Case of observingList import: Import all lists from that file! A .sol only has one list but is else structured identically.
				const QVariantMap observingListMapToImport = map.value(KEY_OBSERVING_LISTS).toMap();
				if (observingListMapToImport.isEmpty())
				{
					qWarning() << "[ObservingList Creation/Edition import] empty list:" << fileToImportJsonPath;
					messageBox(q_("Error"), q_("Empty list."));
					return;
				}
				else
				{
					QVariantMap::const_iterator it;
					for (it = observingListMapToImport.begin(); it != observingListMapToImport.end(); it++) {
						if (it.value().canConvert<QVariantMap>())
						{
							// check here to avoid overwriting of existing lists
							bool overwrite=true;
							if (observingLists.contains(it.key())) // Same OLUD?
							{
								QVariantMap importedMap=it.value().toMap(); // This is a map of {{UUID, QMap},...}
								QString importedName=importedMap.value(KEY_NAME).toString();
								QString importedDate=importedMap.value(KEY_CREATION_DATE).toString();
								QString importedLastEditDate=importedMap.value(KEY_LAST_EDIT, importedMap.value(KEY_CREATION_DATE)).toString();
								qDebug() << "Imported Map named:" << importedName << "created" << importedDate << "changed" << importedLastEditDate << ":" << importedMap;
								QVariantMap existingMap=observingLists.value(it.key()).toMap();
								QString existingName=existingMap.value(KEY_NAME).toString();
								QString existingDate=existingMap.value(KEY_CREATION_DATE).toString();
								QString existingLastEdit=existingMap.value(KEY_LAST_EDIT, existingMap.value(KEY_CREATION_DATE)).toString();
								QString message=QString(q_("A list named '%1', created %2 and last modified %3 would overwrite your existing list '%4', dated %5 and last modified %6. Accept?")).arg(importedName, importedDate, importedLastEditDate, existingName, existingDate, existingLastEdit);
								overwrite=askConfirmation(message);
							}
							if (overwrite)
							{
								observingLists.insert(it.key(), it.value());
								selectedOlud=it.key();
							}
						}
					}
				}
			}
			else if (map.contains(KEY_BOOKMARKS))
			{
				// Case of legacy bookmarks import
				QVariantMap bookmarksListMap = map.value(KEY_BOOKMARKS).toMap();
				if (bookmarksListMap.isEmpty())
				{
					qWarning() << "[ObservingList Creation/Edition import] the file is empty or doesn't contain legacy bookmarks.";
					messageBox(q_("Error"), q_("The file is empty or doesn't contain legacy bookmarks."));
					return;
				}
				else
				{
					QHash<QString, ObsListDialog::observingListItem> bookmarksHash=loadBookmarksFile(jsonFile);
					// Put them to the main list. Note that this may create another list named "bookmarks list", however, with a different OLUD than the existing.
					selectedOlud=saveBookmarksHashInObservingLists(bookmarksHash);
				}
			}
			else
			{
				messageBox(q_("Error"), q_("File does not contain observing lists or legacy bookmarks"));
				return;
			}

			// At this point we have added our lists to the observingLists map. Now update the jsonMap and store to file.
			QFile jsonFile(observingListJsonPath);
			if (!jsonFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
				qWarning() << "[ObservingList] bookmarks list can not be saved. A file can not be open for reading and writing:"
					   << QDir::toNativeSeparators(observingListJsonPath);
				messageBox(q_("Error"), q_("Cannot open observingLists.json to write"));
				return;
			}
			// Update the jsonMap and store
			jsonMap.insert(KEY_OBSERVING_LISTS, observingLists);
			jsonFile.resize(0);
			StelJsonParser::write(jsonMap, &jsonFile);
			jsonFile.flush();
			jsonFile.close();
			tainted=false;

			// Now we have stored to file, but the program is not aware of the new lists!
			loadListNames(); // also populate Combobox and make sure at least some defaultOlud exists.
			loadSelectedList();
		} catch (std::runtime_error &e) {
			qWarning() << "[ObservingList Creation/Edition] File format is wrong! Error: " << e.what();
			messageBox(q_("Error"), q_("File format is wrong!"));
			return;
		}
	}
}

/*
 * Delete the currently selected list. There must be at least one list.
*/
void ObsListDialog::deleteListButtonPressed()
{
	if (observingLists.count()>1 && (selectedOlud!=defaultOlud) && askConfirmation())
	{
		QFile jsonFile(observingListJsonPath);
		if (!jsonFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
			qWarning() << "[ObservingList] bookmarks list can not be saved. A file can not be open for reading and writing:"
				   << QDir::toNativeSeparators(observingListJsonPath);
			messageBox(q_("Error"), q_("Cannot open JSON output file. Will not delete."));
			return;
		}

		observingLists.remove(selectedOlud);
		currentItemCollection.clear();

		selectedOlud=defaultOlud;
		// Update the jsonMap and store
		jsonMap.insert(KEY_OBSERVING_LISTS, observingLists);
		jsonFile.resize(0);
		StelJsonParser::write(jsonMap, &jsonFile);
		jsonFile.flush();
		jsonFile.close();
		tainted=false;

		// Clean up UI
		clearHighlights();
		loadListNames();
		loadSelectedList();
	}
	else
	{
		qDebug() << "deleteButtonPressed: You cannot delete the default or the last list.";
		messageBox(q_("Information"), q_("You cannot delete the default or the last list."));
	}
}

/*
 * Slot for addObjectButton.
 * Save selected object into the list of observed objects.
 */
void ObsListDialog::addObjectButtonPressed()
{
	const double JD = core->getJD();
	const double fov = (ui->fovCheckBox->isChecked() ? GETSTELMODULE(StelMovementMgr)->getCurrentFov() : -1.0);
	const QString Location = core->getCurrentLocation().serializeToLine(); // store completely
	const QString landscapeID=GETSTELMODULE(LandscapeMgr)->getCurrentLandscapeID();

	const QList<StelObjectP> &selectedObject = objectMgr->getSelectedObject();

	if (!selectedObject.isEmpty())
	{
// TBD: this test should prevent adding duplicate entries, but fails. Maybe for V23.4!
//		// No duplicate item in the same list
//		bool is_already_in_list = false;
//		QHash<QString, observingListItem>::iterator i;
//		for (i = observingListItemCollection.begin(); i != observingListItemCollection.end(); i++)
//		{
//			if ((i.value().name.compare(selectedObject[0]->getEnglishName()) == 0) &&
//				(ui->obsListJDCheckBox->isChecked()        && i.value().jd == JD) &&
//				(ui->obsListFovCheckBox->isChecked()       && i.value().fov == fov) &&
//				(ui->obsListLandscapeCheckBox->isChecked() && i.value().landscapeID == landscapeID) &&
//				(ui->obsListLocationCheckBox->isChecked()  && i.value().location == Location))
//			{
//				is_already_in_list = true;
//				break;
//			}
//		}
//
//		if (!is_already_in_list)
//		{
			observingListItem item;

			const QString objectUUID = QUuid::createUuid().toString();

			// Object name (designation) and object name I18n
			if (selectedObject[0]->getType() == "Nebula")
				item.designation = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignationWIC(); // Store most common catalog ID as of our catalog sequence, even if catalog is not active
			else
				item.designation = selectedObject[0]->getEnglishName();
			item.name = selectedObject[0]->getEnglishName();
			item.nameI18n = selectedObject[0]->getNameI18n();
			if(item.nameI18n.isEmpty())
				item.nameI18n = DASH;
			// Check if the object name is empty.
			if (item.designation.isEmpty())
			{
				item.designation = "Unnamed object";
				if (item.nameI18n.isEmpty()) {
					item.nameI18n = q_("Unnamed object");
				}
			}
			// Type, Object Type
			item.objClass = selectedObject[0]->getType();
			item.objTypeI18n = selectedObject[0]->getObjectTypeI18n();

			// Ra & Dec
			if (ui->coordinatesCheckBox->isChecked() || (item.objClass == "Planet" && getFlagUseJD()) || item.objClass == CUSTOM_OBJECT || item.designation.isEmpty()) {
				double ra, dec;
				StelUtils::rectToSphe(&ra, &dec, selectedObject[0]->getJ2000EquatorialPos(core));
				item.ra  = StelUtils::radToHmsStr(ra,  false).trimmed();
				item.dec = StelUtils::radToDmsStr(dec, false).trimmed();
			}
			item.isVisibleMarker=!item.designation.contains("marker", Qt::CaseInsensitive);
			item.magnitude = getMagnitude(selectedObject, core);

			// Constellation
			const Vec3d posNow = selectedObject[0]->getEquinoxEquatorialPos(core);
			item.constellation = core->getIAUConstellation(posNow);

			// Optional: JD, Location, landscape, fov
			if (getFlagUseJD())
				item.jd = JD;
			if (getFlagUseLocation())
				item.location = Location;
			if (getFlagUseLandscape())
				item.landscapeID = landscapeID;
			if (getFlagUseFov() && (fov > 1.e-6))
				item.fov = fov;

			currentItemCollection.insert(objectUUID, item);
			tainted=true;

			// Add object in row model
			StelLocation loc=StelLocation::createFromLine(Location);
			addModelRow(objectUUID,
				    item.designation,
				    item.nameI18n,
				    item.objTypeI18n,
				    item.ra,
				    item.dec,
				    item.magnitude,
				    item.constellation,
				    ui->jdCheckBox->isChecked() ? StelUtils::julianDayToISO8601String(JD + core->getUTCOffset(JD) / 24.).replace("T", " ") : "",
				    ui->locationCheckBox->isChecked() ? loc.name : "",
				    item.landscapeID);
//		}
	}
	else
		qWarning() << "Selected object is empty!";
}

/*
 * Slot for button obsListRemoveObjectButton
 */
void ObsListDialog::removeObjectButtonPressed()
{
	Q_ASSERT(isEditMode);

	int number = ui->treeView->currentIndex().row();
	QString uuid = itemModel->index(number, ColumnUUID).data().toString();
	itemModel->removeRow(number);
	currentItemCollection.remove(uuid);
	tainted=true;
}

/*
 * Slot for saveButton
 */
void ObsListDialog::saveButtonPressed()
{
	Q_ASSERT(isEditMode);
	if (!isEditMode)
	{
		qCritical() << "CALLING ERROR: saveButtonPressed() while not in edit mode.";
		return;
	}

	// we have a valid selectedListOlud. In addition, the list must have a human-readable name.
	QString listName = ui->listNameLineEdit->text().trimmed();
	if (listName.length()==0)
	{
		messageBox(q_("Error"), q_("Empty name"));
		return;
	}
	if (listNames.contains(listName) && isCreationMode)
	{
		messageBox(q_("Error"), q_("List name already exists"));
		return;
	}

	// Last chance to detect any change: Just list name changed?
	if (currentListName!=listName)
		tainted=true;

	if (!tainted)
		return;

	//OK, we save this and keep it as current list.
	currentListName=listName;

	QFile jsonFile(observingListJsonPath);
	if (!jsonFile.open(QIODevice::ReadWrite | QIODevice::Text))
	{
		qWarning() << "[ObservingList Save] Error saving observing list. "
			   << "File cannot be opened for reading and writing:"
			   << QDir::toNativeSeparators(observingListJsonPath);
		return;
	}

	QVariantMap currentList=prepareCurrentList(currentItemCollection);
	observingLists.insert(selectedOlud, currentList);
	jsonMap.insert(KEY_OBSERVING_LISTS, observingLists);

	jsonFile.resize(0);
	StelJsonParser::write(jsonMap, &jsonFile);
	jsonFile.flush();
	jsonFile.close();

	tainted=false;
	switchEditMode(false, false); // Set GUI to normal mode
	loadListNames(); // reload Combobox
	loadSelectedList();
}

/*
 * Slot for cancelButton
 */
void ObsListDialog::cancelButtonPressed()
{
	Q_ASSERT(isEditMode);
	if (!isEditMode)
	{
		qCritical() << "CALLING ERROR: cancelButtonPressed() while not in edit mode.";
		return;
	}
	// Depending on creation or regular edit mode, delete current list and load default, or reload current list,
	if (isCreationMode)
		loadDefaultList();
	else
		loadSelectedList();

	// then close editing mode and set GUI to normal mode
	switchEditMode(false, false);
}

void ObsListDialog::switchEditMode(bool enableEditMode, bool newList)
{
		isEditMode=enableEditMode;
		isCreationMode=newList;
		// The Layout classes have no setVisible(bool), we must configure individual buttons! :-(

		ui->stelWindowTitle->setText(q_("Observing lists"));
		//ui->horizontalLayoutCombo->setEnabled(!isEditMode);     // disable list selection
		ui->obsListComboLabel->setVisible(!isEditMode);
		ui->obsListComboBox->setVisible(!isEditMode);

		// horizontalLayoutLineEdit_1: labelListName, nameOfListLIneEdit
		//ui->horizontalLayout_Name->setEnabled(isEditMode);  // enable list name editing
		ui->listNameLabel->setVisible(isEditMode);
		//ui->horizontalSpacer_listName->sizePolicy().setHeightForWidth(isEditMode);// ->setVisible(isEditMode);
		//qDebug() << "Spacer geometry, policy:" << ui->horizontalSpacer_listName->geometry() << ui->horizontalSpacer_listName->sizePolicy();
		ui->listNameLineEdit->setVisible(isEditMode);
		ui->listNameLineEdit->setText(currentListName);

		ui->descriptionLineEdit->setEnabled(isEditMode);    // (activate description line)

		ui->creationDateLabel->setVisible(!isEditMode);         // Creation date:
		ui->creationDateLineEdit->setVisible(!isEditMode);      //
		ui->lastEditLabel->setVisible(!isEditMode);             // Last edit date:
		ui->lastEditLineEdit->setVisible(!isEditMode);          //

		// line with optional store items
		ui->alsoStoreLabel->setVisible(isEditMode);            // Also store
		ui->coordinatesCheckBox->setVisible(isEditMode);// hide "Coordinates"
		ui->alsoLoadLabel->setVisible(!isEditMode);  // Also load

		//ui->horizontalLayoutButtons->setEnabled(!isEditMode);   // Highlight/Clear/NewList/EditList/DeleteList/ExportList/ImportList
		ui->highlightAllButton->setVisible(!isEditMode);
		ui->clearHighlightButton->setVisible(!isEditMode);
		ui->newListButton->setVisible(!isEditMode);
		ui->editListButton->setVisible(!isEditMode);
		ui->deleteListButton->setVisible(!isEditMode);
		ui->exportListButton->setVisible(!isEditMode);
		ui->importListButton->setVisible(!isEditMode);

		//ui->horizontalLayoutButtons_1->setEnabled(isEditMode); // Add/Remove/Export/Import
		ui->addObjectButton->setVisible(isEditMode);
		ui->removeObjectButton->setVisible(isEditMode);
		ui->saveButton->setVisible(isEditMode);
		ui->cancelButton->setVisible(isEditMode);
}

/*
 * Sort the treeView by the column name given in parameter
*/
void ObsListDialog::sortObsListTreeViewByColumnName(const QString &columnName)
{
	static const QMap<QString,int>map={
		{SORTING_BY_NAME,          ColumnDesignation},
		{SORTING_BY_NAMEI18N,      ColumnNameI18n},
		{SORTING_BY_TYPE,          ColumnType},
		{SORTING_BY_RA,            ColumnRa},
		{SORTING_BY_DEC,           ColumnDec},
		{SORTING_BY_MAGNITUDE,     ColumnMagnitude},
		{SORTING_BY_CONSTELLATION, ColumnConstellation},
		{SORTING_BY_DATE,          ColumnDate},
		{SORTING_BY_LOCATION,      ColumnLocation},
		{SORTING_BY_LANDSCAPE_ID,  ColumnLandscapeID}
	};
	itemModel->sort(map.value(columnName), Qt::AscendingOrder);
}

void ObsListDialog::setFlagUseJD(bool b)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	flagUseJD=b;
	conf->setValue("bookmarks/useJD", b);
	emit flagUseJDChanged(b);
}
void ObsListDialog::setFlagUseLandscape(bool b)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	flagUseLandscape=b;
	conf->setValue("bookmarks/useLandscape", b);
	emit flagUseLandscapeChanged(b);
}
void ObsListDialog::setFlagUseLocation(bool b)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	flagUseLocation=b;
	conf->setValue("bookmarks/useLocation", b);
	emit flagUseLocationChanged(b);
}
void ObsListDialog::setFlagUseFov(bool b)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	flagUseFov=b;
	conf->setValue("bookmarks/useFOV", b);
	emit flagUseFovChanged(b);
}

/*
 * Prepare the currently displayed/edited list for storage
 * Returns QVariantList with keys={creation date, last edit, description, name, objects, sorting}
 */
QVariantMap ObsListDialog::prepareCurrentList(QHash<QString, observingListItem> &itemHash)
{
	// Edit date
	const double JD = StelUtils::getJDFromSystem();
	const QString lastEditDate = StelUtils::julianDayToISO8601String(JD + core->getUTCOffset(JD) / 24.).replace("T", " ");
	QVariantMap currentList = {
		// Name, description, current date for the list, current sorting
		{KEY_NAME,          currentListName},
		{KEY_DESCRIPTION,   ui->descriptionLineEdit->text()},
		{KEY_SORTING,       sorting},
		{KEY_CREATION_DATE, ui->creationDateLineEdit->text()},
		{KEY_LAST_EDIT,     lastEditDate }
	};

	// List of objects
	QVariantList listOfObjects;
	QHashIterator<QString, observingListItem> i(itemHash);
	while (i.hasNext())
	{
		i.next();
		observingListItem item = i.value();
		listOfObjects.push_back(item.toVariantMap());
	}
	currentList.insert(KEY_OBJECTS, listOfObjects);

	return currentList;
}

/*
 * Slot for obsListCreationEditionTreeView header
 */
void ObsListDialog::headerClicked(int index)
{
	static const QMap<int,QString> map={
		{ColumnDesignation,   SORTING_BY_NAME},
		{ColumnNameI18n,      SORTING_BY_NAMEI18N},
		{ColumnType,          SORTING_BY_TYPE},
		{ColumnRa,            SORTING_BY_RA},
		{ColumnDec,           SORTING_BY_DEC},
		{ColumnMagnitude,     SORTING_BY_MAGNITUDE},
		{ColumnConstellation, SORTING_BY_CONSTELLATION},
		{ColumnDate,          SORTING_BY_DATE},
		{ColumnLocation,      SORTING_BY_LOCATION},
		{ColumnLandscapeID,   SORTING_BY_LANDSCAPE_ID}};
	sorting=map.value(index, "");
	//qDebug() << "Sorting = " << sorting;
}

// Get the magnitude from selected object (or a dash if unavailable)
QString ObsListDialog::getMagnitude(const QList<StelObjectP> &selectedObject, StelCore *core)
{
	if (!core)
		return DASH;

	QString objectMagnitudeStr(DASH);
	const float objectMagnitude = selectedObject[0]->getVMagnitude(core);
	if (objectMagnitude > 98.f)
	{
		if (QString::compare(selectedObject[0]->getType(), "Nebula", Qt::CaseSensitive) == 0)
		{
			auto &r_nebula = dynamic_cast<Nebula &>(*selectedObject[0]);
			const float mB = r_nebula.getBMagnitude(core);
			if (mB < 98.f)
				objectMagnitudeStr = QString::number(mB);
		}
	}
	else
		objectMagnitudeStr = QString::number(objectMagnitude, 'f', 2);

	return objectMagnitudeStr;
}

void ObsListDialog::defaultClicked(bool b)
{
	defaultOlud = (b ? selectedOlud : "");
	jsonMap.insert(KEY_DEFAULT_LIST_OLUD, defaultOlud);
	tainted=true;
}

const QString ObsListDialog::JSON_FILE_NAME     = QStringLiteral("observingList.json");
const QString ObsListDialog::JSON_FILE_BASENAME = QStringLiteral("observingList");
const QString ObsListDialog::FILE_VERSION       = QStringLiteral("2.1");

const QString ObsListDialog::JSON_BOOKMARKS_FILE_NAME   = QStringLiteral("bookmarks.json");
const QString ObsListDialog::BOOKMARKS_LIST_NAME        = QStringLiteral("bookmarks list");
const QString ObsListDialog::BOOKMARKS_LIST_DESCRIPTION = QStringLiteral("Bookmarks of previous Stellarium version.");
const QString ObsListDialog::SHORT_NAME_VALUE           = QStringLiteral("Observing list for Stellarium");

const QString ObsListDialog::KEY_DEFAULT_LIST_OLUD = QStringLiteral("defaultListOlud");
const QString ObsListDialog::KEY_OBSERVING_LISTS   = QStringLiteral("observingLists");
const QString ObsListDialog::KEY_CREATION_DATE     = QStringLiteral("creation date");
const QString ObsListDialog::KEY_LAST_EDIT         = QStringLiteral("last edit");
const QString ObsListDialog::KEY_BOOKMARKS         = QStringLiteral("bookmarks");
const QString ObsListDialog::KEY_NAME              = QStringLiteral("name");
const QString ObsListDialog::KEY_NAME_I18N         = QStringLiteral("nameI18n");
const QString ObsListDialog::KEY_JD                = QStringLiteral("jd");
const QString ObsListDialog::KEY_RA                = QStringLiteral("ra");
const QString ObsListDialog::KEY_DEC               = QStringLiteral("dec");
const QString ObsListDialog::KEY_FOV               = QStringLiteral("fov");
const QString ObsListDialog::KEY_DESCRIPTION       = QStringLiteral("description");
const QString ObsListDialog::KEY_LANDSCAPE_ID      = QStringLiteral("landscapeID");
const QString ObsListDialog::KEY_OBJECTS           = QStringLiteral("objects");
const QString ObsListDialog::KEY_OBJECTS_TYPE      = QStringLiteral("objtype");
const QString ObsListDialog::KEY_TYPE              = QStringLiteral("type");
const QString ObsListDialog::KEY_DESIGNATION       = QStringLiteral("designation");
const QString ObsListDialog::KEY_SORTING           = QStringLiteral("sorting");
const QString ObsListDialog::KEY_LOCATION          = QStringLiteral("location");
const QString ObsListDialog::KEY_MAGNITUDE         = QStringLiteral("magnitude");
const QString ObsListDialog::KEY_CONSTELLATION     = QStringLiteral("constellation");
const QString ObsListDialog::KEY_VERSION           = QStringLiteral("version");
const QString ObsListDialog::KEY_SHORT_NAME        = QStringLiteral("shortName");
const QString ObsListDialog::KEY_IS_VISIBLE_MARKER = QStringLiteral("isVisibleMarker");

const QString ObsListDialog::SORTING_BY_NAME          = QStringLiteral("name");
const QString ObsListDialog::SORTING_BY_NAMEI18N      = QStringLiteral("nameI18n");
const QString ObsListDialog::SORTING_BY_TYPE          = QStringLiteral("type");
const QString ObsListDialog::SORTING_BY_RA            = QStringLiteral("right ascension");
const QString ObsListDialog::SORTING_BY_DEC           = QStringLiteral("declination");
const QString ObsListDialog::SORTING_BY_MAGNITUDE     = QStringLiteral("magnitude");
const QString ObsListDialog::SORTING_BY_CONSTELLATION = QStringLiteral("constellation");
const QString ObsListDialog::SORTING_BY_DATE          = QStringLiteral("date");
const QString ObsListDialog::SORTING_BY_LOCATION      = QStringLiteral("location");
const QString ObsListDialog::SORTING_BY_LANDSCAPE_ID  = QStringLiteral("landscapeID");

const QString ObsListDialog::CUSTOM_OBJECT = QStringLiteral("CustomObject");

const QString ObsListDialog::DASH = QString(QChar(0x2014));
