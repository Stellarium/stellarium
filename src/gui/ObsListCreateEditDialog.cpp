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

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QUuid>
#include <StelTranslator.hpp>
#include <utility>
#include <Planet.hpp>
#include <StelOBJ.hpp>
#include <QMessageBox>
#include <QRegularExpression>

#include "NebulaMgr.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "LandscapeMgr.hpp"
#include "StelUtils.hpp"

#include "ObsListCreateEditDialog.hpp"
#include "ui_obsListCreateEditDialog.h"

ObsListCreateEditDialog *ObsListCreateEditDialog::m_instance = nullptr;

ObsListCreateEditDialog::ObsListCreateEditDialog(std::string listUuid) : ui(new Ui_obsListCreateEditDialogForm()),
                                                                         obsListListModel(
                                                                                 new QStandardItemModel(0,
                                                                                                        ColumnCount)),
                                                                         core(StelApp::getInstance().getCore()),
                                                                         listOlud_(std::move(listUuid)), sorting("") {
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
 * Get instance of class
 */
auto ObsListCreateEditDialog::Instance(std::string listUuid) -> ObsListCreateEditDialog * {
    if (m_instance == nullptr) {
        m_instance = new ObsListCreateEditDialog(std::move(listUuid));
    }
    return m_instance;
}

/*
 * Initialize the dialog widgets and connect the signals/slots.
 */
void ObsListCreateEditDialog::createDialogContent() {
    ui->setupUi(dialog);

    // Signals and slots
    connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
    connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

    connect(ui->obsListAddObjectButton, &QPushButton::clicked, this,
            &ObsListCreateEditDialog::obsListAddObjectButtonPressed);
    connect(ui->obsListExitButton, &QPushButton::clicked, this, &ObsListCreateEditDialog::obsListExitButtonPressed);
    connect(ui->obsListSaveButton, &QPushButton::clicked, this, &ObsListCreateEditDialog::obsListSaveButtonPressed);
    connect(ui->obsListRemoveObjectButton, &QPushButton::clicked, this,
            &ObsListCreateEditDialog::obsListRemoveObjectButtonPressed);
    connect(ui->obsListImportListButton, &QPushButton::clicked, this,
            &ObsListCreateEditDialog::obsListImportListButtonPresssed);
    connect(ui->obsListExportListButton, &QPushButton::clicked, this,
            &ObsListCreateEditDialog::obsListExportListButtonPressed);
    connect(ui->nameOfListLineEdit, &QLineEdit::textChanged, this, &ObsListCreateEditDialog::nameOfListTextChange);

    // Initializing the list of observing list
    obsListListModel->setColumnCount(ColumnCount);
    setObservingListHeaderNames();

    ui->obsListCreationEditionTreeView->setModel(obsListListModel);
    ui->obsListCreationEditionTreeView->header()->setSectionsMovable(false);
    ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnName, QHeaderView::ResizeToContents);
    ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnNameI18n, QHeaderView::ResizeToContents);
    ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnType, QHeaderView::ResizeToContents);
    ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnRa, QHeaderView::ResizeToContents);
    ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnDec, QHeaderView::ResizeToContents);
    ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnConstellation,
                                                                       QHeaderView::ResizeToContents);
    ui->obsListCreationEditionTreeView->header()->setSectionResizeMode(ColumnMagnitude, QHeaderView::ResizeToContents);
    ui->obsListCreationEditionTreeView->header()->setStretchLastSection(true);
    ui->obsListCreationEditionTreeView->hideColumn(ColumnUUID);
    ui->obsListCreationEditionTreeView->hideColumn(ColumnDate);
    ui->obsListCreationEditionTreeView->hideColumn(ColumnLocation);
    // Enable the sort for columns
    ui->obsListCreationEditionTreeView->setSortingEnabled(true);

    QHeaderView *header = ui->obsListCreationEditionTreeView->header();
    connect(header, SIGNAL(sectionClicked(int)), this, SLOT(headerClicked(int)));

    // We hide the closeStelWindow to have only two possibilities to close the dialog:
    // Save and close and Exit
    ui->closeStelWindow->setHidden(true);

    initErrorMessage();
    ui->obsListLandscapeCheckBox->setChecked(false);

    // In case of creation the nameOfListLineEdit is empty and button save/close must be disabled
    // In case on edition the nameOfListLineEdit is not empty and the button save/close must be enable
    // delete with space -> no list with only white space as name
    ui->nameOfListLineEdit->setText(QString(ui->nameOfListLineEdit->text()).remove(QRegularExpression("([ ]+)$")));
    if (ui->nameOfListLineEdit->text().isEmpty()) {
        ui->obsListSaveButton->setEnabled(false);
    } else {
        ui->obsListSaveButton->setEnabled(true);
    }

    if (listOlud_.empty()) {
        // case of creation mode
        isCreationMode = true;
        ui->stelWindowTitle->setText("Observing list creation mode");
        ui->obsListImportListButton->setHidden(false);
    } else {
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
void ObsListCreateEditDialog::retranslate() {
    if (dialog != nullptr) {
        ui->retranslateUi(dialog);
    }
}

/*
 * Style changed
 */
void ObsListCreateEditDialog::styleChanged() {
    // Nothing for now
}

/*
 * Set the header for the observing list table
 * (obsListTreeVView)
 */
void ObsListCreateEditDialog::setObservingListHeaderNames() {
    const QStringList headerStrings = {
            "UUID", // Hidden column
            q_("Object designation"),
            q_("Object name"),
            q_("Type"),
            q_("Right ascension"),
            q_("Declination"),
            q_("Magnitude"),
            q_("Constellation"),
            q_("Date"),    // Hided column
            q_("Location") // Hided column
    };

    obsListListModel->setHorizontalHeaderLabels(headerStrings);
}

/*
 * Add row in the obsListListModel
 */
void ObsListCreateEditDialog::addModelRow(int number,
                                          const QString &olud,
                                          const QString &name,
                                          const QString &nameI18n,
                                          const QString &objtype,
                                          const QString &ra,
                                          const QString &dec,
                                          const QString &magnitude,
                                          const QString &constellation) {
    QStandardItem *item = Q_NULLPTR;

    // olud
    item = new QStandardItem(olud);
    item->setEditable(false);
    obsListListModel->setItem(number, ColumnUUID, item);

    // name
    item = new QStandardItem(name);
    item->setEditable(false);
    obsListListModel->setItem(number, ColumnName, item);

    // nameI18n
    item = new QStandardItem(nameI18n);
    item->setEditable(false);
    obsListListModel->setItem(number, ColumnNameI18n, item);

    // objtype (object type)
    item = new QStandardItem(objtype);
    item->setEditable(false);
    obsListListModel->setItem(number, ColumnType, item);

    // ra
    item = new QStandardItem(ra);
    item->setEditable(false);
    obsListListModel->setItem(number, ColumnRa, item);

    // dec
    item = new QStandardItem(dec);
    item->setEditable(false);
    obsListListModel->setItem(number, ColumnDec, item);

    // magnitude
    item = new QStandardItem(magnitude);
    item->setEditable(false);
    obsListListModel->setItem(number, ColumnMagnitude, item);

    // constellation
    item = new QStandardItem(constellation);
    item->setEditable(false);
    obsListListModel->setItem(number, ColumnConstellation, item);

    for (int i = 0; i < ColumnCount; ++i) {
        ui->obsListCreationEditionTreeView->resizeColumnToContents(i);
    }
}

/*
 * Slot for button obsListAddObjectButton.
 * Save selected object into the list of observed objects.
 */
void ObsListCreateEditDialog::obsListAddObjectButtonPressed() {
    initErrorMessage();
    const QList<StelObjectP> &selectedObject = objectMgr->getSelectedObject();

    if (!selectedObject.isEmpty()) {

        // No duplicate item in the same list
        bool is_already_in_list = false;
        QHash<QString, observingListItem>::iterator i;
        for (i = observingListItemCollection.begin(); i != observingListItemCollection.end(); i++) {
            if (i.value().name.compare(selectedObject[0]->getEnglishName()) == 0) {
                is_already_in_list = true;
                break;
            }
        }

        if (!is_already_in_list) {
            const int lastRow = obsListListModel->rowCount();
            const QString objectOlud = QUuid::createUuid().toString();

            // Object name (designation) and object name I18n
            QString objectName = selectedObject[0]->getEnglishName();
            QString objectNameI18n = selectedObject[0]->getNameI18n();

            if(objectNameI18n.isEmpty()){
                objectNameI18n = dash;
            }

            StelObjectP object = objectMgr->searchByNameI18n(objectNameI18n);
            if (selectedObject[0]->getType() == "Nebula" && objectName.isEmpty()) {
                objectName = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();
            }

            // Type
            QString type = selectedObject[0]->getType();

            // Object Type
            QString objectType = selectedObject[0]->getObjectType();

            // Ra & Dec
            float ra = 0.0;
            float dec = 0.0;
            QString objectRaStr = "";
            QString objectDecStr = "";
            bool visibleFlag = false;
            StelUtils::rectToSphe(&ra, &dec, selectedObject[0]->getJ2000EquatorialPos(core));

            if (ui->obsListCoordinatesCheckBox->isChecked() || objectType == CUSTOM_OBJECT || objectName.isEmpty()) {
                objectRaStr = StelUtils::radToHmsStr(ra, false).trimmed();
                objectDecStr = StelUtils::radToDmsStr(dec, false).trimmed();
            }

            // Fov
            double fov = -1.0;
            if (ui->obsListFovCheckBox->isChecked() || objectType == CUSTOM_OBJECT || objectName.isEmpty()) {
                fov = GETSTELMODULE(StelMovementMgr)->getCurrentFov();
            }

            // Visible flag
            if (objectName.contains("marker", Qt::CaseInsensitive)) {
                visibleFlag = true;
            }

            // Magnitude
            QString objectMagnitudeStr = util.getMagnitue(selectedObject, core);

            // Constallation
            QVariantMap objectMap = selectedObject[0]->getInfoMap(core);
            QVariant objectConstellationVariant = objectMap["iauConstellation"];
            QString objectConstellation("unknown");
            if (objectConstellationVariant.canConvert<QString>()) {
                objectConstellation = objectConstellationVariant.value<QString>();
            }

            // JD
            QString JDs = "";
            double JD = core->getJD();
            JDs = StelUtils::julianDayToISO8601String(JD + core->getUTCOffset(JD) / 24.).replace("T", " ");

            // Location
            QString Location;
            StelLocation loc = core->getCurrentLocation();
            if (loc.name.isEmpty()) {
                Location = "";
            } else {
                Location = QString("%1, %2").arg(loc.name, loc.region);
            }

            // Check if the object name is empty.
            if (objectName.isEmpty()) {
                objectName = q_("Unnamed object");
                if (objectNameI18n.isEmpty()) {
                    objectNameI18n = q_("Unnamed object");
                }
            }

            // Add objects in row model
            addModelRow(lastRow,
                        objectOlud,
                        objectName,
                        objectNameI18n,
                        objectType,
                        objectRaStr,
                        objectDecStr,
                        objectMagnitudeStr,
                        objectConstellation);

            observingListItem item;
            util.initItem(item);
            item.name = objectName;
            item.nameI18n = objectNameI18n;

            // Type
            if (!type.isEmpty()) {
                item.type = type;
            }
            // Object Type
            if (!objectType.isEmpty()) {
                item.objtype = objectType;
            }
            // Ra
            if (!objectRaStr.isEmpty()) {
                item.ra = objectRaStr;
            }
            // Dec
            if (!objectDecStr.isEmpty()) {
                item.dec = objectDecStr;
            }
            // Magnitude
            if (!objectMagnitudeStr.isEmpty()) {
                item.magnitude = objectMagnitudeStr;
            }
            // Constellation
            if (!objectConstellation.isEmpty()) {
                item.constellation = objectConstellation;
            }
            // JD
            if (!JDs.isEmpty()) {
                item.jd = JD;
            }
            // Location
            if (!Location.isEmpty()) {
                item.location = Location;
            }
            // Visible Flag
            if (!visibleFlag) {
                item.isVisibleMarker = visibleFlag;
            }
            // Fov
            if (fov > 0.0) {
                item.fov = fov;
            }
            observingListItemCollection.insert(objectOlud, item);
        }
    } else {
        qWarning() << "Selected object is empty !";
    }
}

/*
 * Slot for button obsListRemoveObjectButton
 */
void ObsListCreateEditDialog::obsListRemoveObjectButtonPressed() {
    initErrorMessage();
    int number = ui->obsListCreationEditionTreeView->currentIndex().row();
    QString uuid = obsListListModel->index(number, ColumnUUID).data().toString();
    obsListListModel->removeRow(number);
    observingListItemCollection.remove(uuid);
}

/*
 * Save observed objects and the list into Json file
 */
void ObsListCreateEditDialog::saveObservedObjectsInJsonFile() {

    if (observingListJsonPath.isEmpty()) {
        qWarning() << "[ObservingList Creation/Edition] Error saving observing list";
        return;
    }

    QFile jsonFile(observingListJsonPath);
    if (!jsonFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        qWarning() << "[ObservingList Creation/Edition] observing list can not be saved. A file can not be open for "
                      "reading and writing:"
                   << QDir::toNativeSeparators(observingListJsonPath);
        return;
    }

    // Name of the list
    const QString listName = ui->nameOfListLineEdit->text();

    // Creation date
    double JD = core->getJD();
    QString listCreationDate = StelUtils::julianDayToISO8601String(JD + core->getUTCOffset(JD) / 24.).replace("T", " ");

    try {
        QVariantMap mapFromJsonFile;
        QVariantMap allListsMap;
        const QString oludQs = QString::fromStdString(this->listOlud_);
        const QVariantMap currentList = allListsMap.value(oludQs).toMap();
        if (jsonFile.size() > 0) {
            mapFromJsonFile = StelJsonParser::parse(jsonFile.readAll()).toMap();
            allListsMap = mapFromJsonFile.value(QString(KEY_OBSERVING_LISTS)).toMap();
        }

        QVariantMap observingListDataList;

        // Name of the liste
        observingListDataList.insert(QString(KEY_NAME), listName);

        // Description of the list
        QString description = ui->descriptionLineEdit->text();
        observingListDataList.insert(QString(KEY_DESCRIPTION), description);

        // Creation date
        observingListDataList.insert(QString(KEY_CREATION_DATE), listCreationDate);

        // Landscape
        if (ui->obsListLandscapeCheckBox->isChecked()) {
            QString landscapeId = landscapeMgr->getCurrentLandscapeID();
            observingListDataList.insert(QString(KEY_LANDSCAPE_ID), landscapeId);
        } else {
            observingListDataList.insert(QString(KEY_LANDSCAPE_ID), "");
        }

        // List of objects
        QVariantList listOfObjects;
        QHashIterator<QString, observingListItem> i(observingListItemCollection);
        while (i.hasNext()) {
            i.next();
            observingListItem item = i.value();
            QVariantMap obl;

            // Designation (name)
            obl.insert(QString(KEY_DESIGNATION), item.name);

            // NameI18n
            obl.insert(QString(KEY_NAME_I18N), item.nameI18n);

            // Fov
            obl.insert(QString(KEY_FOV), item.fov);

            // Jd
            obl.insert(QString(KEY_JD), item.jd);

            // Location
            obl.insert(QString(KEY_LOCATION), item.location);

            // Ra
            obl.insert(QString(KEY_RA), item.ra);

            // Dec
            obl.insert(QString(KEY_DEC), item.dec);

            // Type
            obl.insert(QString(KEY_TYPE), item.type);

            // Object type
            obl.insert(QString(KEY_OBJECTS_TYPE), item.objtype);

            // Magnitude
            obl.insert(QString(KEY_MAGNITUDE), item.magnitude);

            // Constellation
            obl.insert(QString(KEY_CONSTELLATION), item.constellation);

            // Visible marker
            obl.insert(QString(KEY_IS_VISIBLE_MARKER), item.isVisibleMarker);

            listOfObjects.push_back(obl);
        }
        observingListDataList.insert(QString(KEY_OBJECTS), listOfObjects);

        // Sorting
        QString existingSorting;
        if (!isCreationMode) {
            existingSorting = currentList.value(QString(KEY_SORTING)).toString();
        }
        if (sorting.isEmpty()) {
            observingListDataList.insert(QString(KEY_SORTING), existingSorting);
        } else {
            observingListDataList.insert(QString(KEY_SORTING), sorting);
        }

        // Olud
        QString oblListOlud;
        if (isCreationMode || isSaveAs) {
            oblListOlud = QUuid::createUuid().toString();
        } else {
            oblListOlud = QString::fromStdString(listOlud_);
        }

        // Default list
        if (ui->obsListDefaultListCheckBox->isChecked()) {
            mapFromJsonFile.insert(KEY_DEFAULT_LIST_OLUD, oblListOlud);
        } else {
            QString defaultListUuid = mapFromJsonFile.value(KEY_DEFAULT_LIST_OLUD).toString();
            if (defaultListUuid.isEmpty()) {
                mapFromJsonFile.insert(KEY_DEFAULT_LIST_OLUD, "");
            } else {
                int compareResult = QString::compare(defaultListUuid, QString::fromStdString(listOlud_),
                                                     Qt::CaseSensitive);
                if (compareResult == 0) {
                    mapFromJsonFile.insert(KEY_DEFAULT_LIST_OLUD, "");
                }
            }
        }

        // Version
        mapFromJsonFile.insert(KEY_VERSION, FILE_VERSION);
        // Short name
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
void ObsListCreateEditDialog::obsListExportListButtonPressed() {
    initErrorMessage();
    QString originalobservingListJsonPath = observingListJsonPath;

    QString filter = "JSON (*.json)";
    observingListJsonPath = QFileDialog::getSaveFileName(Q_NULLPTR, q_("Export observing list as..."),
                                                         QDir::homePath() + "/" + JSON_FILE_NAME, filter);
    saveObservedObjectsInJsonFile();
    observingListJsonPath = originalobservingListJsonPath;
}

/*
 * Slot for button obsListImportListButton
 */
void ObsListCreateEditDialog::obsListImportListButtonPresssed() {

    QString filter = "JSON (*.json)";
    QString fileToImportJsonPath = QFileDialog::getOpenFileName(Q_NULLPTR, q_("Import observing list"),
                                                                QDir::homePath(),
                                                                filter);
    QVariantMap map;
    QFile jsonFile(fileToImportJsonPath);
    if (!jsonFile.open(QIODevice::ReadOnly)) {
        qWarning() << "[ObservingList Creation/Edition import] cannot open"
                   << QDir::toNativeSeparators(jsonFile.fileName());
    } else {

        try {
            initErrorMessage();
            map = StelJsonParser::parse(jsonFile.readAll()).toMap();
            jsonFile.close();

            if (map.contains(KEY_OBSERVING_LISTS)) { // Case of observingList import
                qDebug() << "ObservingList import";

                QVariantMap observingListMap = map.value(QString(KEY_OBSERVING_LISTS)).toMap();
                if (!observingListMap.isEmpty() && observingListMap.size() == 1) {
                    listOlud_ = observingListMap.keys().at(0).toStdString();
                } else {
                    qWarning()
                            << "[ObservingList Creation/Edition import] there is no list or more than one list.";
                    displayErrorMessage("Error: here is no list or more than one list.");
                    return;
                }

                QString originalobservingListJsonPath = observingListJsonPath;
                observingListJsonPath = fileToImportJsonPath;
                loadObservingList();
                observingListJsonPath = originalobservingListJsonPath;
            } else if (map.contains(KEY_BOOKMARKS)) { // Case of legacy bookmarks import
                qDebug() << "Legacy bookmarks import";
                QVariantMap bookmarksListMap = map.value(QString(KEY_BOOKMARKS)).toMap();
                if (!bookmarksListMap.isEmpty()) {
                    listOlud_ = bookmarksListMap.keys().at(0).toStdString();
                    QString originalobservingListJsonPath = observingListJsonPath;
                    observingListJsonPath = fileToImportJsonPath;
                    loadBookmarksInObservingList();
                    observingListJsonPath = originalobservingListJsonPath;
                } else {
                    qWarning()
                            << "[ObservingList Creation/Edition import] the file is empty or doesn't contains legacy bookmarks.";
                    displayErrorMessage("Error: the file is empty or doesn't contains legacy bookmarks.");
                    return;
                }
            }
        } catch (std::runtime_error &e) {
            qWarning() << "[ObservingList Creation/Edition] File format is wrong! Error: " << e.what();
            displayErrorMessage("File format is wrong!");
            return;
        }
    }
}

/*
 * Slot for button obsListSaveButton
 */
void ObsListCreateEditDialog::obsListSaveButtonPressed() {
    initErrorMessage();
    QString listName = ui->nameOfListLineEdit->text();

    //delete with space at the end of the name
    listName = QString(listName).remove(QRegularExpression("([ ]+)$"));

    bool isListAlreadyExists = !this->listNames_.isEmpty() && this->listNames_.contains(listName) &&
                               (isCreationMode || (listName.compare(currentListName) != 0 && !isCreationMode));

    if (isListAlreadyExists) {
        QString errorMessage;
        errorMessage.append("Error: a list with the name ")
                .append(ui->nameOfListLineEdit->text())
                .append(" already exists !");
        qWarning() << "[ObservingList Creation/Edition] Error: a list with the name " << ui->nameOfListLineEdit->text()
                   << " already exists !";
        std::string errorMessage_str = errorMessage.toStdString();
        displayErrorMessage(errorMessage_str.c_str());
    } else if (ui->nameOfListLineEdit->text().isEmpty()) {
        qWarning() << "[ObservingList Creation/Edition] Error: the list name is empty.";
        displayErrorMessage("Error: the list name is empty.");
    } else {
        isSaveAs = listName.compare(currentListName) != 0 && !isCreationMode;
        saveObservedObjectsInJsonFile();
        this->close();
        emit exitButtonClicked();
    }
}

/*
 * Slot for button obsListExitButton
 */
void ObsListCreateEditDialog::obsListExitButtonPressed() {
    ui->obsListErrorMessage->setHidden(true);
    ui->obsListErrorMessage->clear();
    this->close();
}

/*
 * Overload StelDialog::close()
 */
void ObsListCreateEditDialog::close() {
    this->setVisible(false);;
    emit this->exitButtonClicked();
}

/*
 * Slot for obsListCreationEditionTreeView header
 */
void ObsListCreateEditDialog::headerClicked(int index) {
    switch (index) {
        case ColumnName:
            sorting = QString(SORTING_BY_NAME);
            break;
        case ColumnNameI18n:
            sorting = QString(SORTING_BY_NAMEI18N);
            break;
        case ColumnType:
            sorting = QString(SORTING_BY_TYPE);
            break;
        case ColumnRa:
            sorting = QString(SORTING_BY_RA);
            break;
        case ColumnDec:
            sorting = QString(SORTING_BY_DEC);
            break;
        case ColumnMagnitude:
            sorting = QString(SORTING_BY_MAGNITUDE);
            break;
        case ColumnConstellation:
            sorting = QString(SORTING_BY_CONSTELLATION);;
            break;
        default:
            sorting = "";
            break;
    }
    qDebug() << "Sorting = " << sorting;
}

/*
 * Load the observing list in case of edit mode
 */
void ObsListCreateEditDialog::loadObservingList() {

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
            if (defaultListOlud.toStdString() == listOlud_) {
                ui->obsListDefaultListCheckBox->setChecked(true);
            }

            observingListItemCollection.clear();
            const QString keyOlud = QString::fromStdString(listOlud_);
            QVariantMap observingListMap = map.value(QString(KEY_OBSERVING_LISTS)).toMap().value(keyOlud).toMap();
            QVariantList listOfObjects;

            auto listName = observingListMap.value(QString(KEY_NAME)).value<QString>();

            // List name
            currentListName = listName;
            ui->nameOfListLineEdit->setText(listName);

            // List description
            auto listDescription = observingListMap.value(QString(KEY_DESCRIPTION)).value<QString>();
            ui->descriptionLineEdit->setText(listDescription);

            // Landscape
            auto landscape = observingListMap.value(QString(KEY_LANDSCAPE_ID)).value<QString>();
            if (!landscape.isEmpty()) {
                ui->obsListLandscapeCheckBox->setChecked(true);
            }

            if (observingListMap.value(QString(KEY_OBJECTS)).canConvert<QVariantList>()) {
                QVariant data = observingListMap.value(QString(KEY_OBJECTS));
                listOfObjects = data.value<QVariantList>();
            } else {
                qCritical() << "[ObservingList Creation/Edition] conversion error";
                return;
            }

            for (const QVariant &object: listOfObjects) {
                QVariantMap objectMap;
                if (object.canConvert<QVariantMap>()) {
                    objectMap = object.value<QVariantMap>();
                    int lastRow = obsListListModel->rowCount();
                    QString objectOlud = QUuid::createUuid().toString();

                    // Name
                    auto objectName = objectMap.value(QString(KEY_DESIGNATION)).value<QString>();

                    // NameI18n
                    auto objectNameI18n = objectMap.value(QString(KEY_NAME_I18N)).value<QString>();

                    // Fov
                    auto fov = objectMap.value(QString(KEY_FOV)).value<double>();

                    // Object type
                    auto type = objectMap.value(QString(KEY_TYPE)).value<QString>();

                    // Object type
                    auto objectType = objectMap.value(QString(KEY_OBJECTS_TYPE)).value<QString>();

                    // Ra & dec
                    auto objectRaStr = objectMap.value(QString(KEY_RA)).value<QString>();
                    auto objectDecStr = objectMap.value(QString(KEY_DEC)).value<QString>();

                    // Magnitude
                    auto objectMagnitudeStr = objectMap.value(QString(KEY_MAGNITUDE)).value<QString>();

                    // Constellation
                    auto objectConstellation = objectMap.value(QString(KEY_CONSTELLATION)).value<QString>();

                    // Julian Day
                    auto JDs = objectMap.value(QString(KEY_JD)).value<QString>();

                    // Location
                    auto location = objectMap.value(QString(KEY_LOCATION)).value<QString>();

                    // Visible flag
                    bool visibleFlag = objectMap.value(QString(KEY_IS_VISIBLE_MARKER)).value<bool>();



                    // Add data into model row
                    addModelRow(lastRow,
                                objectOlud,
                                objectName,
                                objectNameI18n,
                                objectType,
                                objectRaStr,
                                objectDecStr,
                                objectMagnitudeStr,
                                objectConstellation);


                    observingListItem item;
                    util.initItem(item);
                    item.name = objectName;
                    item.nameI18n = objectNameI18n;

                    // Type
                    if (!type.isEmpty()) {
                        item.type = type;
                    }

                    // Object type
                    if (!objectType.isEmpty()) {
                        item.objtype = objectType;
                    }

                    // Ra
                    if (!objectRaStr.isEmpty()) {
                        item.ra = objectRaStr;
                    }

                    // Dec
                    if (!objectDecStr.isEmpty()) {
                        item.dec = objectDecStr;
                    }

                    // Magnitude
                    if (!objectMagnitudeStr.isEmpty()) {
                        item.magnitude = objectMagnitudeStr;
                    }

                    // Constellation
                    if (!objectConstellation.isEmpty()) {
                        item.constellation = objectConstellation;
                    }

                    // JD
                    if (!JDs.isEmpty()) {
                        item.jd = JDs.toDouble();
                    }

                    // Location
                    if (!location.isEmpty()) {
                        item.location = location;
                    }

                    // Flag
                    if (!visibleFlag) {
                        item.isVisibleMarker = visibleFlag;
                    }

                    // Fov
                    if (fov > 0.0) {
                        item.fov = fov;
                    }
                    observingListItemCollection.insert(objectOlud, item);

                } else {
                    qCritical() << "[ObservingList Creation/Edition] conversion error";
                    return;
                }
            }

            objectMgr->unSelect();

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
    QVariantMap map;

    QFile jsonFile(observingListJsonPath);
    if (!jsonFile.open(QIODevice::ReadOnly)) {
        qWarning() << "[ObservingList] cannot open" << QDir::toNativeSeparators(observingListJsonPath);
    } else {

        try {

            map = StelJsonParser::parse(jsonFile.readAll()).toMap();
            jsonFile.close();
            QVariantMap bookmarksMap = map.value(KEY_BOOKMARKS).toMap();
            observingListItemCollection.clear();

            for (const auto &bookmarkKey: bookmarksMap.keys()) {

                QVariantMap bookmarkData = bookmarksMap.value(bookmarkKey).toMap();
                observingListItem item;
                util.initItem(item);
                QString objectUuid = QUuid::createUuid().toString();
                int lastRow = obsListListModel->rowCount();

                // Name
                QString objectName = bookmarkData.value(KEY_NAME).toString();
                item.name = objectName;

                // We need to select the object to add additional information that is not in the Bookmark file
                if (objectMgr->findAndSelect(item.name) && !objectMgr->getSelectedObject().isEmpty()) {
                    const QList<StelObjectP> &selectedObject = objectMgr->getSelectedObject();

                    // Ra & Dec - ra and dec are not empty in case of Custom Object
                    QString raStr = bookmarkData.value(KEY_RA).toString();
                    QString decStr = bookmarkData.value(KEY_DEC).toString();
                    if (raStr.isEmpty() || decStr.isEmpty()) {
                        float ra = 0.0;
                        float dec = 0.0;
                        StelUtils::rectToSphe(&ra, &dec, selectedObject[0]->getJ2000EquatorialPos(core));
                        raStr = StelUtils::radToHmsStr(ra, false).trimmed();
                        decStr = StelUtils::radToDmsStr(dec, false).trimmed();
                    }

                    if (!raStr.isEmpty()) {
                        item.ra = raStr;
                    } else {
                        item.ra = "";
                    }

                    if (!decStr.isEmpty()) {
                        item.dec = decStr;
                    } else {
                        item.dec = "";
                    }

                    // NameI18n
                    QString nameI18n = bookmarkData.value(KEY_NAME_I18N).toString();
                    if (!nameI18n.isEmpty()) {
                        item.nameI18n = nameI18n;
                    } else {
                        item.nameI18n = "";
                    }

                    // JDs
                    QString JDs = bookmarkData.value(KEY_JD).toString();
                    if (!JDs.isEmpty()) {
                        item.jd = JDs.toDouble();
                    } else {
                        item.jd = 0.0;
                    }

                    // Location
                    QString location = bookmarkData.value(KEY_LOCATION).toString();
                    if (!location.isEmpty()) {
                        item.location = location;
                    } else {
                        item.location = "";
                    }

                    // Constallation
                    QVariantMap objectMap = selectedObject[0]->getInfoMap(core);
                    QVariant objectConstellationVariant = objectMap["iauConstellation"];
                    QString objectConstellation("unknown");
                    if (objectConstellationVariant.canConvert<QString>()) {
                        objectConstellation = objectConstellationVariant.value<QString>();
                    }
                    if (!objectConstellation.isEmpty()) {
                        item.constellation = objectConstellation;
                    }

                    // Type
                    QString type = selectedObject[0]->getType();
                    if (!type.isEmpty()) {
                        item.type = type;
                    }

                    // Object Type
                    QString objectType = selectedObject[0]->getObjectType();
                    if (!objectType.isEmpty()) {
                        item.objtype = objectType;
                    }

                    // Magnitude
                    QString objectMagnitudeStr = util.getMagnitue(selectedObject, core);
                    if (!objectMagnitudeStr.isEmpty()) {
                        item.magnitude = objectMagnitudeStr;
                    }

                    // Fov
                    double fov = bookmarkData.value(KEY_FOV).toDouble();
                    if (fov > 0.0) {
                        item.fov = fov;
                    }

                    // Visible marker
                    item.isVisibleMarker = bookmarkData.value(KEY_IS_VISIBLE_MARKER, false).toBool();

                    // Add data into model row
                    addModelRow(lastRow,
                                objectUuid,
                                objectName,
                                nameI18n,
                                objectType,
                                raStr,
                                decStr,
                                objectMagnitudeStr,
                                objectConstellation);

                    observingListItemCollection.insert(objectUuid, item);
                }
            }
            objectMgr->unSelect();
        } catch (std::runtime_error &e) {
            qWarning() << "[ObservingList] Load bookmarks in observing list: File format is wrong! Error: " << e.what();
            return;
        }
    }
}

/*
 * Called when the text of the nameOfListLineEdit change
 */
void ObsListCreateEditDialog::nameOfListTextChange() {
    ui->obsListErrorMessage->setHidden(true);
    //delete with space -> no list with only white space as name
    QString listeName = QString(ui->nameOfListLineEdit->text()).remove(QRegularExpression("([ ]+)$"));
    if (listeName.isEmpty()) {
        ui->obsListSaveButton->setEnabled(false);
    } else {
        ui->obsListSaveButton->setEnabled(true);
    }
}

/*
 * Setter for listName
 */
void ObsListCreateEditDialog::setListName(QList<QString> listName) {
    this->listNames_ = std::move(listName);
}

/*
 * Initialize error mssage
 */
void ObsListCreateEditDialog::initErrorMessage() {
    ui->obsListErrorMessage->setHidden(true);
    ui->obsListErrorMessage->clear();
}

/*
 * Display error message
 */
void ObsListCreateEditDialog::displayErrorMessage(const char *message) {
    QString errorMessage;
    errorMessage.append(q_(message));
    ui->obsListErrorMessage->setHidden(false);
    ui->obsListErrorMessage->setText(errorMessage);
}

/*
 * Destructor of singleton
 */
void ObsListCreateEditDialog::kill() {
    if (m_instance != nullptr) {
        delete m_instance;
        m_instance = nullptr;
    }
}
