/*
 * * Stellarium
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
#include <StelTranslator.hpp>
#include <QMessageBox>

#include "NebulaMgr.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocationMgr.hpp"
#include "LandscapeMgr.hpp"
#include "StelUtils.hpp"

#include "ObsListCreateEditDialog.hpp"
#include "ui_obsListCreateEditDialog.h"

ObsListCreateEditDialog *ObsListCreateEditDialog::m_instance = nullptr;

ObsListCreateEditDialog::ObsListCreateEditDialog(const QString &listUuid) :
	StelDialog("ObservingListCreateEdit"),
	ui(new Ui_obsListCreateEditDialogForm()),
	isCreationMode(false),
	isSaveAs(false),
	obsListListModel(new QStandardItemModel(0, ColumnCount)),
	core(StelApp::getInstance().getCore()),
	listOlud_(listUuid),
	sorting("")
{
	objectMgr = GETSTELMODULE(StelObjectMgr);
	landscapeMgr = GETSTELMODULE(LandscapeMgr);
	observingListJsonPath =
			StelFileMgr::findFile("data",
					      static_cast<StelFileMgr::Flags>(StelFileMgr::Directory | StelFileMgr::Writable)) +
			"/" +
			QString(JSON_FILE_NAME);
}

ObsListCreateEditDialog::~ObsListCreateEditDialog() {
	delete ui;
	delete obsListListModel;
	ui = Q_NULLPTR;
	obsListListModel = Q_NULLPTR;
}

/**
 * Get singleton instance of class
 */
ObsListCreateEditDialog * ObsListCreateEditDialog::Instance(const QString &listUuid)
{
	if (!m_instance)
		m_instance = new ObsListCreateEditDialog(listUuid);
	return m_instance;
}

/*
 * Initialize the dialog widgets and connect the signals/slots.
 */
void ObsListCreateEditDialog::createDialogContent()
{
	ui->setupUi(dialog);

	// Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(ui->obsListAddObjectButton,    SIGNAL(clicked()),     this, SLOT(obsListAddObjectButtonPressed()));
	connect(ui->obsListExitButton,         SIGNAL(clicked()),     this, SLOT(obsListExitButtonPressed()));
	connect(ui->obsListSaveButton,         SIGNAL(clicked()),     this, SLOT(obsListSaveButtonPressed()));
	connect(ui->obsListRemoveObjectButton, SIGNAL(clicked()),     this, SLOT(obsListRemoveObjectButtonPressed()));
	connect(ui->obsListImportListButton,   SIGNAL(clicked()),     this, SLOT(obsListImportListButtonPresssed()));
	connect(ui->obsListExportListButton,   SIGNAL(clicked()),     this, SLOT(obsListExportListButtonPressed()));
	connect(ui->nameOfListLineEdit,        SIGNAL(textChanged(const QString&)), this, SLOT(nameOfListTextChange(const QString&)));
	connectBoolProperty(ui->obsListJDCheckBox,        "ObsListDialog.flagUseJD");
	connectBoolProperty(ui->obsListLocationCheckBox,  "ObsListDialog.flagUseLocation");
	connectBoolProperty(ui->obsListLandscapeCheckBox, "ObsListDialog.flagUseLandscape");
	connectBoolProperty(ui->obsListFovCheckBox,       "ObsListDialog.flagUseFov");


	// Initializing the list of observing list
	//obsListListModel->setColumnCount(ColumnCount); // has been done in constructor
	setObservingListHeaderNames();

	ui->obsListCreationEditionTreeView->setModel(obsListListModel);
	ui->obsListCreationEditionTreeView->header()->setSectionsMovable(false);
	ui->obsListCreationEditionTreeView->hideColumn(ColumnUUID);
	ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnName,          QHeaderView::ResizeToContents);
	ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnNameI18n,      QHeaderView::ResizeToContents);
	ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnType,          QHeaderView::ResizeToContents);
	ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnRa,            QHeaderView::ResizeToContents);
	ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnDec,           QHeaderView::ResizeToContents);
	ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnMagnitude,     QHeaderView::ResizeToContents);
	ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnConstellation, QHeaderView::ResizeToContents);
	ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnDate,          QHeaderView::ResizeToContents);
	ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnLocation,      QHeaderView::ResizeToContents);
	ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnLandscapeID,   QHeaderView::ResizeToContents);
	ui->obsListCreationEditionTreeView->header()->setStretchLastSection(true);
	// Enable the sort for columns
	ui->obsListCreationEditionTreeView->setSortingEnabled(true);

	QHeaderView *header = ui->obsListCreationEditionTreeView->header();
	connect(header, SIGNAL(sectionClicked(int)), this, SLOT(headerClicked(int)));

	// We hide the closeStelWindow to have only two possibilities to close the dialog:
	// Save and close and Cancel
	ui->closeStelWindow->setHidden(true);

	initErrorMessage();
	ui->obsListLandscapeCheckBox->setChecked(false);

	// In case of creation the nameOfListLineEdit is empty and button save/close must be disabled
	// In case on edition the nameOfListLineEdit is not empty and the button save/close must be enable
	// delete with space -> no list with only white space as name
	ui->nameOfListLineEdit->setText(QString(ui->nameOfListLineEdit->text()).trimmed());
	ui->obsListSaveButton->setEnabled(!(ui->nameOfListLineEdit->text().isEmpty()));

	if (listOlud_.isEmpty())
	{
		// case of creation mode
		isCreationMode = true;
		ui->stelWindowTitle->setText("Observing list creation mode");
		ui->obsListImportListButton->setHidden(false);
	}
	else
	{
		// case of edit mode
		isCreationMode = false;
		ui->stelWindowTitle->setText("Observing list editor mode");
		ui->obsListImportListButton->setHidden(true);
		loadObservingList();
	}
}

/*
 * Retranslate dialog
 */
void ObsListCreateEditDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setObservingListHeaderNames();
	}
}

/*
 * Set the header for the observing list table
 * (obsListTreeVView)
 */
void ObsListCreateEditDialog::setObservingListHeaderNames()
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
void ObsListCreateEditDialog::addModelRow(const QString &olud,
                                          const QString &name,
                                          const QString &nameI18n,
                                          const QString &objtype,
                                          const QString &ra,
                                          const QString &dec,
                                          const QString &magnitude,
					  const QString &constellation,
					  const QString &date,
					  const QString &location,
					  const QString &landscapeID)
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

	item = new QStandardItem(objtype);
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
		ui->obsListCreationEditionTreeView->resizeColumnToContents(i);
}

/*
 * Slot for button obsListAddObjectButton.
 * Save selected object into the list of observed objects.
 */
void ObsListCreateEditDialog::obsListAddObjectButtonPressed()
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
			util.initItem(item);

			const QString objectOlud = QUuid::createUuid().toString();

			// Object name (designation) and object name I18n
			item.name = selectedObject[0]->getEnglishName();
			item.nameI18n = selectedObject[0]->getNameI18n();
			if(item.nameI18n.isEmpty())
				item.nameI18n = dash;
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
			item.magnitude = util.getMagnitude(selectedObject, core);

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
void ObsListCreateEditDialog::obsListRemoveObjectButtonPressed()
{
	initErrorMessage();
	int number = ui->obsListCreationEditionTreeView->currentIndex().row();
	QString uuid = obsListListModel->index(number, ColumnUUID).data().toString();
	obsListListModel->removeRow(number);
	observingListItemCollection.remove(uuid);
}

/*
 * Save observed objects and the list into Json file
 */
void ObsListCreateEditDialog::saveObservedObjectsInJsonFile()
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
 * Slot for button obsListExportListButton
 */
void ObsListCreateEditDialog::obsListExportListButtonPressed()
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
void ObsListCreateEditDialog::obsListImportListButtonPresssed()
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
					qWarning() << "[ObservingList Creation/Edition import] the file is empty or doesn't contains legacy bookmarks.";
					displayErrorMessage(q_("Error: the file is empty or doesn't contains legacy bookmarks."));
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
void ObsListCreateEditDialog::obsListSaveButtonPressed()
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
		this->close();
		emit exitButtonClicked();
	}
}

/*
 * Slot for button obsListExitButton
 */
void ObsListCreateEditDialog::obsListExitButtonPressed()
{
	ui->obsListErrorMessage->setHidden(true);
	ui->obsListErrorMessage->clear();
	this->close();
	emit this->exitButtonClicked();

}

/*
 * Overload StelDialog::close()
 */
void ObsListCreateEditDialog::close()
{
	this->setVisible(false);
	//emit this->exitButtonClicked(); // Disabled says JC.
}

/*
 * Slot for obsListCreationEditionTreeView header
 */
void ObsListCreateEditDialog::headerClicked(int index)
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
 * Load the observing list in case of edit mode
 */
void ObsListCreateEditDialog::loadObservingList()
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
					util.initItem(item);

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
void ObsListCreateEditDialog::loadBookmarksInObservingList() {
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
				util.initItem(item);
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
					item.magnitude = util.getMagnitude(selectedObject, core);
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
 * Called when the text of the nameOfListLineEdit change
 */
void ObsListCreateEditDialog::nameOfListTextChange(const QString &newText)
{
	ui->obsListErrorMessage->setHidden(true);
	//delete whitespace -> no list with only white space as name
	QString listName = newText.trimmed();
	qDebug() << "listName:" << listName;
	ui->obsListSaveButton->setEnabled(!(listName.isEmpty()));
}

/*
 * Setter for listName
 */
void ObsListCreateEditDialog::setListName(const QList<QString> &listName)
{
	this->listNames_ = listName;
}

/*
 * Initialize error mssage
 */
void ObsListCreateEditDialog::initErrorMessage()
{
	ui->obsListErrorMessage->setHidden(true);
	ui->obsListErrorMessage->clear();
}

/*
 * Display error message
 */
void ObsListCreateEditDialog::displayErrorMessage(const QString &message)
{
	ui->obsListErrorMessage->setHidden(false);
	ui->obsListErrorMessage->setText(message);
}

/*
 * Destructor of singleton
 */
void ObsListCreateEditDialog::kill()
{
	if (m_instance)
	{
		delete m_instance;
		m_instance = Q_NULLPTR;
	}
}
