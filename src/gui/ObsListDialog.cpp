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

#include "ui_obsListUnifiedDialog.h"

ObsListDialog::ObsListDialog(QObject *parent) :
	StelDialog("ObservingList", parent),
	ui(new Ui_obsListUnifiedDialogForm()),
	obsListListModel(new QStandardItemModel(0, ColumnCount)),
	core(StelApp::getInstance().getCore()) //,
//	createEditDialog_instance(Q_NULLPTR)
{
	setObjectName("ObsListDialog");
	objectMgr    = GETSTELMODULE(StelObjectMgr);
	landscapeMgr = GETSTELMODULE(LandscapeMgr);
	labelMgr     = GETSTELMODULE(LabelMgr);
	StelApp::getInstance().getStelPropertyManager()->registerObject(this);

	observingListJsonPath =
			StelFileMgr::findFile("data",
					      static_cast<StelFileMgr::Flags>(StelFileMgr::Directory | StelFileMgr::Writable)) +
			"/" +
			QString(JSON_FILE_NAME);
	bookmarksJsonPath =
			StelFileMgr::findFile("data",
					      static_cast<StelFileMgr::Flags>(StelFileMgr::Directory | StelFileMgr::Writable)) +
			"/" +
			QString(JSON_BOOKMARKS_FILE_NAME);
	QSettings* conf  = StelApp::getInstance().getSettings();
	flagUseJD        = conf->value("bookmarks/useJD", false).toBool();
	flagUseLandscape = conf->value("bookmarks/useLandscape", false).toBool();
	flagUseLocation  = conf->value("bookmarks/useLocation", false).toBool();
	flagUseFov       = conf->value("bookmarks/useFOV", false).toBool();
}

ObsListDialog::~ObsListDialog() {
	delete ui;
	delete obsListListModel;
	ui = Q_NULLPTR;
	obsListListModel = Q_NULLPTR;
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

	connect(ui->obsListNewListButton,        SIGNAL(clicked()), this, SLOT(obsListNewListButtonPressed()));
	connect(ui->obsListEditListButton,       SIGNAL(clicked()), this, SLOT(obsListEditButtonPressed()));
	connect(ui->obsListHighlightAllButton,   SIGNAL(clicked()), this, SLOT(highlightAll()));
	connect(ui->obsListClearHighlightButton, SIGNAL(clicked()), this, SLOT(clearHighlights()));
	connect(ui->obsListDeleteButton,         SIGNAL(clicked()), this, SLOT(obsListDeleteButtonPressed()));
	connect(ui->obsListTreeView,             SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectAndGoToObject(QModelIndex)));


	// NEW: EDITS
	connect(ui->obsListAddObjectButton,    SIGNAL(clicked()),     this, SLOT(obsListAddObjectButtonPressed()));
	connect(ui->obsListExitButton,         SIGNAL(clicked()),     this, SLOT(obsListCancelButtonPressed()));
	connect(ui->obsListSaveButton,         SIGNAL(clicked()),     this, SLOT(obsListSaveButtonPressed()));
	connect(ui->obsListRemoveObjectButton, SIGNAL(clicked()),     this, SLOT(obsListRemoveObjectButtonPressed()));
	connect(ui->obsListImportListButton,   SIGNAL(clicked()),     this, SLOT(obsListImportListButtonPresssed()));
	connect(ui->obsListExportListButton,   SIGNAL(clicked()),     this, SLOT(obsListExportListButtonPressed()));
	connect(ui->nameOfListLineEdit,        SIGNAL(textChanged(const QString&)), this, SLOT(nameOfListTextChange(const QString&)));
	switchEditMode(false); // NEW  start with view mode

	connectBoolProperty(ui->obsListJDCheckBox,        "ObsListDialog.flagUseJD");
	connectBoolProperty(ui->obsListLocationCheckBox,  "ObsListDialog.flagUseLocation");
	connectBoolProperty(ui->obsListLandscapeCheckBox, "ObsListDialog.flagUseLandscape");
	connectBoolProperty(ui->obsListFovCheckBox,       "ObsListDialog.flagUseFov");

	//obsListCombo settings
	connect(ui->obsListComboBox, SIGNAL(activated(int)), this, SLOT(loadSelectedObservingList(int)));

	//Initialize the list of observing lists
	//obsListListModel->setColumnCount(ColumnCount); //Has been done in constructor.
	setObservingListHeaderNames();

	ui->obsListTreeView->setModel(obsListListModel);
	ui->obsListTreeView->header()->setSectionsMovable(false);
	ui->obsListTreeView->hideColumn(ColumnUUID);
	ui->obsListTreeView->header()->setSectionResizeMode(ColumnName,          QHeaderView::ResizeToContents);
	ui->obsListTreeView->header()->setSectionResizeMode(ColumnNameI18n,      QHeaderView::ResizeToContents);
	ui->obsListTreeView->header()->setSectionResizeMode(ColumnType,          QHeaderView::ResizeToContents);
	ui->obsListTreeView->header()->setSectionResizeMode(ColumnRa,            QHeaderView::ResizeToContents);
	ui->obsListTreeView->header()->setSectionResizeMode(ColumnDec,           QHeaderView::ResizeToContents);
	ui->obsListTreeView->header()->setSectionResizeMode(ColumnMagnitude,     QHeaderView::ResizeToContents);
	ui->obsListTreeView->header()->setSectionResizeMode(ColumnConstellation, QHeaderView::ResizeToContents);
	ui->obsListTreeView->header()->setSectionResizeMode(ColumnDate,          QHeaderView::ResizeToContents);
	ui->obsListTreeView->header()->setSectionResizeMode(ColumnLocation,      QHeaderView::ResizeToContents);
	ui->obsListTreeView->header()->setSectionResizeMode(ColumnLandscapeID,   QHeaderView::ResizeToContents);
	ui->obsListTreeView->header()->setStretchLastSection(true);
	//Enable the sort for columns
	ui->obsListTreeView->setSortingEnabled(true);

	//By default buttons are disabled
	ui->obsListEditListButton->setEnabled(false);
	ui->obsListHighlightAllButton->setEnabled(false);
	ui->obsListClearHighlightButton->setEnabled(false);
	ui->obsListDeleteButton->setEnabled(false);

	// For no regression we must take into account the legacy bookmarks file
	// TBD: We should load the global list only once!
	QFile jsonBookmarksFile(bookmarksJsonPath);
	if (jsonBookmarksFile.exists())
	{
		//qDebug() << "OLD BOOKMARKS FOUND: TRY IF WE NEED TO PROCESS/IMPORT THEM";

		// check if already loaded.
		QFile jsonFile(observingListJsonPath);
		if (!jsonFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
			qWarning() << "[ObservingList] bookmarks list can not be saved. A file can not be open for reading and writing:"
				   << QDir::toNativeSeparators(observingListJsonPath);
			return;
		}
		QVariantMap mapFromJsonFile;
		QVariantMap allListsMap;
		if (jsonFile.size() > 0) {
			mapFromJsonFile = StelJsonParser::parse(jsonFile.readAll()).toMap();
			allListsMap = mapFromJsonFile.value(QString(KEY_OBSERVING_LISTS)).toMap();
		}

		// QString defaultListValue = mapFromJsonFile.value(QString(KEY_DEFAULT_LIST_OLUD)).toString();
		// if (defaultListValue.isEmpty()) {
		// 	// If empty, set to empty? Is that useful?
		// 	mapFromJsonFile.insert(KEY_DEFAULT_LIST_OLUD, "");
		// }

		if (!checkIfBookmarksListExists(allListsMap))
		{
			//qDebug() << "NO BOOKMARK LIST SO FAR. IMPORTING...";
			QHash<QString, observingListItem> bookmarksForImport=loadBookmarksInObservingList();
			saveBookmarksInObsListJsonFile(allListsMap, bookmarksForImport);
			//qDebug() << "read into allListMap of size" << allListsMap.size();
			mapFromJsonFile.insert(QString(KEY_OBSERVING_LISTS), allListsMap);
		}
		//else
		//	qDebug() << "BOOKMARK LIST EXISTS. WE CAN SKIP THE IMPORT";

		mapFromJsonFile.insert(QString(KEY_VERSION), QString(FILE_VERSION));
		mapFromJsonFile.insert(QString(KEY_SHORT_NAME), QString(SHORT_NAME_VALUE));

		jsonFile.resize(0);
		StelJsonParser::write(mapFromJsonFile, &jsonFile);
		jsonFile.flush();
		jsonFile.close();
	}
	// Now we certainly have a json file.
	loadListsNameFromJsonFile();
	defaultListOlud_ = extractDefaultListOludFromJsonFile();
	loadDefaultList();
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
		"UUID", // Hidden column
		q_("Object designation"),
		q_("Object name"),
		q_("Type"),
		q_("Right ascension"),
		q_("Declination"),
		q_("Magnitude"),
		q_("Constellation"),
		q_("Date"),
		q_("Location"),
		q_("Landscape")
	};
	Q_ASSERT(headerStrings.length()==ColumnCount);
	obsListListModel->setHorizontalHeaderLabels(headerStrings);
}

/*
 * Add row in the obsListListModel
*/
void ObsListDialog::addModelRow(const QString &olud, const QString &name, const QString &nameI18n,
                                const QString &type, const QString &ra,
                                const QString &dec, const QString &magnitude, const QString &constellation,
				const QString &date, const QString &location, const QString &landscapeID)
{
	const int number=obsListListModel->rowCount();
	QStandardItem *item = Q_NULLPTR;

	item = new QStandardItem(olud);
	item->setEditable(false);
	obsListListModel->setItem(number, ColumnUUID, item);

	item = new QStandardItem(name);
	item->setEditable(false);
	obsListListModel->setItem(number, ColumnName, item);

	item = new QStandardItem(nameI18n);
	item->setEditable(false);
	obsListListModel->setItem(number, ColumnNameI18n, item);

	item = new QStandardItem(type);
	item->setEditable(false);
	obsListListModel->setItem(number, ColumnType, item);

	item = new QStandardItem(ra);
	item->setEditable(false);
	obsListListModel->setItem(number, ColumnRa, item);

	item = new QStandardItem(dec);
	item->setEditable(false);
	obsListListModel->setItem(number, ColumnDec, item);

	item = new QStandardItem(magnitude);
	item->setEditable(false);
	obsListListModel->setItem(number, ColumnMagnitude, item);

	item = new QStandardItem(constellation);
	item->setEditable(false);
	obsListListModel->setItem(number, ColumnConstellation, item);

	item = new QStandardItem(date);
	item->setEditable(false);
	obsListListModel->setItem(number, ColumnDate, item);

	item = new QStandardItem(location);
	item->setEditable(false);
	obsListListModel->setItem(number, ColumnLocation, item);

	item = new QStandardItem(landscapeID);
	item->setEditable(false);
	obsListListModel->setItem(number, ColumnLandscapeID, item);

	for (int i = 0; i < ColumnCount; ++i)
		ui->obsListTreeView->resizeColumnToContents(i);
}

/*
 * Slot for button obsListHighLightAllButton
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

	for (const auto &item: qAsConst(observingListItemCollection)) {
		const QString name = item.name;
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
 * Clear highlight
*/
void ObsListDialog::clearHighlights()
{
	//objectMgr->unSelect();
	GETSTELMODULE (HighlightMgr)->cleanHighlightList();
	// Clear labels
	for (auto l: highlightLabelIDs) {
		labelMgr->deleteLabel(l);
	}
	highlightLabelIDs.clear();
}

/*
 * Slot for button obsListNewListButton
*/
void ObsListDialog::obsListNewListButtonPressed()
{
	//invokeObsListCreateEditDialog("");

	listOlud_="";
	connect(this, SIGNAL (exitButtonClicked()), this, SLOT (finishEditMode()));
	switchEditMode(true);

	ui->nameOfListLineEdit->setText(q_("New Observation List"));
	isCreationMode = true;
	ui->stelWindowTitle->setText(q_("Observing list creation mode"));
	ui->obsListImportListButton->setHidden(false);

}
/*
 * Slot for button obsListEditButton
*/
void ObsListDialog::obsListEditButtonPressed()
{
	if (!selectedObservingListUuid.isEmpty())
	{
		//invokeObsListCreateEditDialog(selectedObservingListUuid);

		listOlud_=selectedObservingListUuid;
		connect(this, SIGNAL (exitButtonClicked()), this, SLOT (finishEditMode()));
		switchEditMode(true);

		//ui->nameOfListLineEdit->setText(selectedObservingListUuid);
		ui->nameOfListLineEdit->setText(currentListName);
		isCreationMode = false;
		ui->stelWindowTitle->setText(q_("Observing list editor mode"));
		ui->obsListImportListButton->setHidden(true);
		//loadObservingList();
	}
	else
		qWarning() << "The selected observing list olud is empty";
}

/*
// * Open the observing list create/edit dialog
//
//void ObsListDialog::invokeObsListCreateEditDialog(QString listOlud) {
//	//createEditDialog_instance = ObsListCreateEditDialog::Instance(listOlud);
//	listOlud_=listOlud;
//	connect(this, SIGNAL (exitButtonClicked()), this, SLOT (finishEditMode()));
//	switchEditMode(true);
//	//setListName(listNames_);
//	//createEditDialog_instance->setVisible(true);
//}*/

/*
 * Load the lists names from Json file
 * to populate the combo box and get the default list uuid
*/
void ObsListDialog::loadListsNameFromJsonFile()
{
	QVariantMap map; // The full map.
	QFile jsonFile(observingListJsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly))
		qWarning() << "[ObservingList] cannot open" << QDir::toNativeSeparators(JSON_FILE_NAME);
	else
	{
		try {
			map = StelJsonParser::parse(jsonFile.readAll()).toMap();
			jsonFile.close();

			// init combo box
			//ui->obsListComboBox->clear();

			QVariantMap observingListsMap = map.value(QString(KEY_OBSERVING_LISTS)).toMap();
			populateListNameInComboBox(observingListsMap, defaultListOlud_);
			//populateDataInComboBox(observingListsMap, defaultListOlud_);

		} catch (std::runtime_error &e) {
			qWarning() << "[ObservingList] File format is wrong! Error: " << e.what();
			return;
		}
	}
}

/*
 * Populate the list names into combo box
*/
void ObsListDialog::populateListNameInComboBox(QVariantMap map, const QString &defaultListOlud)
{
	//listNamesModel.clear();
	QStringList listNamesModel;
	listNames_.clear();
	QMap<QString, QVariant>::iterator i;
	for (i = map.begin(); i != map.end(); ++i) {
		if (i.value().canConvert<QVariantMap>())
		{
			QVariant var = i.value();
			auto data = var.value<QVariantMap>();
			QString listName = data.value(KEY_NAME).toString();
			listNamesModel.append(listName);
			listNames_.append(listName);
		}
	}
	listNamesModel.sort(Qt::CaseInsensitive);
	ui->obsListComboBox->clear();
	ui->obsListComboBox->addItems(listNamesModel);

	// Now add the item data into the ComboBox
	//const int comboCount=ui->obsListComboBox->count();
	for (i = map.begin(); i != map.end(); ++i) {
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
		// Make sure defaultListOlud only gets changed if it occurs in the map
		if (defaultListOlud == listUuid)
			defaultListOlud_ = defaultListOlud;
	}
	// For now, add a debug output. Should there be a fallback?
	if (defaultListOlud_ != defaultListOlud)
		qDebug() << "populateDataInComboBox: Cannot find defaultListOlud" << defaultListOlud << "not changing.";
}

/*
 * Populate data into combo box
*
void ObsListDialog::populateDataInComboBox(QVariantMap map, const QString &defaultListOlud)
{
//	QStringList listNamesModel=ui->obsListComboBox->currentText();

	QMap<QString, QVariant>::iterator i;
	for (i = map.begin(); i != map.end(); ++i) {
		const QString &listUuid = i.key();
		if (i.value().canConvert<QVariantMap>()) {
			QVariant var = i.value();
			auto data = var.value<QVariantMap>();
			QString listName = data.value(KEY_NAME).toString();

			foreach (QString str, listNamesModel)
			{
				if (QString::compare(str, listName) == 0)
				{
					int index = listNamesModel.indexOf(listName);
					Q_ASSERT(index>=0);
					ui->obsListComboBox->setItemData(index, listUuid);
					break;
				}
			}
		}
		// Make sure defaultListOlud only gets changed if it occurs in the map
		if (defaultListOlud == listUuid)
			defaultListOlud_ = defaultListOlud;
	}
	// For now, add a debug output. Should there be a fallback?
	if (defaultListOlud_ != defaultListOlud)
		qDebug() << "populateDataInComboBox: Cannot find defaultListOlud" << defaultListOlud << "not changing.";
}
*/

/*
 * Load the default list
*/
void ObsListDialog::loadDefaultList()
{
	if (defaultListOlud_ != "")
	{
		int index = ui->obsListComboBox->findData(defaultListOlud_);
		if (index != -1)
		{
			ui->obsListComboBox->setCurrentIndex(index);
			//ui->obsListEditListButton->setEnabled(true);
			selectedObservingListUuid = defaultListOlud_;
			loadSelectedObservingListFromJsonFile(defaultListOlud_);
		}
	}
	else
	{
		// If there is no default list we load the current, or the first list in the combo box
		const int currentIndex = ui->obsListComboBox->currentIndex();
		if (currentIndex != -1)
		{
			ui->obsListComboBox->setCurrentIndex(0);
			loadSelectedObservingList(0);
		}
	}
}

/*
 * Load the selected observing list in the combo box, from Json file into dialog.
*/
void ObsListDialog::loadSelectedObservingListFromJsonFile(const QString &listOlud)
{
	QVariantMap map;
	QVariantList listOfObjects;
	QFile jsonFile(observingListJsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly))
		qWarning() << "[ObservingList] cannot open" << QDir::toNativeSeparators(JSON_FILE_NAME);
	else
	{
		// We must keep selection for the user. It is not enough to store/restore the existingSelection.
		// The QList<StelObjectP> objects are apparently volatile. We must retrieve the actual object.
		const QList<StelObjectP>&existingSelection = objectMgr->getSelectedObject();
		QList<StelObjectP> existingSelectionToRestore;
		if (existingSelection.length()>0)
		{
			existingSelectionToRestore.append(existingSelection.at(0));
		}

		try {
			map = StelJsonParser::parse(jsonFile.readAll()).toMap();
			QVariantMap observingListMap = map.value(QString(KEY_OBSERVING_LISTS)).toMap().value(listOlud).toMap();

			// Check or not default list checkbox information
			ui->obsListDefaultListCheckBox->setChecked(listOlud.compare(defaultListOlud_, Qt::CaseSensitive) == 0);

			listOfObjects = loadListFromJson(map, listOlud);

			ui->obsListDeleteButton->setEnabled(true);

			if (!listOfObjects.isEmpty())
			{
				ui->obsListHighlightAllButton->setEnabled(true);
				ui->obsListClearHighlightButton->setEnabled(true);

				for (const QVariant &object: listOfObjects)
				{
					if (object.canConvert<QVariantMap>())
					{
						QVariantMap objectMap = object.value<QVariantMap>();

						observingListItem item;
						const QString objectOlud = QUuid::createUuid().toString();
						item.name = objectMap.value(QString(KEY_DESIGNATION)).toString();
						item.nameI18n = objectMap.value(QString(KEY_NAME_I18N)).toString();

						// Caveat - Please make the code more readable!
						// We assign KEY_TYPE to item.objtype and KEY_OBJECTS_TYPE to item.type.
						item.objtype    = objectMap.value(QString(KEY_TYPE)).toString();
						//item.type = objectMap.value(QString(KEY_OBJECTS_TYPE)).toString();

						item.ra  = objectMap.value(QString(KEY_RA)).toString();
						item.dec = objectMap.value(QString(KEY_DEC)).toString();

						if (objectMgr->findAndSelect(item.name, item.objtype) && !objectMgr->getSelectedObject().isEmpty())
						{
							const QList<StelObjectP> &selectedObject = objectMgr->getSelectedObject();
							double ra, dec;
							StelUtils::rectToSphe(&ra, &dec, selectedObject[0]->getJ2000EquatorialPos(core));

							if (item.ra.isEmpty())
								item.ra = StelUtils::radToHmsStr(ra, false).trimmed();
							if (item.dec.isEmpty())
								item.dec = StelUtils::radToDmsStr(dec, false).trimmed();
							item.type = selectedObject[0]->getObjectTypeI18n();
						}
						else
							qWarning() << "[ObservingList] object: " << item.name << " not found or empty !";

						item.magnitude = objectMap.value(QString(KEY_MAGNITUDE)).toString();
						item.constellation = objectMap.value(QString(KEY_CONSTELLATION)).toString();

						// Julian Day / Date
						item.jd = objectMap.value(QString(KEY_JD)).toDouble();
						QString dateStr;
						if (item.jd != 0.)
							dateStr = StelUtils::julianDayToISO8601String(item.jd + core->getUTCOffset(item.jd) / 24.).replace("T", " ");

						// Location, landscapeID (may be empty)
						item.location = objectMap.value(QString(KEY_LOCATION)).toString();
						item.landscapeID = objectMap.value(QString(KEY_LANDSCAPE_ID)).toString();

						// Fov (may be empty/0=invalid)
						item.fov = objectMap.value(QString(KEY_FOV)).toDouble();

						// Visible flag
						item.isVisibleMarker = objectMap.value(QString(KEY_IS_VISIBLE_MARKER)).toBool();

						QString LocationStr;
						if (!item.location.isEmpty())
						{
							StelLocation loc=StelApp::getInstance().getLocationMgr().locationForString(item.location);
							LocationStr=loc.name;
						}

						addModelRow(objectOlud,
							    item.name,
							    item.nameI18n,
							    item.type,
							    item.ra,
							    item.dec,
							    item.magnitude,
							    item.constellation,
							    dateStr,
							    LocationStr,
							    item.landscapeID);

						observingListItemCollection.insert(objectOlud, item);
					}
					else
					{
						qCritical() << "[ObservingList] conversion error";
						return;
					}
				}
			}
			else
			{
				ui->obsListHighlightAllButton->setEnabled(false);
				ui->obsListClearHighlightButton->setEnabled(false);
				//ui->obsListDeleteButton->setEnabled(false);
			}

			// Sorting for the objects list.
			QString sortingBy = observingListMap.value(KEY_SORTING).toString();
			if (!sortingBy.isEmpty())
				sortObsListTreeViewByColumnName(sortingBy);
		} catch (std::runtime_error &e) {
			qWarning() << "[ObservingList] Load selected observing list: File format is wrong! Error: " << e.what();
		}

		jsonFile.close();

		// Restore selection that was active before calling this
		if (existingSelectionToRestore.length()>0)
			objectMgr->setSelectedObject(existingSelectionToRestore, StelModule::ReplaceSelection);
		else
			objectMgr->unSelect();
	}
}

/*
 * Load the list from JSON file QVariantMap map
*/
QVariantList ObsListDialog::loadListFromJson(const QVariantMap &map, const QString& listOlud)
{
	observingListItemCollection.clear();
	QVariantMap observingListMap = map.value(QString(KEY_OBSERVING_LISTS)).toMap().value(listOlud).toMap();
	QVariantList listOfObjects;

	// Display description and creation date
	QString listDescription = observingListMap.value(QString(KEY_DESCRIPTION)).toString();
	ui->descriptionLineEdit->setText(listDescription);
	QString listCreationDate = observingListMap.value(QString(KEY_CREATION_DATE)).toString();
	ui->obsListCreationDateLineEdit->setText(listCreationDate);

	if (observingListMap.value(QString(KEY_OBJECTS)).canConvert<QVariantList>())
	{
		QVariant data = observingListMap.value(QString(KEY_OBJECTS));
		listOfObjects = data.value<QVariantList>();
	}
	else
		qCritical() << "[ObservingList] conversion error";

	// Clear model
	obsListListModel->removeRows(0, obsListListModel->rowCount()); // don't use clear() here!
	return listOfObjects;
}


/*
 * Load the bookmarks of bookmarks.json file into observing lists file
 * For no regression with must take into account the legacy bookmarks.json file
*/
QHash<QString, observingListItem> ObsListDialog::loadBookmarksInObservingList()
{
	//qDebug() << "LOADING OLD BOOKMARKS...";

	QHash<QString, observingListItem> bookmarksCollection;
	QVariantMap map;

	QFile jsonFile(bookmarksJsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly)) {
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
			map = StelJsonParser::parse(jsonFile.readAll()).toMap();
			jsonFile.close();
			QVariantMap bookmarksMap = map.value(KEY_BOOKMARKS).toMap();

			QMapIterator<QString,QVariant>it(bookmarksMap);
			while(it.hasNext())
			{
				it.next();

				QVariantMap bookmarkData = it.value().toMap();
				observingListItem item;

				// Name
				item.name = bookmarkData.value(KEY_NAME).toString();
				QString nameI18n = bookmarkData.value(KEY_NAME_I18N).toString();
				item.nameI18n = nameI18n.isEmpty() ? QString(dash) : nameI18n;

				// We need to select the object to add additional information that is not in the Bookmark file
				if (objectMgr->findAndSelect(item.name) && !objectMgr->getSelectedObject().isEmpty()) {
					const QList<StelObjectP> &selectedObject = objectMgr->getSelectedObject();

					item.type = selectedObject[0]->getType();
					item.objtype = selectedObject[0]->getObjectType();
					item.jd = bookmarkData.value(KEY_JD).toDouble();
					if (item.jd!=0.)
					{
						core->setJD(item.jd);
						core->update(0.); // Force position updates
					}

					// Ra & Dec - ra and dec are not empty in case of Custom Object
					item.ra = bookmarkData.value(KEY_RA).toString();
					item.dec = bookmarkData.value(KEY_DEC).toString();
					if (item.ra.isEmpty() || item.dec.isEmpty()) {
						double ra, dec;
						StelUtils::rectToSphe(&ra, &dec, selectedObject[0]->getJ2000EquatorialPos(core));
						item.ra = StelUtils::radToHmsStr(ra, false).trimmed();
						item.dec = StelUtils::radToDmsStr(dec, false).trimmed();
					}
					item.magnitude = ObservingListUtil::getMagnitude(selectedObject, core);
					// Several data items were not part of the original bookmarks, so we have no entry.
					const Vec3d posNow = selectedObject[0]->getEquinoxEquatorialPos(core);
					item.constellation = core->getIAUConstellation(posNow);
					item.location = bookmarkData.value(KEY_LOCATION).toString();
					// No landscape in original bookmarks. Just leave empty.
					//item.landscapeID = bookmarkData.value(KEY_LANDSCAPE_ID).toString();
					double bmFov = bookmarkData.value(KEY_FOV).toDouble();
					if (bmFov>1.e-10) // FoV may look like 3.121251e-310, which is bogus.
						item.fov=bmFov;
					item.isVisibleMarker = bookmarkData.value(KEY_IS_VISIBLE_MARKER, false).toBool();

					bookmarksCollection.insert(it.key(), item);
				}
			}

			//saveBookmarksInObsListJsonFile(bookmarksCollection);
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
	return bookmarksCollection;
}

/*
 * Save the bookmarks into observing list file
*/
void ObsListDialog::saveBookmarksInObsListJsonFile(QVariantMap &allListsMap, const QHash<QString, observingListItem> &bookmarksCollection)
{
		QVariantMap bookmarksObsList;

		// Name, Description
		bookmarksObsList.insert(QString(KEY_NAME), BOOKMARKS_LIST_NAME);
		bookmarksObsList.insert(QString(KEY_DESCRIPTION), QString(BOOKMARKS_LIST_DESCRIPTION));

		// Creation date
		double JD = core->getJD(); // TODO: Replace by current system time
		QString listCreationDate = StelUtils::julianDayToISO8601String(JD + core->getUTCOffset(JD) / 24.).replace("T", " ");
		bookmarksObsList.insert(QString(KEY_CREATION_DATE), listCreationDate);

		// List of objects
		QVariantList listOfObjects;
		QHashIterator<QString, observingListItem> i(bookmarksCollection);
		while (i.hasNext())
		{
			i.next();

			observingListItem item = i.value();
			QVariantMap obl;

			obl.insert(QString(KEY_DESIGNATION), item.name);
			obl.insert(QString(KEY_NAME_I18N), item.nameI18n);
			obl.insert(QString(KEY_FOV), item.fov);
			obl.insert(QString(KEY_JD), item.jd);
			obl.insert(QString(KEY_LOCATION), item.location);
			obl.insert(QString(KEY_LANDSCAPE_ID), item.landscapeID);
			obl.insert(QString(KEY_MAGNITUDE), item.magnitude);
			obl.insert(QString(KEY_CONSTELLATION), item.constellation);
			obl.insert(QString(KEY_RA), item.ra);
			obl.insert(QString(KEY_DEC), item.dec);
			obl.insert(QString(KEY_TYPE), item.type);
			obl.insert(QString(KEY_OBJECTS_TYPE), item.objtype);
			obl.insert(QString(KEY_IS_VISIBLE_MARKER), item.isVisibleMarker);

			listOfObjects.push_back(obl);
		}

		bookmarksObsList.insert(QString(KEY_OBJECTS), listOfObjects);
		bookmarksObsList.insert(QString(KEY_SORTING), QString(SORTING_BY_NAME));

		QList<QString> keys = bookmarksCollection.keys();
		QString oblListUuid= (keys.empty() ? QUuid::createUuid().toString() : keys.at(0));

		allListsMap.insert(oblListUuid, bookmarksObsList);
}

/*
 * Check if bookmarks list already exists in observing list file,
 * in fact if the file of bookmarks has already be loaded.
*/
bool ObsListDialog::checkIfBookmarksListExists(const QVariantMap &allListsMap)
{
	QMapIterator<QString,QVariant>it(allListsMap);
	while(it.hasNext())
	{
		it.next();
		QVariantMap bookmarkData = it.value().toMap();
		QString listName = bookmarkData.value(KEY_NAME).toString();

		if (QString::compare(QString(BOOKMARKS_LIST_NAME), listName) == 0)
			return true;
	}
	return false;
}

/*
 * Select and go to object
*/
void ObsListDialog::selectAndGoToObject(QModelIndex index)
{
	QStandardItem *selectedItem = obsListListModel->itemFromIndex(index);
	int rowNumber = selectedItem->row();

	QStandardItem *uuidItem = obsListListModel->item(rowNumber, ColumnUUID);
	QString itemUuid = uuidItem->text();
	observingListItem item = observingListItemCollection.value(itemUuid);

	// Load landscape/location before dealing with the object: It could be a view fom another planet!
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

	bool objectFound = objectMgr->findAndSelect(item.name);
	if (!item.ra.isEmpty() && !item.dec.isEmpty() && (!objectFound || item.name.contains("marker", Qt::CaseInsensitive))) {
		Vec3d pos;
		StelUtils::spheToRect(StelUtils::getDecAngle(item.ra.trimmed()), StelUtils::getDecAngle(item.dec.trimmed()), pos);
		if (item.name.contains("marker", Qt::CaseInsensitive))
		{
			// Add a custom object on the sky
			GETSTELMODULE (CustomObjectMgr)->addCustomObject(item.name, pos, item.isVisibleMarker);
			objectFound = objectMgr->findAndSelect(item.name);
		}
		else
		{
			// The unnamed stars
			StelObjectP sobj;
			const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
			double fov = (item.fov > 0.0 ? item.fov : 5.0);

			mvmgr->zoomTo(fov, 0.0);
			mvmgr->moveToJ2000(pos, mvmgr->mountFrameToJ2000(Vec3d(0., 0., 1.)), 0.0);

			QList<StelObjectP> candidates = GETSTELMODULE (StarMgr)->searchAround(pos, 0.5, core);
			if (candidates.empty()) { // The FOV is too big, let's reduce it to see dimmer objects.
				mvmgr->zoomTo(0.5 * fov, 0.0);
				candidates = GETSTELMODULE (StarMgr)->searchAround(pos, 0.5, core);
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

			if (sobj != nullptr)
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
	ui->obsListEditListButton->setEnabled(true);
	ui->obsListDeleteButton->setEnabled(true);
	QString listUuid = ui->obsListComboBox->itemData(selectedIndex).toString();
	selectedObservingListUuid = listUuid;
	loadSelectedObservingListFromJsonFile(listUuid);
}

/*
 * Delete the selected list
*/
void ObsListDialog::obsListDeleteButtonPressed()
{
	if (askConfirmation())
	{
		QVariantMap map;
		QFile jsonFile(observingListJsonPath);
		if (!jsonFile.open(QIODevice::ReadWrite))
			qWarning() << "[ObservingList] cannot open" << QDir::toNativeSeparators(JSON_FILE_NAME);
		else
		{
			try {
				QVariantMap newMap;
				QVariantMap newObsListMap;
				map = StelJsonParser::parse(jsonFile.readAll()).toMap();

				newMap.insert(QString(KEY_DEFAULT_LIST_OLUD), map.value(QString(KEY_DEFAULT_LIST_OLUD)));
				QVariantMap obsListMap = map.value(QString(KEY_OBSERVING_LISTS)).toMap();

				QMap<QString, QVariant>::iterator i;
				for (i = obsListMap.begin(); i != obsListMap.end(); ++i)
				{
					if (i.key().compare(selectedObservingListUuid) != 0)
						newObsListMap.insert(i.key(), i.value());
				}

				newMap.insert(QString(KEY_OBSERVING_LISTS), newObsListMap);
				newMap.insert(QString(KEY_SHORT_NAME), map.value(QString(KEY_SHORT_NAME)));
				newMap.insert(QString(KEY_VERSION), map.value(QString(KEY_VERSION)));
				//objectMgr->unSelect();
				observingListItemCollection.clear();

				// Clear row in model
				obsListListModel->removeRows(0, obsListListModel->rowCount()); // don't just clear()!
				ui->obsListCreationDateLineEdit->clear();
				ui->descriptionLineEdit->clear();
				int currentIndex = ui->obsListComboBox->currentIndex();
				ui->obsListComboBox->removeItem(currentIndex);

				selectedObservingListUuid = "";

				jsonFile.resize(0);
				StelJsonParser::write(newMap, &jsonFile);
				jsonFile.flush();
				jsonFile.close();

				clearHighlights();
				loadListsNameFromJsonFile();

				if (ui->obsListComboBox->count() > 0)
				{
					ui->obsListComboBox->setCurrentIndex(0);
					loadSelectedObservingList(0);
				}

			} catch (std::runtime_error &e) {
				qWarning() << "[ObservingList] File format is wrong! Error: " << e.what();
				return;
			}
		}
	}
}

/*
 * Slot to manage the close of obsListCreateEditDialog.
*/
void ObsListDialog::finishEditMode()
{
	// The defaultListOlud may have changed in the create dialog.
	// The defaultListOlud needs to be updated.
	defaultListOlud_ = extractDefaultListOludFromJsonFile();

	// We must reload the list name
	loadListsNameFromJsonFile();
	int index = 0;
	if (!selectedObservingListUuid.isEmpty())
		index = ui->obsListComboBox->findData(selectedObservingListUuid);

	// Reload of the selected observing list.
	if (index != -1) {
		ui->obsListComboBox->setCurrentIndex(index);
		loadSelectedObservingList(index);
	}

	ui->stelWindowTitle->setText(q_("Observing lists"));
	//ObsListCreateEditDialog::kill();
	//createEditDialog_instance = Q_NULLPTR;
}

void ObsListDialog::switchEditMode(bool enableEditMode)
{
		isEditMode=enableEditMode;
		// The Layout classes have no setVisible(bool), we must configure individual buttons! :-(

		//ui->horizontalLayoutCombo->setEnabled(!isEditMode);     // disable list selection
		ui->obsListComboLabel->setVisible(!isEditMode);
		ui->obsListComboBox->setVisible(!isEditMode);

		// horizontalLayoutLineEdit_1: labelListName, nameOfListLIneEdit
		//ui->horizontalLayoutLineEdit_1->setEnabled(isEditMode);  // enable list name editing
		ui->labelListeName->setVisible(isEditMode);
		//ui->horizontalSpacer->sizePolicy().setHeightForWidth(isEditMode);// ->setVisible(isEditMode);
		qDebug() << "Spacer geometry, policy:" << ui->horizontalSpacer->geometry() << ui->horizontalSpacer->sizePolicy();
		ui->nameOfListLineEdit->setVisible(isEditMode);

		ui->descriptionLineEdit->setEnabled(isEditMode);    // (activate description line)

		ui->creationDateLabel->setVisible(!isEditMode);         // Creation date:
		ui->obsListCreationDateLineEdit->setVisible(!isEditMode);      //

		// line with optional store items
		ui->alsoStoreLabel->setVisible(isEditMode);            // Also store
		ui->obsListCoordinatesCheckBox->setVisible(isEditMode);// hide "Coordinates"
		ui->useOptionalElementsLabel->setVisible(!isEditMode);  // Also load

		//ui->horizontalLayoutButtons->setEnabled(!isEditMode);   // Highlight/Clear/NewList/EditList/DeleteList
		ui->obsListHighlightAllButton->setVisible(!isEditMode);
		ui->obsListClearHighlightButton->setVisible(!isEditMode);
		ui->obsListNewListButton->setVisible(!isEditMode);
		ui->obsListEditListButton->setVisible(!isEditMode);
		ui->obsListDeleteButton->setVisible(!isEditMode);

		//ui->horizontalLayoutButtons_1->setEnabled(isEditMode); // Add/Remove/Export/Import
		ui->obsListAddObjectButton->setVisible(isEditMode);
		ui->obsListRemoveObjectButton->setVisible(isEditMode);
		ui->obsListExportListButton->setVisible(isEditMode);
		ui->obsListImportListButton->setVisible(isEditMode);

		//ui->horizontalLayoutButtons_2->setEnabled(isEditMode); // Save and close/Cancel
		ui->obsListSaveButton->setVisible(isEditMode);
		ui->obsListExitButton->setVisible(isEditMode);
}

/*
 * Extract the defaultListOlud from the Json file.
*/
QString ObsListDialog::extractDefaultListOludFromJsonFile()
{
	QFile jsonFile(observingListJsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly))
		qWarning() << "[ObservingList] cannot open" << QDir::toNativeSeparators(JSON_FILE_NAME);
	else
	{
		try {
			QVariantMap map = StelJsonParser::parse(jsonFile.readAll()).toMap();
			jsonFile.close();

			// Return the default list uuid
			return map.value(KEY_DEFAULT_LIST_OLUD).toString();
		} catch (std::runtime_error &e) {
			qWarning() << "[ObservingList] File format is wrong! Error: ()()" << e.what();
			return QString();
		}
	}
	return QString();
}

/*
 * Override of the StelDialog::setVisible((bool) method
 * We need to load the default list when opening the obsListDialog
*/
void ObsListDialog::setVisible(bool v)
{
	StelDialog::setVisible(v);
	if (v)
		this->loadDefaultList();
}

/*
 * Sort the obsListTreeView by the column name given in parameter
*/
void ObsListDialog::sortObsListTreeViewByColumnName(const QString &columnName)
{
	static const QMap<QString,int>map={
		{SORTING_BY_NAME,          COLUMN_NUMBER_NAME},
		{SORTING_BY_NAMEI18N,      COLUMN_NUMBER_NAMEI18N},
		{SORTING_BY_TYPE,          COLUMN_NUMBER_TYPE},
		{SORTING_BY_RA,            COLUMN_NUMBER_RA},
		{SORTING_BY_DEC,           COLUMN_NUMBER_DEC},
		{SORTING_BY_MAGNITUDE,     COLUMN_NUMBER_MAGNITUDE},
		{SORTING_BY_CONSTELLATION, COLUMN_NUMBER_CONSTELLATION},
		{SORTING_BY_DATE,          COLUMN_NUMBER_DATE},
		{SORTING_BY_LOCATION,      COLUMN_NUMBER_LOCATION},
		{SORTING_BY_LANDSCAPE_ID,  COLUMN_NUMBER_LANDSCAPE}
	};
	obsListListModel->sort(map.value(columnName), Qt::AscendingOrder);
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


/* METHODS MOVED OVER FROM ObsListCreateEditDialog */


/*
 * Save observed objects and the list into Json file
 */
void ObsListDialog::saveObservedObjectsInJsonFile()
{
	if (observingListJsonPath.isEmpty())
	{
		qWarning() << "[ObservingList Creation/Edition] Error saving observing list: empty filename";
		return;
	}

	QFile jsonFile(observingListJsonPath);
	if (!jsonFile.open(QIODevice::ReadWrite | QIODevice::Text))
	{
		qWarning() << "[ObservingList Creation/Edition] Error saving observing list. "
			   << "File cannot be opened for reading and writing:"
			   << QDir::toNativeSeparators(observingListJsonPath);
		return;
	}

	// Creation date
	const double JD = core->getJD();
	const QString listCreationDate = StelUtils::julianDayToISO8601String(JD + core->getUTCOffset(JD) / 24.).replace("T", " ");

	try {
		QVariantMap mapFromJsonFile; // Maps the whole JSON file
		QVariantMap allListsMap;     // Extracts just the observingLists, not the global metadata of the JSON file
		const QString oludQs = this->listOlud_;
		const QVariantMap currentList = allListsMap.value(oludQs).toMap();
		if (jsonFile.size() > 0)
		{
			mapFromJsonFile = StelJsonParser::parse(jsonFile.readAll()).toMap();
			allListsMap = mapFromJsonFile.value(QString(KEY_OBSERVING_LISTS)).toMap();
		}

		QVariantMap observingListDataList; // One particular observingList.

		// Name of the list
		observingListDataList.insert(QString(KEY_NAME), ui->nameOfListLineEdit->text());

		// Description of the list
		observingListDataList.insert(QString(KEY_DESCRIPTION), ui->descriptionLineEdit->text());

		// Creation date
		observingListDataList.insert(QString(KEY_CREATION_DATE), listCreationDate);

		// List of objects
		QVariantList listOfObjects;
		QHashIterator<QString, observingListItem> i(observingListItemCollection);
		while (i.hasNext())
		{
			i.next();
			observingListItem item = i.value();
			QVariantMap obl;

			// copy data
			obl.insert(QString(KEY_DESIGNATION),       item.name);
			obl.insert(QString(KEY_NAME_I18N),         item.nameI18n);
			obl.insert(QString(KEY_TYPE),              item.type);
			//obl.insert(QString(KEY_OBJECTS_TYPE),      item.objtype);
			obl.insert(QString(KEY_RA),                item.ra);
			obl.insert(QString(KEY_DEC),               item.dec);
			obl.insert(QString(KEY_MAGNITUDE),         item.magnitude);
			obl.insert(QString(KEY_CONSTELLATION),     item.constellation);
			obl.insert(QString(KEY_JD),                item.jd);
			obl.insert(QString(KEY_LOCATION),          item.location);
			obl.insert(QString(KEY_LANDSCAPE_ID),      item.landscapeID);
			obl.insert(QString(KEY_FOV),               (item.fov>1.e-6 ? item.fov : 0));
			obl.insert(QString(KEY_IS_VISIBLE_MARKER), item.isVisibleMarker);

			listOfObjects.push_back(obl);
		}
		observingListDataList.insert(QString(KEY_OBJECTS), listOfObjects);

		// Sorting
		QString existingSorting;
		if (!isCreationMode)
			existingSorting = currentList.value(QString(KEY_SORTING)).toString();
		observingListDataList.insert(QString(KEY_SORTING), (sorting.isEmpty() ? existingSorting : sorting));

		// Olud
		QString oblListOlud = (isCreationMode || isSaveAs) ? QUuid::createUuid().toString() : listOlud_;

		// Default list
		if (ui->obsListDefaultListCheckBox->isChecked())
			mapFromJsonFile.insert(KEY_DEFAULT_LIST_OLUD, oblListOlud);
		else
		{
			QString defaultListUuid = mapFromJsonFile.value(KEY_DEFAULT_LIST_OLUD).toString();
			if (defaultListUuid.isEmpty())
				mapFromJsonFile.insert(KEY_DEFAULT_LIST_OLUD, "");
			else
				if (QString::compare(defaultListUuid, listOlud_, Qt::CaseSensitive) == 0)
					mapFromJsonFile.insert(KEY_DEFAULT_LIST_OLUD, "");
		}

		// Version, Short name
		mapFromJsonFile.insert(KEY_VERSION, FILE_VERSION);
		mapFromJsonFile.insert(KEY_SHORT_NAME, SHORT_NAME_VALUE);

		allListsMap.insert(oblListOlud, observingListDataList);
		mapFromJsonFile.insert(QString(KEY_OBSERVING_LISTS), allListsMap);

		jsonFile.resize(0);
		StelJsonParser::write(mapFromJsonFile, &jsonFile);
		jsonFile.flush();
		jsonFile.close();

	} catch (std::runtime_error &e) {
		qCritical() << "[ObservingList Creation/Edition] File format is wrong! Error: " << e.what();
		return;
	}
}


/*
 * Load the observing list in case of edit mode
 */
void ObsListDialog::loadObservingList()
{
	QVariantMap map;
	QFile jsonFile(observingListJsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly)) {
		qWarning() << "[ObservingList Creation/Edition] cannot open" << QDir::toNativeSeparators(JSON_FILE_NAME);
	} else {
		try {
			map = StelJsonParser::parse(jsonFile.readAll()).toMap();
			jsonFile.close();

			// Get the default list uuid
			QString defaultListOlud = map.value(KEY_DEFAULT_LIST_OLUD).toString();
			if (defaultListOlud == listOlud_)
				ui->obsListDefaultListCheckBox->setChecked(true);

			observingListItemCollection.clear();
			const QString keyOlud = listOlud_;
			// extract the active obsList
			QVariantMap observingListMap = map.value(QString(KEY_OBSERVING_LISTS)).toMap().value(keyOlud).toMap();
			QVariantList listOfObjects;

			// List name
			currentListName = observingListMap.value(QString(KEY_NAME)).toString();
			ui->nameOfListLineEdit->setText(currentListName);

			// List description
			QString listDescription = observingListMap.value(QString(KEY_DESCRIPTION)).toString();
			ui->descriptionLineEdit->setText(listDescription);

			if (observingListMap.value(QString(KEY_OBJECTS)).canConvert<QVariantList>())
			{
				QVariant data = observingListMap.value(QString(KEY_OBJECTS));
				listOfObjects = data.value<QVariantList>();
			}
			else
			{
				qCritical() << "[ObservingList Creation/Edition] conversion error";
				return;
			}

			for (const QVariant &object: listOfObjects)
			{
				QVariantMap objectMap;
				if (object.canConvert<QVariantMap>())
				{
					objectMap = object.value<QVariantMap>();
					observingListItem item;

					//int lastRow = obsListListModel->rowCount();
					QString objectOlud = QUuid::createUuid().toString();

					item.name = objectMap.value(QString(KEY_DESIGNATION)).toString();
					item.nameI18n = objectMap.value(QString(KEY_NAME_I18N)).toString();
					item.fov = objectMap.value(QString(KEY_FOV)).toDouble();
					item.type = objectMap.value(QString(KEY_TYPE)).toString();
					item.objtype = objectMap.value(QString(KEY_OBJECTS_TYPE)).toString();
					item.ra = objectMap.value(QString(KEY_RA)).toString();
					item.dec = objectMap.value(QString(KEY_DEC)).toString();
					item.magnitude = objectMap.value(QString(KEY_MAGNITUDE)).toString();
					item.constellation = objectMap.value(QString(KEY_CONSTELLATION)).toString();
					item.jd = objectMap.value(QString(KEY_JD)).toDouble();
					item.location = objectMap.value(QString(KEY_LOCATION)).toString();
					item.landscapeID = objectMap.value(QString(KEY_LANDSCAPE_ID)).toString();
					item.isVisibleMarker = objectMap.value(QString(KEY_IS_VISIBLE_MARKER)).toBool();
					observingListItemCollection.insert(objectOlud, item);

					// Add data into model row
					QString dateStr;
					if (item.jd != 0.)
						dateStr = StelUtils::julianDayToISO8601String(item.jd + core->getUTCOffset(item.jd) / 24.).replace("T", " ");
					QString LocationStr;
					if (!item.location.isEmpty())
					{
						StelLocation loc=StelApp::getInstance().getLocationMgr().locationForString(item.location);
						LocationStr=loc.name;
					}
					addModelRow(objectOlud,
						    item.name,
						    item.nameI18n,
						    item.objtype,
						    item.ra,
						    item.dec,
						    item.magnitude,
						    item.constellation,
						    dateStr,
						    LocationStr,
						    item.landscapeID);
				}
				else
				{
					qCritical() << "[ObservingList Creation/Edition] conversion error";
					return;
				}
			}
		} catch (std::runtime_error &e) {
			qWarning() << "[ObservingList Creation/Edition] File format is wrong! Error: " << e.what();
			return;
		}
	}
}

/*
* Load the bookmarks of bookmarks.json file into observing lists file
* For no regression with must take into account the legacy bookmarks.json file
*/
void ObsListDialog::loadBookmarksInObservingList_old() {
	// We must keep selection for the user!
	const QList<StelObjectP>&existingSelection = objectMgr->getSelectedObject();
	QList<StelObjectP> existingSelectionToRestore;
	if (existingSelection.length()>0)
	{
		existingSelectionToRestore.append(existingSelection.at(0));
	}

	QVariantMap map;
	// Keep time in sync to fix slow down of time when we manipulate entries now
	const double currentJD = core->getJDOfLastJDUpdate();
	const qint64 millis = core->getMilliSecondsOfLastJDUpdate();

	QFile jsonFile(observingListJsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly))
		qWarning() << "[ObservingList] cannot open" << QDir::toNativeSeparators(observingListJsonPath);
	else
	{
		try {
			map = StelJsonParser::parse(jsonFile.readAll()).toMap();
			jsonFile.close();
			QVariantMap bookmarksMap = map.value(KEY_BOOKMARKS).toMap();
			observingListItemCollection.clear();

			QMapIterator<QString,QVariant> it(bookmarksMap);
			while (it.hasNext())
			{
				it.next();

				QVariantMap bookmarkData = it.value().toMap();
				observingListItem item;
				QString objectUuid = QUuid::createUuid().toString();
				//const int lastRow = obsListListModel->rowCount();

				// Name
				item.name = bookmarkData.value(KEY_NAME).toString();
				item.nameI18n = bookmarkData.value(KEY_NAME_I18N).toString();

				// We need to select the object to add additional information that is not in the Bookmark file
				if (objectMgr->findAndSelect(item.name) && !objectMgr->getSelectedObject().isEmpty()) {
					const QList<StelObjectP> &selectedObject = objectMgr->getSelectedObject();

					item.type = selectedObject[0]->getType();
					item.objtype = selectedObject[0]->getObjectType();
					item.jd = bookmarkData.value(KEY_JD).toDouble();
					if (item.jd!=0.)
					{
						core->setJD(item.jd);
						core->update(0.); // Force position updates
					}
					// Ra & Dec - ra and dec are not empty in case of Custom Object
					item.ra = bookmarkData.value(KEY_RA).toString();
					item.dec = bookmarkData.value(KEY_DEC).toString();
					if (item.ra.isEmpty() || item.dec.isEmpty()) {
						double ra, dec;
						StelUtils::rectToSphe(&ra, &dec, selectedObject[0]->getJ2000EquatorialPos(core));
						item.ra = StelUtils::radToHmsStr(ra, false).trimmed();
						item.dec = StelUtils::radToDmsStr(dec, false).trimmed();
					}
					item.magnitude = ObservingListUtil::getMagnitude(selectedObject, core);
					// Several data items were not part of the original bookmarks, so we have no entry.
					const Vec3d posNow = selectedObject[0]->getEquinoxEquatorialPos(core);
					item.constellation=core->getIAUConstellation(posNow);
					item.location = bookmarkData.value(KEY_LOCATION).toString();
					// No landscape in original bookmarks. Just leave empty.
					//item.landscapeID = bookmarkData.value(KEY_LANDSCAPE_ID).toString();
					item.fov = bookmarkData.value(KEY_FOV).toDouble();
					item.isVisibleMarker = bookmarkData.value(KEY_IS_VISIBLE_MARKER, false).toBool();

					// Add data into model row
					QString dateStr;
					if (item.jd != 0.)
						dateStr = StelUtils::julianDayToISO8601String(item.jd + core->getUTCOffset(item.jd) / 24.).replace("T", " ");

					QString LocationStr;
					if (!item.location.isEmpty())
					{
						StelLocation loc=StelApp::getInstance().getLocationMgr().locationForString(item.location);
						LocationStr=loc.name;
					}
					addModelRow(objectUuid,
						    item.name,
						    item.nameI18n,
						    item.objtype,
						    item.ra,
						    item.dec,
						    item.magnitude,
						    item.constellation,
						    dateStr,
						    LocationStr,
						    item.landscapeID);

					observingListItemCollection.insert(objectUuid, item);
				}
			}
		} catch (std::runtime_error &e) {
			qWarning() << "[ObservingList] Load bookmarks in observing list: File format is wrong! Error: " << e.what();
		}
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

/*
 * Initialize error mssage
 */
void ObsListDialog::initErrorMessage()
{
	ui->obsListErrorMessage->setHidden(true);
	ui->obsListErrorMessage->clear();
}

/*
 * Display error message
 */
void ObsListDialog::displayErrorMessage(const QString &message)
{
	ui->obsListErrorMessage->setHidden(false);
	ui->obsListErrorMessage->setText(message);
}


// NEW BUTTON PRESS PRIVATE METHODS

/*
 * Slot for button obsListAddObjectButton.
 * Save selected object into the list of observed objects.
 */
void ObsListDialog::obsListAddObjectButtonPressed()
{
	const double JD = core->getJD();
	const double fov = (ui->obsListFovCheckBox->isChecked() ? GETSTELMODULE(StelMovementMgr)->getCurrentFov() : -1.0);
	const QString Location = core->getCurrentLocation().serializeToLine(); // store completely
	const QString landscapeID=GETSTELMODULE(LandscapeMgr)->getCurrentLandscapeID();

	initErrorMessage();
	const QList<StelObjectP> &selectedObject = objectMgr->getSelectedObject();

	if (!selectedObject.isEmpty())
	{
// TBD: this test should prevent adding duplicate entries, but fails. Maybe for V1.1!
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

			const QString objectOlud = QUuid::createUuid().toString();

			// Object name (designation) and object name I18n
			item.name = selectedObject[0]->getEnglishName();
			item.nameI18n = selectedObject[0]->getNameI18n();
			if(item.nameI18n.isEmpty())
				item.nameI18n = QString(dash);
			// Check if the object name is empty.
			if (item.name.isEmpty())
			{
				if (selectedObject[0]->getType() == "Nebula")
					item.name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();
				else
				{
					item.name = "Unnamed object";
					if (item.nameI18n.isEmpty()) {
						item.nameI18n = q_("Unnamed object");
					}
				}
			}
			// Type, Object Type
			item.type = selectedObject[0]->getType();
			item.objtype = selectedObject[0]->getObjectTypeI18n();

			// Ra & Dec
			if (ui->obsListCoordinatesCheckBox->isChecked() || item.objtype == CUSTOM_OBJECT || item.name.isEmpty()) {
				double ra, dec;
				StelUtils::rectToSphe(&ra, &dec, selectedObject[0]->getJ2000EquatorialPos(core));
				item.ra  = StelUtils::radToHmsStr(ra,  false).trimmed();
				item.dec = StelUtils::radToDmsStr(dec, false).trimmed();
			}
			item.isVisibleMarker=!item.name.contains("marker", Qt::CaseInsensitive);
			item.magnitude = ObservingListUtil::getMagnitude(selectedObject, core);

			// Constellation
			const Vec3d posNow = selectedObject[0]->getEquinoxEquatorialPos(core);
			item.constellation = core->getIAUConstellation(posNow);


			// Optional: JD, Location, landscape, fov
			if (ui->obsListJDCheckBox->isChecked())
				item.jd = JD;
			if (ui->obsListLocationCheckBox->isChecked())
				item.location = Location;
			if (ui->obsListLandscapeCheckBox->isChecked())
				item.landscapeID = landscapeID;
			if (ui->obsListFovCheckBox->isChecked() && (fov > 1.e-6))
				item.fov = fov;

			observingListItemCollection.insert(objectOlud, item);

			// Add object in row model
			StelLocation loc=StelLocation::createFromLine(Location);
			addModelRow(objectOlud,
				    item.name,
				    item.nameI18n,
				    item.objtype,
				    item.ra,
				    item.dec,
				    item.magnitude,
				    item.constellation,
				    ui->obsListJDCheckBox->isChecked() ? StelUtils::julianDayToISO8601String(JD + core->getUTCOffset(JD) / 24.).replace("T", " ") : "",
				    ui->obsListLocationCheckBox->isChecked() ? loc.name : "",
				    item.landscapeID);
//		}
	}
	else
		qWarning() << "Selected object is empty!";
}

/*
 * Slot for button obsListRemoveObjectButton
 */
void ObsListDialog::obsListRemoveObjectButtonPressed()
{
	initErrorMessage();
	int number = ui->obsListTreeView->currentIndex().row();
	QString uuid = obsListListModel->index(number, ColumnUUID).data().toString();
	obsListListModel->removeRow(number);
	observingListItemCollection.remove(uuid);
}

/*
 * Slot for button obsListExportListButton
 */
void ObsListDialog::obsListExportListButtonPressed()
{
	initErrorMessage();
	QString originalobservingListJsonPath = observingListJsonPath;

	static const QString filter = "JSON (*.json)";
	observingListJsonPath = QFileDialog::getSaveFileName(Q_NULLPTR, q_("Export observing list as..."),
							     QDir::homePath() + "/" + JSON_FILE_NAME, filter);
	saveObservedObjectsInJsonFile();
	observingListJsonPath = originalobservingListJsonPath;
}

/*
 * Slot for button obsListImportListButton
 */
void ObsListDialog::obsListImportListButtonPresssed()
{
	static const QString filter = "JSON (*.json)";
	QString fileToImportJsonPath = QFileDialog::getOpenFileName(Q_NULLPTR, q_("Import observing list"),
								    QDir::homePath(),
								    filter);
	QVariantMap map;
	QFile jsonFile(fileToImportJsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "[ObservingList Creation/Edition import] cannot open"
			   << QDir::toNativeSeparators(jsonFile.fileName());
	}
	else
	{
		try {
			initErrorMessage();
			map = StelJsonParser::parse(jsonFile.readAll()).toMap();
			jsonFile.close();

			if (map.contains(KEY_OBSERVING_LISTS))
			{
				// Case of observingList import
				QVariantMap observingListMap = map.value(QString(KEY_OBSERVING_LISTS)).toMap();
				if (!observingListMap.isEmpty() && observingListMap.size() == 1)
					//listOlud_ = observingListMap.keys().at(0).toStdString();
					listOlud_ = observingListMap.firstKey();
				else
				{
					qWarning() << "[ObservingList Creation/Edition import] there is no list or more than one list:" << observingListMap.size();
					displayErrorMessage(q_("Error: here is no list or more than one list."));
					return;
				}

				QString originalobservingListJsonPath = observingListJsonPath;
				observingListJsonPath = fileToImportJsonPath;
				loadObservingList();
				observingListJsonPath = originalobservingListJsonPath;
			}
			else if (map.contains(KEY_BOOKMARKS))
			{
				// Case of legacy bookmarks import
				QVariantMap bookmarksListMap = map.value(QString(KEY_BOOKMARKS)).toMap();
				if (!bookmarksListMap.isEmpty())
				{
					//listOlud_ = bookmarksListMap.keys().at(0).toStdString();
					listOlud_ = bookmarksListMap.firstKey();
					QString originalobservingListJsonPath = observingListJsonPath;
					observingListJsonPath = fileToImportJsonPath;
					loadBookmarksInObservingList();
					observingListJsonPath = originalobservingListJsonPath;
				}
				else
				{
					qWarning() << "[ObservingList Creation/Edition import] the file is empty or doesn't contain legacy bookmarks.";
					displayErrorMessage(q_("Error: the file is empty or doesn't contain legacy bookmarks."));
					return;
				}
			}
		} catch (std::runtime_error &e) {
			qWarning() << "[ObservingList Creation/Edition] File format is wrong! Error: " << e.what();
			displayErrorMessage(q_("File format is wrong!"));
			return;
		}
	}
}

/*
 * Slot for button obsListSaveButton
 */
void ObsListDialog::obsListSaveButtonPressed()
{
	initErrorMessage();
	QString listName = ui->nameOfListLineEdit->text().trimmed();

	bool isListAlreadyExists = (!this->listNames_.isEmpty() && this->listNames_.contains(listName) && isCreationMode);

	if (isListAlreadyExists) {
		qWarning() << "[ObservingList Creation/Edition] Error: a list with the name " << ui->nameOfListLineEdit->text()
			   << " already exists!";
		displayErrorMessage(QString(q_("Error: a list with the name %1 already exists!")).arg(ui->nameOfListLineEdit->text()));
	}
	else if (ui->nameOfListLineEdit->text().isEmpty())
	{
		qWarning() << "[ObservingList Creation/Edition] Error: the list name is empty.";
		displayErrorMessage(q_("Error: the list name is empty."));
	}
	else
	{
		isSaveAs = listName.compare(currentListName) != 0 && !isCreationMode;
		//qDebug() << "ObsListCreateEditDialog::obsListSaveButtonPressed(): isSaveAs:" << isSaveAs;
		saveObservedObjectsInJsonFile();
		emit exitButtonClicked();
	}
	switchEditMode(false);
}

/*
 * Slot for button obsListExitButton
 */
void ObsListDialog::obsListCancelButtonPressed()
{
	ui->obsListErrorMessage->setHidden(true);
	ui->obsListErrorMessage->clear();
	emit this->exitButtonClicked();
	switchEditMode(false);
}

/*
 * Slot for obsListCreationEditionTreeView header
 */
void ObsListDialog::headerClicked(int index)
{
	static const QMap<int,QString> map={
		{ColumnName,          QString(SORTING_BY_NAME)},
		{ColumnNameI18n,      QString(SORTING_BY_NAMEI18N)},
		{ColumnType,          QString(SORTING_BY_TYPE)},
		{ColumnRa,            QString(SORTING_BY_RA)},
		{ColumnDec,           QString(SORTING_BY_DEC)},
		{ColumnMagnitude,     QString(SORTING_BY_MAGNITUDE)},
		{ColumnConstellation, QString(SORTING_BY_CONSTELLATION)},
		{ColumnDate,          QString(SORTING_BY_DATE)},
		{ColumnLocation,      QString(SORTING_BY_LOCATION)},
		{ColumnLandscapeID,   QString(SORTING_BY_LANDSCAPE_ID)}};
	sorting=map.value(index, "");
	//qDebug() << "Sorting = " << sorting;
}

/*
 * Called when the text of the nameOfListLineEdit change
 */
void ObsListDialog::nameOfListTextChange(const QString &newText)
{
	ui->obsListErrorMessage->setHidden(true);
	//delete whitespace -> no list with only white space as name
	QString listName = newText.trimmed();
	//qDebug() << "listName:" << listName;
	ui->obsListSaveButton->setEnabled(!(listName.isEmpty()));
}
