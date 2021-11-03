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
#include "StelFileMgr.hpp"
#include "StelMovementMgr.hpp"
#include "CustomObjectMgr.hpp"
#include "HighlightMgr.hpp"
#include "StarMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelUtils.hpp"
#include "ObsListDialog.hpp"
#include "LabelMgr.hpp"

#include "ui_obsListDialog.h"
#include <QSortFilterProxyModel>

using namespace std;


ObsListDialog::ObsListDialog ( QObject* parent ) : StelDialog ( "Observing list", parent )
{
    ui = new Ui_obsListDialogForm();
    core = StelApp::getInstance().getCore();
    objectMgr = GETSTELMODULE ( StelObjectMgr );
    labelMgr = GETSTELMODULE ( LabelMgr );
    obsListListModel = new QStandardItemModel ( 0,ColumnCount );
    observingListJsonPath = StelFileMgr::findFile ( "data", ( StelFileMgr::Flags ) ( StelFileMgr::Directory|StelFileMgr::Writable ) ) + "/" + QString ( JSON_FILE_NAME );
    bookmarksJsonPath = StelFileMgr::findFile ( "data", ( StelFileMgr::Flags ) ( StelFileMgr::Directory|StelFileMgr::Writable ) ) + "/" + QString ( JSON_BOOKMARKS_FILE_NAME );
    createEditDialog_instance = Q_NULLPTR;
    defaultListUuid_ = "";
}

ObsListDialog::~ObsListDialog()
{
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
    ui->setupUi ( dialog );

    //Signals and slots
    connect ( &StelApp::getInstance(), SIGNAL ( languageChanged() ), this, SLOT ( retranslate() ) );
    connect ( ui->closeStelWindow, SIGNAL ( clicked() ), this, SLOT ( close() ) );

    connect ( ui->obsListNewListButton, SIGNAL ( clicked() ), this, SLOT ( obsListNewListButtonPressed() ) );
    connect ( ui->obsListEditListButton, SIGNAL ( clicked() ), this, SLOT ( obsListEditButtonPressed() ) );
    connect ( ui->obsListClearHighlightButton, SIGNAL ( clicked() ), this, SLOT ( obsListClearHighLightButtonPressed() ) );
    connect ( ui->obsListHighlightAllButton, SIGNAL ( clicked() ), this, SLOT ( obsListHighLightAllButtonPressed() ) );
    connect ( ui->obsListExitButton, SIGNAL ( clicked() ), this, SLOT ( obsListExitButtonPressed() ) );
    connect ( ui->obsListDeleteButton, SIGNAL ( clicked() ), this, SLOT ( obsListDeleteButtonPressed() ) );
    connect ( ui->obsListTreeView, SIGNAL ( doubleClicked ( QModelIndex ) ), this, SLOT ( selectAndGoToObject ( QModelIndex ) ) );

    //obsListCombo settings
    connect ( ui->obsListComboBox, SIGNAL ( activated ( int ) ), this, SLOT ( loadSelectedObservingList ( int ) ) );

    //Initialize the list of observing lists
    obsListListModel->setColumnCount ( ColumnCount );
    setObservingListHeaderNames();

    ui->obsListTreeView->setModel ( obsListListModel );
    ui->obsListTreeView->header()->setSectionsMovable ( false );
    ui->obsListTreeView->header()->setSectionResizeMode ( ColumnName, QHeaderView::ResizeToContents );
    ui->obsListTreeView->header()->setSectionResizeMode ( ColumnType, QHeaderView::ResizeToContents );
    ui->obsListTreeView->header()->setSectionResizeMode ( ColumnRa, QHeaderView::ResizeToContents );
    ui->obsListTreeView->header()->setSectionResizeMode ( ColumnDec, QHeaderView::ResizeToContents );
    ui->obsListTreeView->header()->setSectionResizeMode ( ColumnConstellation, QHeaderView::ResizeToContents );
    ui->obsListTreeView->header()->setSectionResizeMode ( ColumnMagnitude, QHeaderView::ResizeToContents );
    ui->obsListTreeView->header()->setStretchLastSection ( true );
    ui->obsListTreeView->hideColumn ( ColumnUUID );
    ui->obsListTreeView->hideColumn ( ColumnNameI18n );
    ui->obsListTreeView->hideColumn ( ColumnJD );
    ui->obsListTreeView->hideColumn ( ColumnLocation );
    //Enable the sort for columns
    ui->obsListTreeView->setSortingEnabled ( true );

    //By default buttons are disable
    ui->obsListEditListButton->setEnabled ( false );
    ui->obsListHighlightAllButton->setEnabled ( false );
    ui->obsListClearHighlightButton->setEnabled ( false );
    ui->obsListDeleteButton->setEnabled ( false );


    // For no regression we must take into account the legacy bookmarks file
    QFile jsonBookmarksFile ( bookmarksJsonPath );
    if ( jsonBookmarksFile.exists() ) {
        qDebug() << "Fichiers bookmarks détecté";
        loadBookmarksInObservingList();
    }

    QFile jsonFile ( observingListJsonPath );
    if ( jsonFile.exists() ) {
        loadListsName();
        loadDefaultList();
    }

}

/*
 * Retranslate dialog
*/
void ObsListDialog::retranslate()
{
    if ( dialog ) {
        ui->retranslateUi ( dialog );
        setObservingListHeaderNames();
    }
}

/*
 * Style changed
*/
void ObsListDialog::styleChanged()
{
    // Nothing for now
}

/*
 * Set the header for the observing list table
 * (obsListTreeVView)
*/
void ObsListDialog::setObservingListHeaderNames()
{
    const QStringList headerStrings = {
        "UUID", // Hidden column
        q_ ( "Object name" ),
        q_ ( "Object name I18N" ), // Hidden column
        q_ ( "Type" ),
        q_ ( "Right ascension" ),
        q_ ( "Declination" ),
        q_ ( "Magnitude" ),
        q_ ( "Constellation" ),
        q_ ( "Date" ), // Hidden column
        q_ ( "Location" ) // Hidden column
    };

    obsListListModel->setHorizontalHeaderLabels ( headerStrings );
}

/*
 * Add row in the obsListListModel
*/
void ObsListDialog::addModelRow ( int number, QString uuid, QString name, QString nameI18n, QString type, QString ra, QString dec, QString magnitude, QString constellation )
{
    QStandardItem* item = Q_NULLPTR;

    item = new QStandardItem ( uuid );
    item->setEditable ( false );
    obsListListModel->setItem ( number, ColumnUUID, item );

    item = new QStandardItem ( name );
    item->setEditable ( false );
    obsListListModel->setItem ( number, ColumnName, item );

    item = new QStandardItem ( nameI18n );
    item->setEditable ( false );
    obsListListModel->setItem ( number, ColumnNameI18n, item );

    item = new QStandardItem ( type );
    item->setEditable ( false );
    obsListListModel->setItem ( number, ColumnType, item );

    item = new QStandardItem ( ra );
    item->setEditable ( false );
    obsListListModel->setItem ( number, ColumnRa, item );

    item = new QStandardItem ( dec );
    item->setEditable ( false );
    obsListListModel->setItem ( number, ColumnDec, item );

    item = new QStandardItem ( magnitude );
    item->setEditable ( false );
    obsListListModel->setItem ( number, ColumnMagnitude, item );

    item = new QStandardItem ( constellation );
    item->setEditable ( false );
    obsListListModel->setItem ( number, ColumnConstellation, item );

    for ( int i = 0; i < ColumnCount; ++i ) {
        ui->obsListTreeView->resizeColumnToContents ( i );
    }
}

/*
 * Slot for button obsListHighLightAllButton
*/
void ObsListDialog::obsListHighLightAllButtonPressed()
{
    QList<Vec3d> highlights;
    highlights.clear();
    obsListClearHighLightButtonPressed(); // Enable fool protection
    int fontSize = StelApp::getInstance().getScreenFontSize();
    HighlightMgr* hlMgr = GETSTELMODULE ( HighlightMgr );
    QString color = hlMgr->getColor().toHtmlColor();
    float distance = hlMgr->getMarkersSize();

    for ( auto item : observingListItemCollection ) {
        QString name	= item.name;
        QString raStr	= item.ra.trimmed();
        QString decStr	= item.dec.trimmed();

        Vec3d pos;
        bool status = false;
        if ( !raStr.isEmpty() && !decStr.isEmpty() ) {
            StelUtils::spheToRect ( StelUtils::getDecAngle ( raStr ), StelUtils::getDecAngle ( decStr ), pos );
            status = true;
        } else {
            status = objectMgr->findAndSelect ( name );
            const QList<StelObjectP>& selected = objectMgr->getSelectedObject();
            if ( !selected.isEmpty() ) {
                pos = selected[0]->getJ2000EquatorialPos ( core );
            }
        }

        if ( status ) {
            highlights.append ( pos );
        }

        objectMgr->unSelect();
        // Add labels for named highlights (name in top right corner)
        highlightLabelIDs.append ( labelMgr->labelObject ( name, name, true, fontSize, color, "NE", distance ) );
    }

    hlMgr->fillHighlightList ( highlights );
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
    objectMgr->unSelect();
    GETSTELMODULE ( HighlightMgr )->cleanHighlightList();
    // Clear labels
    for ( auto l : highlightLabelIDs ) {
        labelMgr->deleteLabel ( l );
    }
    highlightLabelIDs.clear();
}

/*
 * Slot for button obsListNewListButton
*/
void ObsListDialog::obsListNewListButtonPressed()
{
    string listUuid = string();
    invokeObsListCreateEditDialog ( listUuid );
}

/*
 * Slot for button obsListEditButton
*/
void ObsListDialog::obsListEditButtonPressed()
{
    if ( !selectedObservingListUuid.empty() ) {
        invokeObsListCreateEditDialog ( selectedObservingListUuid );
    } else {
        qWarning() << "The selected observing list uuid is empty";
    }
}

/**
 * Open the observing list create/edit dialog
*/
void ObsListDialog::invokeObsListCreateEditDialog ( string listUuid )
{
    createEditDialog_instance = ObsListCreateEditDialog::Instance ( listUuid );
    connect ( createEditDialog_instance, SIGNAL ( exitButtonClicked() ), this, SLOT ( obsListCreateEditDialogClosed() ) );
    createEditDialog_instance->setListName ( listName_ );
    createEditDialog_instance->setVisible ( true );
}

/*
 * Load the lists names for populate the combo box and get the default list uuid
*/
void ObsListDialog::loadListsName()
{
    QVariantMap map;
    QFile jsonFile ( observingListJsonPath );
    if ( !jsonFile.open ( QIODevice::ReadOnly ) ) {
        qWarning() << "[ObservingList] cannot open" << QDir::toNativeSeparators ( JSON_FILE_NAME );

    } else {
        try {
            map = StelJsonParser::parse ( jsonFile.readAll() ).toMap();
            jsonFile.close();

            // init combo box
            ui->obsListComboBox->clear();

            // Get the default list uuid
            QString defaultListUuid = map.value ( KEY_DEFAULT_LIST_UUID ).toString();

            QVariantMap observingListsMap = map.value ( QString ( KEY_OBSERVING_LISTS ) ).toMap();
            populateListNameInComboBox ( observingListsMap );
            populateDataInComboBox ( observingListsMap, defaultListUuid );

        } catch ( std::runtime_error &e ) {
            qWarning() << "[ObservingList] File format is wrong! Error: " << e.what();
            return;
        }
    }
}


/*
 * Populate the list names into combo box
*/
void ObsListDialog::populateListNameInComboBox ( QVariantMap map )
{
    QMap<QString, QVariant>::iterator i;
    ui->obsListComboBox->clear();
    listNamesModel.clear();
    listName_.clear();
    for ( i = map.begin(); i != map.end(); ++i ) {
        if ( i.value().canConvert<QVariantMap>() ) {
            QVariant var = i.value();
            QVariantMap data = var.value<QVariantMap>();
            QString listName = data.value ( KEY_NAME ).value<QString>();
            listNamesModel.append ( listName );
            listName_.append(listName);
        }
    }
    listNamesModel.sort ( Qt::CaseInsensitive );
    ui->obsListComboBox->addItems ( listNamesModel );
}

/*
 * Populate data into combo box
*/
void ObsListDialog::populateDataInComboBox ( QVariantMap map, QString defaultListUuid )
{
    QMap<QString, QVariant>::iterator i;
    for ( i = map.begin(); i != map.end(); ++i ) {
        QString listUuid = i.key();
        if ( i.value().canConvert<QVariantMap>() ) {
            QVariant var = i.value();
            QVariantMap data = var.value<QVariantMap>();
            QString listName = data.value ( KEY_NAME ).value<QString>();

            foreach ( QString str, listNamesModel ) {
                if ( QString::compare ( str, listName ) == 0 ) {
                    int index = listNamesModel.indexOf ( listName, 0 );
                    ui->obsListComboBox->setItemData ( index,listUuid );
                    break;
                }
            }
        }
        if ( defaultListUuid == listUuid ) {
            defaultListUuid_ = defaultListUuid;
        }
    }
}



/*
 * Load the default list
*/
void ObsListDialog::loadDefaultList()
{
    if ( defaultListUuid_ != "" ) {
        int index = ui->obsListComboBox->findData ( defaultListUuid_ );
        if ( index != -1 ) {
            ui->obsListComboBox->setCurrentIndex ( index );
            ui->obsListEditListButton->setEnabled ( true );
            selectedObservingListUuid = defaultListUuid_.toStdString();
            loadObservingList ( defaultListUuid_ );
        }
    } else {
        ui->obsListComboBox->setCurrentIndex ( 0 );
        loadSelectedObservingList ( 0 );
    }
}

/*
 * Load selected observing list
*/
void ObsListDialog::loadObservingList ( QString listUuid )
{
    QVariantMap map;
    QVariantList listOfObjects;
    QFile jsonFile ( observingListJsonPath );
    if ( !jsonFile.open ( QIODevice::ReadOnly ) ) {
        qWarning() << "[ObservingList] cannot open" << QDir::toNativeSeparators ( JSON_FILE_NAME );
    } else {
        try {

            map = StelJsonParser::parse ( jsonFile.readAll() ).toMap();
            QVariantMap observingListMap = map.value ( QString ( KEY_OBSERVING_LISTS ) ).toMap().value ( listUuid ).toMap();
            QString sortingBy = observingListMap.value ( KEY_SORTING ).toString();

            listOfObjects = loadListFromJson ( map, listUuid );

            if ( listOfObjects.size() > 0 ) {
                ui->obsListHighlightAllButton->setEnabled ( true );
                ui->obsListClearHighlightButton->setEnabled ( true );
                ui->obsListDeleteButton->setEnabled ( true );

                for ( QVariant object: listOfObjects ) {
                    QVariantMap objectMap;
                    if ( object.canConvert<QVariantMap>() ) {
                        objectMap = object.value<QVariantMap>();
                        QString objectName = objectMap.value ( QString ( KEY_DESIGNATION ) ).value<QString>();

                        if ( objectMgr->findAndSelect ( objectName ) ) {
                            const QList<StelObjectP>& selectedObject = objectMgr->getSelectedObject();
                            if ( !selectedObject.isEmpty() ) {

                                int lastRow = obsListListModel->rowCount();
                                QString objectUuid = QUuid::createUuid().toString();
                                QString objectNameI18n = selectedObject[0]->getNameI18n();
                                QString objectRaStr = "", objectDecStr = "";
                                bool visibleFlag = false;
                                double fov = -1.0;

                                QString objectType = selectedObject[0]->getType();

                                float ra, dec;
                                StelUtils::rectToSphe ( &ra, &dec, selectedObject[0]->getJ2000EquatorialPos ( core ) );
                                objectRaStr = StelUtils::radToHmsStr ( ra, false ).trimmed();
                                objectDecStr = StelUtils::radToDmsStr ( dec, false ).trimmed();
                                if ( objectName.contains ( "marker", Qt::CaseInsensitive ) ) {
                                    visibleFlag = true;
                                }

                                float objectMagnitude = selectedObject[0]->getVMagnitude ( core );
                                QString objectMagnitudeStr = QString::number ( objectMagnitude );

                                QVariantMap objectMap = selectedObject[0]->getInfoMap ( core );
                                QVariant objectConstellationVariant = objectMap["iauConstellation"];
                                QString objectConstellation ( "unknown" );
                                if ( objectConstellationVariant.canConvert<QString>() ) {
                                    objectConstellation = objectConstellationVariant.value<QString>();
                                }

                                QString JDs = "";
                                double JD = core->getJD();

                                JDs = StelUtils::julianDayToISO8601String ( JD + core->getUTCOffset ( JD ) /24. ).replace ( "T", " " );

                                QString Location = "";
                                StelLocation loc = core->getCurrentLocation();
                                if ( loc.name.isEmpty() ) {
                                    Location = QString ( "%1, %2" ).arg ( loc.latitude ).arg ( loc.longitude );
                                } else {
                                    Location = QString ( "%1, %2" ).arg ( loc.name ).arg ( loc.region );
                                }

                                addModelRow ( lastRow,objectUuid,objectName, objectNameI18n, objectType, objectRaStr, objectDecStr, objectMagnitudeStr, objectConstellation );

                                observingListItem item;
                                item.name = objectName;
                                item.nameI18n = objectNameI18n;
                                if ( !objectType.isEmpty() ) {
                                    item.type = objectType;
                                }
                                if ( !objectRaStr.isEmpty() ) {
                                    item.ra = objectRaStr;
                                }
                                if ( !objectDecStr.isEmpty() ) {
                                    item.dec = objectDecStr;
                                }
                                if ( !objectMagnitudeStr.isEmpty() ) {
                                    item.magnitude = objectMagnitudeStr;
                                }
                                if ( !objectConstellation.isEmpty() ) {
                                    item.constellation = objectConstellation;
                                }
                                if ( !JDs.isEmpty() ) {
                                    item.jd	= QString::number ( JD, 'f', 6 );
                                }
                                if ( !Location.isEmpty() ) {
                                    item.location = Location;
                                }
                                if ( !visibleFlag ) {
                                    item.isVisibleMarker = visibleFlag;
                                }
                                if ( fov > 0.0 ) {
                                    item.fov = fov;
                                }

                                observingListItemCollection.insert ( objectUuid,item );

                            } else {
                                qWarning() << "[ObservingList] selected object is empty !";
                            }

                        } else {
                            qWarning() << "[ObservingList] object: " << objectName << " not found !" ;
                        }
                    } else {
                        qCritical() << "[ObservingList] conversion error";
                        return;
                    }
                }
            } else {
                ui->obsListHighlightAllButton->setEnabled ( false );
                ui->obsListClearHighlightButton->setEnabled ( false );
                ui->obsListDeleteButton->setEnabled ( false );
            }

            objectMgr->unSelect();
            if ( !sortingBy.isEmpty() ) {
                sortObsListTreeViewByColumnName ( sortingBy );
            }
            jsonFile.close();
        } catch ( std::runtime_error &e ) {
            qWarning() << "[ObservingList] Load selected observing list: File format is wrong! Error: " << e.what();
            jsonFile.close();
            return;
        }
    }
}

/*
 * Load the list from JSON file QVariantMap map
*/
QVariantList ObsListDialog::loadListFromJson ( QVariantMap map, QString listUuid )
{

    observingListItemCollection.clear();
    QVariantMap observingListMap = map.value ( QString ( KEY_OBSERVING_LISTS ) ).toMap().value ( listUuid ).toMap();
    QVariantList listOfObjects;

    QString listDescription = observingListMap.value ( QString ( KEY_DESCRIPTION ) ).value<QString>();
    ui->obsListDescriptionTextEdit->setPlainText ( listDescription );

    // Displaying the creation date
    QString JDs = observingListMap.value ( QString ( KEY_JD ) ).value<QString>();
    double JD = JDs.toDouble();
    QString listCreationDate = JDs = StelUtils::julianDayToISO8601String ( JD + core->getUTCOffset ( JD ) /24. ).replace ( "T", " " );
    ui->obsListCreationDateLineEdit->setText ( listCreationDate );

    if ( observingListMap.value ( QString ( KEY_OBJECTS ) ).canConvert<QVariantList>() ) {
        QVariant data = observingListMap.value ( QString ( KEY_OBJECTS ) );
        listOfObjects = data.value<QVariantList>();
    } else {
        qCritical() << "[ObservingList] conversion error";
    }

    // Clear model
    obsListListModel->removeRows ( 0,obsListListModel->rowCount() );
    return listOfObjects;
}


/*
 * Load the bookmarks of bookmarks.json file into observing lists file
 * For no regression with must take into account the legacy bookmarks.json file
*/
void ObsListDialog::loadBookmarksInObservingList()
{
    QHash<QString, bookmark> bookmarksCollection;
    QVariantMap map;

    QFile jsonFile ( bookmarksJsonPath );
    if ( !jsonFile.open ( QIODevice::ReadOnly ) ) {
        qWarning() << "[ObservingList] cannot open" << QDir::toNativeSeparators ( bookmarksJsonPath );
    } else {

        try {

            map = StelJsonParser::parse ( jsonFile.readAll() ).toMap();
            jsonFile.close();
            QVariantMap bookmarksMap = map.value ( KEY_BOOKMARKS ).toMap();

            for ( auto bookmarkKey : bookmarksMap.keys() ) {

                QVariantMap bookmarkData = bookmarksMap.value ( bookmarkKey ).toMap();
                bookmark item;

                QString JDs = "";

                item.name = bookmarkData.value ( KEY_NAME ).toString();
                QString nameI18n = bookmarkData.value ( KEY_NAME_I18N ).toString();
                if ( !nameI18n.isEmpty() ) {
                    item.nameI18n = nameI18n;
                }

                QString JD = bookmarkData.value ( KEY_JD ).toString();
                if ( !JD.isEmpty() ) {
                    item.jd = JD;
                    JDs = StelUtils::julianDayToISO8601String ( JD.toDouble() + core->getUTCOffset ( JD.toDouble() ) /24. ).replace ( "T", " " );
                }

                QString Location = bookmarkData.value ( KEY_LOCATION ).toString();
                if ( !Location.isEmpty() ) {
                    item.location = Location;
                }

                QString RA = bookmarkData.value ( KEY_RA ).toString();
                if ( !RA.isEmpty() ) {
                    item.ra = RA;
                }

                QString Dec = bookmarkData.value ( KEY_DEC ).toString();
                if ( !Dec.isEmpty() ) {
                    item.dec = Dec;
                }

                item.isVisibleMarker = bookmarkData.value ( KEY_IS_VISIBLE_MARKER, false ).toBool();
                double fov = bookmarkData.value ( KEY_FOV ).toDouble();
                if ( fov > 0.0 ) {
                    item.fov = fov;
                }

                bookmarksCollection.insert ( bookmarkKey, item );

            }

            saveBookmarks ( bookmarksCollection );

        } catch ( std::runtime_error &e ) {
            qWarning() << "[ObservingList] Load bookmarks in observing list: File format is wrong! Error: " << e.what();
            return;
        }
    }
}


/*
 * Save the bookmarks into observing list file
*/
void ObsListDialog::saveBookmarks ( QHash<QString, bookmark> bookmarksCollection )
{
    QFile jsonFile ( observingListJsonPath );
    if ( !jsonFile.open ( QIODevice::ReadWrite|QIODevice::Text ) ) {
        qWarning() << "[ObservingList] bookmarks list can not be saved. A file can not be open for reading and writing:"
                   << QDir::toNativeSeparators ( observingListJsonPath );
        return;
    }

    try {

        QVariantMap mapFromJsonFile;
        QVariantMap allListsMap;
        QVariantMap observingListDataList;
        if ( jsonFile.size() > 0 ) {
            mapFromJsonFile = StelJsonParser::parse ( jsonFile.readAll() ).toMap();
            allListsMap = mapFromJsonFile.value ( QString ( KEY_OBSERVING_LISTS ) ).toMap();
        }

        QString defaultListValue = mapFromJsonFile.value ( QString ( KEY_DEFAULT_LIST_UUID ) ).toString();
        if ( defaultListValue.isEmpty() ) {
            mapFromJsonFile.insert ( KEY_DEFAULT_LIST_UUID, "" );
        }

        if ( checkIfBookmarksListExists ( allListsMap ) ) {
            //the bookmarks file is already loaded
            return;
        }

        // Description
        QString description = QString ( BOOKMARKS_LIST_DESCRIPTION );
        observingListDataList.insert ( QString ( KEY_DESCRIPTION ), description );

        // Julian day
        QString JDString = "";
        double JD = core->getJD();
        JDString = QString::number ( JD, 'f', 6 );
        observingListDataList.insert ( QString ( KEY_JD ), JDString );

        // Location
        QString Location = "";
        StelLocation loc = core->getCurrentLocation();
        if ( loc.name.isEmpty() ) {
            Location = QString ( "%1, %2" ).arg ( loc.latitude ).arg ( loc.longitude );
        } else {
            Location = QString ( "%1, %2" ).arg ( loc.name ).arg ( loc.region );
        }
        observingListDataList.insert ( QString ( KEY_LOCATION ), Location );

        // Name of the liste
        observingListDataList.insert ( QString ( KEY_NAME ), BOOKMARKS_LIST_NAME );

        // List of objects
        QVariantList listOfObjects;
        QHashIterator<QString, bookmark> i ( bookmarksCollection );
        while ( i.hasNext() ) {
            i.next();

            bookmark item = i.value();
            QVariantMap obl;
            QString objectName = item.name;
            obl.insert ( QString ( KEY_DESIGNATION ), objectName );
            listOfObjects.push_back ( obl );
        }

        observingListDataList.insert ( QString ( KEY_OBJECTS ), listOfObjects );
        observingListDataList.insert ( QString ( KEY_SORTING ), SORTING_BY_NAME );

        QList<QString> keys = bookmarksCollection.keys();
        QString oblListUuid;
        if ( keys.empty() ) {
            oblListUuid = QUuid::createUuid().toString();
        } else {
            oblListUuid =  keys.at ( 0 );
        }

        mapFromJsonFile.insert ( KEY_VERSION, FILE_VERSION );
        mapFromJsonFile.insert ( KEY_SHORT_NAME,SHORT_NAME_VALUE );

        allListsMap.insert ( oblListUuid, observingListDataList );
        mapFromJsonFile.insert ( QString ( KEY_OBSERVING_LISTS ), allListsMap );

        jsonFile.resize ( 0 );
        StelJsonParser::write ( mapFromJsonFile, &jsonFile );
        jsonFile.flush();
        jsonFile.close();

    } catch ( std::runtime_error &e ) {
        qCritical() << "[ObservingList] File format is wrong! Error: " << e.what();
        return;
    }
}


/*
 * Check if bookmarks list already exists in observing list file,
 * in fact if the file of bookarks has already be loaded.
*/
bool ObsListDialog::checkIfBookmarksListExists ( QVariantMap allListsMap )
{

    for ( auto bookmarkKey : allListsMap.keys() ) {

        QVariantMap bookmarkData = allListsMap.value ( bookmarkKey ).toMap();
        QString listName = bookmarkData.value ( KEY_NAME ).toString();

        if ( QString::compare ( QString ( BOOKMARKS_LIST_NAME ), listName ) == 0 ) {
            return true;
        }
    }

    return false;
}



/*
 * Select and go to object
*/
void ObsListDialog::selectAndGoToObject ( QModelIndex index )
{
    QStandardItem * selectedItem = obsListListModel->itemFromIndex ( index );
    int rawNumber = selectedItem->row();

    QStandardItem * uuidItem = obsListListModel->item ( rawNumber, ColumnUUID );
    QString itemUuid = uuidItem->text();
    observingListItem item = observingListItemCollection.value ( itemUuid );

    if ( !item.jd.isEmpty() ) {
        core->setJD ( item.jd.toDouble() );
    }
    if ( !item.location.isEmpty() ) {
        StelLocationMgr* locationMgr = &StelApp::getInstance().getLocationMgr();
        core->moveObserverTo ( locationMgr->locationForString ( item.location ) );
    }

    StelMovementMgr* mvmgr = GETSTELMODULE ( StelMovementMgr );
    objectMgr->unSelect();

    bool status = objectMgr->findAndSelect ( item.name );
    float amd = mvmgr->getAutoMoveDuration();
    if ( !item.ra.isEmpty() && !item.dec.isEmpty() && !status ) {
        Vec3d pos;
        StelUtils::spheToRect ( StelUtils::getDecAngle ( item.ra.trimmed() ), StelUtils::getDecAngle ( item.dec.trimmed() ), pos );
        if ( item.name.contains ( "marker", Qt::CaseInsensitive ) ) {
            // Add a custom object on the sky
            GETSTELMODULE ( CustomObjectMgr )->addCustomObject ( item.name, pos, item.isVisibleMarker );
            status = objectMgr->findAndSelect ( item.name );
        } else {
            // The unnamed stars
            StelObjectP sobj;
            const StelProjectorP prj = core->getProjection ( StelCore::FrameJ2000 );
            double fov = 5.0;
            if ( item.fov > 0.0 ) {
                fov = item.fov;
            }

            mvmgr->zoomTo ( fov, 0.0 );
            mvmgr->moveToJ2000 ( pos, mvmgr->mountFrameToJ2000 ( Vec3d ( 0., 0., 1. ) ), 0.0 );

            QList<StelObjectP> candidates = GETSTELMODULE ( StarMgr )->searchAround ( pos, 0.5, core );
            if ( candidates.empty() ) { // The FOV is too big, let's reduce it
                mvmgr->zoomTo ( 0.5*fov, 0.0 );
                candidates = GETSTELMODULE ( StarMgr )->searchAround ( pos, 0.5, core );
            }

            Vec3d winpos;
            prj->project ( pos, winpos );
            float xpos = winpos[0];
            float ypos = winpos[1];
            float best_object_value = 1000.f;
            for ( const auto& obj : candidates ) {
                prj->project ( obj->getJ2000EquatorialPos ( core ), winpos );
                float distance = std::sqrt ( ( xpos-winpos[0] ) * ( xpos-winpos[0] ) + ( ypos-winpos[1] ) * ( ypos-winpos[1] ) );
                if ( distance < best_object_value ) {
                    best_object_value = distance;
                    sobj = obj;
                }
            }

            if ( sobj ) {
                status = objectMgr->setSelectedObject ( sobj );
            }
        }
    }

    if ( status ) {
        const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
        if ( !newSelected.empty() ) {
            mvmgr->moveToObject ( newSelected[0], amd );
            mvmgr->setFlagTracking ( true );
        }
    }
}

/*
 * Method called when a list name is selected in the combobox
*/
void ObsListDialog::loadSelectedObservingList ( int selectedIndex )
{
    ui->obsListEditListButton->setEnabled ( true );
    ui->obsListDeleteButton->setEnabled ( true );
    QString listUuid = ui->obsListComboBox->itemData ( selectedIndex ).toString();
    selectedObservingListUuid = listUuid.toStdString();
    loadObservingList ( listUuid );
}

/*
 * Delete the selected list
*/
void ObsListDialog::obsListDeleteButtonPressed()
{
    bool res = askConfirmation();

    if ( res ) {
        QVariantMap map;
        QFile jsonFile ( observingListJsonPath );
        if ( !jsonFile.open ( QIODevice::ReadWrite ) ) {
            qWarning() << "[ObservingList] cannot open" << QDir::toNativeSeparators ( JSON_FILE_NAME );
        } else {
            try {
                QVariantMap newMap;
                QVariantMap newObsListMap;
                map = StelJsonParser::parse ( jsonFile.readAll() ).toMap();

                newMap.insert ( QString ( KEY_DEFAULT_LIST_UUID ), map.value ( QString ( KEY_DEFAULT_LIST_UUID ) ) );
                QVariantMap obsListMap = map.value ( QString ( KEY_OBSERVING_LISTS ) ).toMap();

                QMap<QString, QVariant>::iterator i;
                for ( i = obsListMap.begin(); i != obsListMap.end(); ++i ) {
                    if ( i.key().compare ( QString::fromStdString ( selectedObservingListUuid ) ) !=0 ) {
                        newObsListMap.insert ( i.key(),i.value() );
                    }
                }

                newMap.insert ( QString ( KEY_OBSERVING_LISTS ),newObsListMap );
                newMap.insert ( QString ( KEY_SHORT_NAME ), map.value ( QString ( KEY_SHORT_NAME ) ) );
                newMap.insert ( QString ( KEY_VERSION ), map.value ( QString ( KEY_VERSION ) ) );
                objectMgr->unSelect();
                observingListItemCollection.clear();

                // Clear row in model
                obsListListModel->removeRows ( 0,obsListListModel->rowCount() );
                ui->obsListCreationDateLineEdit->setText ( "" );
                ui->obsListDescriptionTextEdit->setPlainText ( "" );
                int currentIndex = ui->obsListComboBox->currentIndex();
                ui->obsListComboBox->removeItem ( currentIndex );
                
                //TODO pas utile car listeName_ est réinitialiséé et rechargée dans loadListsName().
                //TODO a supprimer après les tests finaux.
                //QString listName = ui->obsListComboBox->itemText(currentIndex);
                //listName_.removeOne(listName);
                
                selectedObservingListUuid = "";


                jsonFile.resize ( 0 );
                StelJsonParser::write ( newMap, &jsonFile );
                jsonFile.flush();
                jsonFile.close();
                
                clearHighlight();
                loadListsName();

                if ( ui->obsListComboBox->count() > 0 ) {
                    ui->obsListComboBox->setCurrentIndex ( 0 );
                    loadSelectedObservingList ( 0 );
                }

            } catch ( std::runtime_error &e ) {
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
 * Slot to manage the close of obsListCreateEditDialog
*/
void ObsListDialog::obsListCreateEditDialogClosed()
{
    // We must reload the list of list name
    loadListsName();
    int index = 0;
    if ( !selectedObservingListUuid.empty() ) {
        index = ui->obsListComboBox->findData ( QString::fromStdString ( selectedObservingListUuid ) ) ;
    }
    if ( index != -1 ) {
        ui->obsListComboBox->setCurrentIndex ( index );
        loadSelectedObservingList ( index );
    }
    
    //TODO remove after
    qDebug() << "obsListCreateEditDialogClosed() before kill ObsListCreateEditDialog";

    ObsListCreateEditDialog::kill();
    createEditDialog_instance = Q_NULLPTR;
}


/*
 * Override of the StelDialog::setVisible((bool) methode
 * We need to load the default liste when opening the obsListDialog
*/
void ObsListDialog::setVisible ( bool v )
{
    if ( v ) {
        StelDialog::setVisible ( true );
        this->loadDefaultList();
    } else {
        StelDialog::setVisible ( false );
    }
}

/*
 * Sort the obsListTreeView by the column name given in parameter
*/
void ObsListDialog::sortObsListTreeViewByColumnName ( QString columnName )
{
    if ( QString::compare ( columnName,SORTING_BY_NAME ) == 0 ) {
        obsListListModel->sort ( COLUMN_NUMBER_NAME, Qt::AscendingOrder );
    } else if ( QString::compare ( columnName,SORTING_BY_TYPE ) == 0 ) {
        obsListListModel->sort ( COLUMN_NUMBER_TYPE, Qt::AscendingOrder );
    } else if ( QString::compare ( columnName,SORTING_BY_RA ) == 0 ) {
        obsListListModel->sort ( COLUMN_NUMBER_RA, Qt::AscendingOrder );
    } else if ( QString::compare ( columnName,SORTING_BY_DEC ) == 0 ) {
        obsListListModel->sort ( COLUMN_NUMBER_DEC, Qt::AscendingOrder );
    } else if ( QString::compare ( columnName,SORTING_BY_MAGNITUDE ) == 0 ) {
        obsListListModel->sort ( COLUMN_NUMBER_MAGNITUDE, Qt::AscendingOrder );
    } else if ( QString::compare ( columnName,SORTING_BY_CONSTELLATION ) == 0 ) {
        obsListListModel->sort ( COLUMN_NUMBER_CONSTELLATION, Qt::AscendingOrder );
    }
}





