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

#include <StelTranslator.hpp>
#include <QDir>
#include <QUuid>

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
#include "StelJsonParser.hpp"
#include "StelUtils.hpp"
#include "ObsListDialog.hpp"
#include "LabelMgr.hpp"

#include "ui_obsListDialog.h"
#include <QSortFilterProxyModel>
#include <utility>

using namespace std;


ObsListDialog::ObsListDialog(QObject *parent) :
	StelDialog("ObservingList", parent),
	ui(new Ui_obsListDialogForm()),
	obsListListModel(new QStandardItemModel(0, ColumnCount)),
	core(StelApp::getInstance().getCore()),
	//currentJd(core->getJD()),
	createEditDialog_instance(Q_NULLPTR)
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

	connect(ui->obsListNewListButton,        SIGNAL(clicked()), this, SLOT(obsListNewListButtonPressed()));
	connect(ui->obsListEditListButton,       SIGNAL(clicked()), this, SLOT(obsListEditButtonPressed()));
	connect(ui->obsListClearHighlightButton, SIGNAL(clicked()), this, SLOT(obsListClearHighLightButtonPressed()));
	connect(ui->obsListHighlightAllButton,   SIGNAL(clicked()), this, SLOT(obsListHighLightAllButtonPressed()));
	connect(ui->obsListExitButton,           SIGNAL(clicked()), this, SLOT(obsListExitButtonPressed()));
	connect(ui->obsListDeleteButton,         SIGNAL(clicked()), this, SLOT(obsListDeleteButtonPressed()));
	connect(ui->obsListTreeView,             SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectAndGoToObject(QModelIndex)));

	connectBoolProperty(ui->obsListJdCheckBox,        "ObsListDialog.flagUseJD");
	connectBoolProperty(ui->obsListLocationCheckBox,  "ObsListDialog.flagUseLocation");
	connectBoolProperty(ui->obsListLandscapeCheckBox, "ObsListDialog.flagUseLandscape");
	connectBoolProperty(ui->obsListFovCheckBox,       "ObsListDialog.flagUseFov");

	//obsListCombo settings
	connect(ui->obsListComboBox, SIGNAL(activated(int)), this, SLOT(loadSelectedObservingList(int)));

	//Initialize the list of observing lists
	//obsListListModel->setColumnCount(ColumnCount); Has been done in constructor.
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


	//By default buttons are disable
	ui->obsListEditListButton->setEnabled(false);
	ui->obsListHighlightAllButton->setEnabled(false);
	ui->obsListClearHighlightButton->setEnabled(false);
	ui->obsListDeleteButton->setEnabled(false);

	// We hide the closeStelWindow to have only two possibilities to close the dialog:
	// Exit
	ui->closeStelWindow->setHidden(true);

	// For no regression we must take into account the legacy bookmarks file
	QFile jsonBookmarksFile(bookmarksJsonPath);
	if (jsonBookmarksFile.exists())
		loadBookmarksInObservingList();

	QFile jsonFile(observingListJsonPath);
	if (jsonFile.exists())
	{
		loadListsNameFromJsonFile();
		defaultListOlud_ = extractDefaultListOludFromJsonFile();
		loadDefaultList();
	}
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
void ObsListDialog::addModelRow(//const int number,
				const QString &olud, const QString &name, const QString &nameI18n,
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
void ObsListDialog::obsListHighLightAllButtonPressed()
{
	// We must keep selection for the user!
	const QList<StelObjectP>&existingSelection = objectMgr->getSelectedObject();

	QList<Vec3d> highlights;
	highlights.clear();
	obsListClearHighLightButtonPressed(); // Enable fool protection
	const int fontSize = StelApp::getInstance().getScreenFontSize();
	HighlightMgr *hlMgr = GETSTELMODULE(HighlightMgr);
	const QString color = hlMgr->getColor().toHtmlColor();
	float distance = hlMgr->getMarkersSize();

	for (const auto &item: qAsConst(observingListItemCollection)) {
		QString name = item.name;
		QString raStr = item.ra.trimmed();
		QString decStr = item.dec.trimmed();

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
			const QList<StelObjectP> &selected = objectMgr->getSelectedObject();
			if (!selected.isEmpty()) {
				pos = selected[0]->getJ2000EquatorialPos(core);
			}
		}

		if (usablePosition)
			highlights.append(pos);

		//objectMgr->unSelect();
		// Add labels for named highlights (name in top right corner)
		highlightLabelIDs.append(labelMgr->labelObject(name, name, true, fontSize, color, "NE", distance));
	}

	hlMgr->fillHighlightList(highlights);
	// Restore selection that was active before calling this
	objectMgr->setSelectedObject(existingSelection, StelModule::ReplaceSelection);
}

/*
 * Slot for button obsListClearHighLightButton
*/
void ObsListDialog::obsListClearHighLightButtonPressed()
{
	clearHighlight();
}

/*
 * Clear highlight
*/
void ObsListDialog::clearHighlight()
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
	string olud = string();
	invokeObsListCreateEditDialog(olud);
}
/*
 * Slot for button obsListEditButton
*/
void ObsListDialog::obsListEditButtonPressed()
{
	if (!selectedObservingListUuid.empty())
		invokeObsListCreateEditDialog(selectedObservingListUuid);
	else
		qWarning() << "The selected observing list olud is empty";
}

/**
 * Open the observing list create/edit dialog
*/
void ObsListDialog::invokeObsListCreateEditDialog(string listOlud) {
	createEditDialog_instance = ObsListCreateEditDialog::Instance(std::move(listOlud));
	connect(createEditDialog_instance, SIGNAL (exitButtonClicked()), this, SLOT (obsListCreateEditDialogClosed()));
	createEditDialog_instance->setListName(listName_);
	createEditDialog_instance->setVisible(true);
}

/*
 * Load the lists names from Json file
 * to populate the combo box and get the default list uuid
*/
void ObsListDialog::loadListsNameFromJsonFile()
{
	QVariantMap map;
	QFile jsonFile(observingListJsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly))
		qWarning() << "[ObservingList] cannot open" << QDir::toNativeSeparators(JSON_FILE_NAME);
	else
	{
		try {
			map = StelJsonParser::parse(jsonFile.readAll()).toMap();
			jsonFile.close();

			// init combo box
			ui->obsListComboBox->clear();

			QVariantMap observingListsMap = map.value(QString(KEY_OBSERVING_LISTS)).toMap();
			populateListNameInComboBox(observingListsMap);
			populateDataInComboBox(observingListsMap, defaultListOlud_);

		} catch (std::runtime_error &e) {
			qWarning() << "[ObservingList] File format is wrong! Error: " << e.what();
			return;
		}
	}
}

/*
 * Populate the list names into combo box
*/
void ObsListDialog::populateListNameInComboBox(QVariantMap map)
{
	QMap<QString, QVariant>::iterator i;
	ui->obsListComboBox->clear();
	listNamesModel.clear();
	listName_.clear();
	for (i = map.begin(); i != map.end(); ++i) {
		if (i.value().canConvert<QVariantMap>())
		{
			QVariant var = i.value();
			auto data = var.value<QVariantMap>();
			QString listName = data.value(KEY_NAME).toString();
			listNamesModel.append(listName);
			listName_.append(listName);
		}
	}
	listNamesModel.sort(Qt::CaseInsensitive);
	ui->obsListComboBox->addItems(listNamesModel);
}

/*
 * Populate data into combo box
*/
void ObsListDialog::populateDataInComboBox(QVariantMap map, const QString &defaultListOlud)
{
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
					int index = listNamesModel.indexOf(listName, 0);
					ui->obsListComboBox->setItemData(index, listUuid);
					break;
				}
			}
		}
		if (defaultListOlud == listUuid)
			defaultListOlud_ = defaultListOlud;
	}
}


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
			ui->obsListEditListButton->setEnabled(true);
			selectedObservingListUuid = defaultListOlud_.toStdString();
			loadSelectedObservingListFromJsonFile(defaultListOlud_);
		}
	}
	else
	{
		// If there is no default list we load the first list in the combo box
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
		// The QList<StelObjectP> objects are apparently volatile. WE must take out the actual object.
		const QList<StelObjectP>&existingSelection = objectMgr->getSelectedObject();
		StelObject *preSelectedObject=Q_NULLPTR;
		if (existingSelection.length()>0)
		{
			preSelectedObject=existingSelection[0].data();
			qDebug() << "\t Selected object: " << existingSelection[0]->getEnglishName();
		}

		try {
			map = StelJsonParser::parse(jsonFile.readAll()).toMap();
			QVariantMap observingListMap = map.value(QString(KEY_OBSERVING_LISTS)).toMap().value(listOlud).toMap();

			// Check or not default list checkbox information
			ui->obsListIsDefaultListCheckBox->setChecked(listOlud.compare(defaultListOlud_, Qt::CaseSensitive) == 0);

			//// Landscape
			//QString landscapeId = observingListMap.value(KEY_LANDSCAPE_ID).toString();
			//if (!landscapeId.isEmpty()) {
			//    landscapeMgr->setCurrentLandscapeID(landscapeId);
			//}

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
						util.initItem(item);
						const QString objectOlud = QUuid::createUuid().toString();
						item.name = objectMap.value(QString(KEY_DESIGNATION)).toString();
						item.nameI18n = objectMap.value(QString(KEY_NAME_I18N)).toString();
						double fov = -1.0;

						// FIXME?: There was a mismatch!
						item.type    = objectMap.value(QString(KEY_TYPE)).toString();
						item.objtype = objectMap.value(QString(KEY_OBJECTS_TYPE)).toString();

						item.ra  = objectMap.value(QString(KEY_RA)).toString();
						item.dec = objectMap.value(QString(KEY_DEC)).toString();

						if (objectMgr->findAndSelect(item.name) && !objectMgr->getSelectedObject().isEmpty())
						{
							const QList<StelObjectP> &selectedObject = objectMgr->getSelectedObject();
							double ra, dec;
							StelUtils::rectToSphe(&ra, &dec, selectedObject[0]->getJ2000EquatorialPos(core));

							if (item.ra.isEmpty())
								item.ra = StelUtils::radToHmsStr(ra, false).trimmed();
							if (item.dec.isEmpty())
								item.dec = StelUtils::radToDmsStr(dec, false).trimmed();
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
			jsonFile.close();
		} catch (std::runtime_error &e) {
			qWarning() << "[ObservingList] Load selected observing list: File format is wrong! Error: " << e.what();
			jsonFile.close();
		}
		// Restore selection that was active before calling this
		if (preSelectedObject)
			objectMgr->setSelectedObject(StelObjectP(preSelectedObject), StelModule::ReplaceSelection);
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
	ui->obsListDescriptionTextEdit->setPlainText(listDescription);
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
	obsListListModel->clear(); //removeRows(0, obsListListModel->rowCount());
	return listOfObjects;
}


/*
 * Load the bookmarks of bookmarks.json file into observing lists file
 * For no regression with must take into account the legacy bookmarks.json file
*/
void ObsListDialog::loadBookmarksInObservingList()
{
	QHash<QString, observingListItem> bookmarksCollection;
	QVariantMap map;

	QFile jsonFile(bookmarksJsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly)) {
		qWarning() << "[ObservingList] cannot open" << QDir::toNativeSeparators(bookmarksJsonPath);
	}
	else
	{
		const double currentJD=core->getJD(); // Restore at end
		const qint64 millis = core->getMilliSecondsOfLastJDUpdate();

		// We must keep selection for the user!
		const QList<StelObjectP>&existingSelection = objectMgr->getSelectedObject();
		StelObject *preSelectedObject=Q_NULLPTR;
		if (existingSelection.length()>0)
		{
			preSelectedObject=existingSelection[0].data();
			qDebug() << "\t Selected object: " << existingSelection[0]->getEnglishName();
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
				util.initItem(item);

				// Name
				item.name = bookmarkData.value(KEY_NAME).toString();
				QString nameI18n = bookmarkData.value(KEY_NAME_I18N).toString();
				item.nameI18n = nameI18n.isEmpty() ? dash : nameI18n;

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
					item.constellation = core->getIAUConstellation(posNow);
					item.location = bookmarkData.value(KEY_LOCATION).toString();
					// No landscape in original bookmarks. Just leave empty.
					//item.landscapeID = bookmarkData.value(KEY_LANDSCAPE_ID).toString();
					item.fov = bookmarkData.value(KEY_FOV).toDouble();
					item.isVisibleMarker = bookmarkData.value(KEY_IS_VISIBLE_MARKER, false).toBool();

					bookmarksCollection.insert(it.key(), item);
				}
			}

			saveBookmarksInObsListJsonFile(bookmarksCollection);
			//objectMgr->unSelect();
		}
		catch (std::runtime_error &e)
		{
			qWarning() << "[ObservingList] Load bookmarks in observing list: File format is wrong! Error: " << e.what();
		}
		// Restore selection that was active before calling this
		if (preSelectedObject)
			objectMgr->setSelectedObject(StelObjectP(preSelectedObject), StelModule::ReplaceSelection);
		else
			objectMgr->unSelect();

		core->setJD(currentJD);
		core->setMilliSecondsOfLastJDUpdate(millis); // restore millis.
		core->update(0); // enforce update to the previous positions
	}
}

/*
 * Save the bookmarks into observing list file
*/
void ObsListDialog::saveBookmarksInObsListJsonFile(const QHash<QString, observingListItem> &bookmarksCollection)
{
	QFile jsonFile(observingListJsonPath);
	if (!jsonFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
		qWarning() << "[ObservingList] bookmarks list can not be saved. A file can not be open for reading and writing:"
			   << QDir::toNativeSeparators(observingListJsonPath);
		return;
	}

	try {
		QVariantMap mapFromJsonFile;
		QVariantMap allListsMap;
		QVariantMap observingListDataList;
		if (jsonFile.size() > 0) {
			mapFromJsonFile = StelJsonParser::parse(jsonFile.readAll()).toMap();
			allListsMap = mapFromJsonFile.value(QString(KEY_OBSERVING_LISTS)).toMap();
		}

		QString defaultListValue = mapFromJsonFile.value(QString(KEY_DEFAULT_LIST_OLUD)).toString();
		if (defaultListValue.isEmpty()) {
			mapFromJsonFile.insert(KEY_DEFAULT_LIST_OLUD, "");
		}

		if (checkIfBookmarksListExists(allListsMap)) {
			//the bookmarks file is already loaded
			return;
		}

		// Description
		observingListDataList.insert(QString(KEY_DESCRIPTION), QString(BOOKMARKS_LIST_DESCRIPTION));

		// Creation date
		double JD = core->getJD();
		QString listCreationDate = StelUtils::julianDayToISO8601String(JD + core->getUTCOffset(JD) / 24.).replace("T", " ");
		observingListDataList.insert(QString(KEY_CREATION_DATE), listCreationDate);

		//// Landscape
		//QString landscapeId = landscapeMgr->getCurrentLandscapeID();
		//observingListDataList.insert(QString(KEY_LANDSCAPE_ID), landscapeId);

		// Name of the liste
		observingListDataList.insert(QString(KEY_NAME), BOOKMARKS_LIST_NAME);

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

		observingListDataList.insert(QString(KEY_OBJECTS), listOfObjects);
		observingListDataList.insert(QString(KEY_SORTING), SORTING_BY_NAME);

		QList<QString> keys = bookmarksCollection.keys();
		QString oblListUuid= (keys.empty() ? QUuid::createUuid().toString() : keys.at(0));

		mapFromJsonFile.insert(KEY_VERSION, FILE_VERSION);
		mapFromJsonFile.insert(KEY_SHORT_NAME, SHORT_NAME_VALUE);

		allListsMap.insert(oblListUuid, observingListDataList);
		mapFromJsonFile.insert(QString(KEY_OBSERVING_LISTS), allListsMap);

		jsonFile.resize(0);
		StelJsonParser::write(mapFromJsonFile, &jsonFile);
		jsonFile.flush();
		jsonFile.close();
	}
	catch (std::runtime_error &e)
	{
		qCritical() << "[ObservingList] File format is wrong! Error: " << e.what();
		return;
	}
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
		core->moveObserverTo(StelApp::getInstance().getLocationMgr().locationForString(item.location));
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
	selectedObservingListUuid = listUuid.toStdString();
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
					if (i.key().compare(QString::fromStdString(selectedObservingListUuid)) != 0)
						newObsListMap.insert(i.key(), i.value());
				}

				newMap.insert(QString(KEY_OBSERVING_LISTS), newObsListMap);
				newMap.insert(QString(KEY_SHORT_NAME), map.value(QString(KEY_SHORT_NAME)));
				newMap.insert(QString(KEY_VERSION), map.value(QString(KEY_VERSION)));
				//objectMgr->unSelect();
				observingListItemCollection.clear();

				// Clear row in model
				//obsListListModel->removeRows(0, obsListListModel->rowCount());
				obsListListModel->clear(); // simpler
				ui->obsListCreationDateLineEdit->setText("");
				ui->obsListDescriptionTextEdit->setPlainText("");
				int currentIndex = ui->obsListComboBox->currentIndex();
				ui->obsListComboBox->removeItem(currentIndex);

				selectedObservingListUuid = "";

				jsonFile.resize(0);
				StelJsonParser::write(newMap, &jsonFile);
				jsonFile.flush();
				jsonFile.close();

				clearHighlight();
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
 * Slot for button obsListExitButton
*/
void ObsListDialog::obsListExitButtonPressed()
{
	this->close();
}

/*
 * Slot to manage the close of obsListCreateEditDialog.
*/
void ObsListDialog::obsListCreateEditDialogClosed()
{
	// The defaultListOlud may have changed in the create dialog.
	// The defaultListOlud needs to be updated.
	defaultListOlud_ = extractDefaultListOludFromJsonFile();

	// We must reload the list name
	loadListsNameFromJsonFile();
	int index = 0;
	if (!selectedObservingListUuid.empty())
		index = ui->obsListComboBox->findData(QString::fromStdString(selectedObservingListUuid));

	// Reload of the selected observing list.
	if (index != -1) {
		ui->obsListComboBox->setCurrentIndex(index);
		loadSelectedObservingList(index);
	}

	ObsListCreateEditDialog::kill();
	createEditDialog_instance = Q_NULLPTR;
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
	if (v) {
		StelDialog::setVisible(true);
		this->loadDefaultList();
	}
	else
		StelDialog::setVisible(false);
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
		{SORTING_BY_LANDSCAPE,     COLUMN_NUMBER_LANDSCAPE}
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
