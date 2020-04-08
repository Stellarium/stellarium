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

#include <StelTranslator.hpp>
#include <QUuid>
#include <QDir>
#include <QDateTime>
#include <QDebug>

#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelModuleMgr.hpp"
#include "NebulaMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelUtils.hpp"
#include "StelJsonParser.hpp"

#include "ObsListCreateEditDialog.hpp"
#include "ui_obsListCreateEditDialog.h"

using namespace std;

ObsListCreateEditDialog * ObsListCreateEditDialog::m_instance = nullptr;

ObsListCreateEditDialog::ObsListCreateEditDialog ( string listUuid )
{
    listUuid_ = listUuid;

    ui = new Ui_obsListCreateEditDialogForm();
    core = StelApp::getInstance().getCore();
    objectMgr = GETSTELMODULE ( StelObjectMgr );
    obsListListModel = new QStandardItemModel ( 0,ColumnCount );
    observingListJsonPath = StelFileMgr::findFile ( "data", ( StelFileMgr::Flags ) ( StelFileMgr::Directory|StelFileMgr::Writable ) ) + "/" + QString ( JSON_FILE_NAME );
    sorting = "";
}

ObsListCreateEditDialog::~ObsListCreateEditDialog()
{
    delete ui;
    delete obsListListModel;
}

/**
 * Get instance of class
*/
ObsListCreateEditDialog * ObsListCreateEditDialog::Instance ( string listUuid )
{
    if ( m_instance == nullptr ) {
        m_instance = new ObsListCreateEditDialog ( listUuid );
    }

    return m_instance;
}


/*
 * Initialize the dialog widgets and connect the signals/slots.
*/
void ObsListCreateEditDialog::createDialogContent()
{
    ui->setupUi ( dialog );

    //Signals and slots
    connect ( &StelApp::getInstance(), SIGNAL ( languageChanged() ), this, SLOT ( retranslate() ) );
    connect ( ui->closeStelWindow, SIGNAL ( clicked() ), this, SLOT ( close() ) );

    connect ( ui->obsListAddObjectButton, SIGNAL ( clicked() ), this, SLOT ( obsListAddObjectButtonPressed() ) );
    connect ( ui->obsListExitButton, SIGNAL ( clicked() ), this, SLOT ( obsListExitButtonPressed() ) );
    connect ( ui->obsListSaveButton, SIGNAL ( clicked() ), this, SLOT ( obsListSaveButtonPressed() ) );
    connect ( ui->obsListRemoveObjectButton, SIGNAL ( clicked() ), this, SLOT ( obsListRemoveObjectButtonPressed() ) );
    connect ( ui->obsListImportListButton, SIGNAL ( clicked() ), this, SLOT ( obsListImportListButtonPresssed() ) );
    connect ( ui->obsListExportListButton, SIGNAL ( clicked() ), this, SLOT ( obsListExportListButtonPressed() ) );

    //Initializing the list of observing list
    obsListListModel->setColumnCount ( ColumnCount );
    setObservingListHeaderNames();

    ui->obsListCreationEditionTreeView->setModel ( obsListListModel );
    ui->obsListCreationEditionTreeView->header()->setSectionsMovable ( false );
    ui->obsListCreationEditionTreeView->header()->setSectionResizeMode ( ColumnName, QHeaderView::ResizeToContents );
    ui->obsListCreationEditionTreeView->header()->setSectionResizeMode ( ColumnType, QHeaderView::ResizeToContents );
    ui->obsListCreationEditionTreeView->header()->setSectionResizeMode ( ColumnRa, QHeaderView::ResizeToContents );
    ui->obsListCreationEditionTreeView->header()->setSectionResizeMode ( ColumnDec, QHeaderView::ResizeToContents );
    ui->obsListCreationEditionTreeView->header()->setSectionResizeMode ( ColumnConstellation, QHeaderView::ResizeToContents );
    ui->obsListCreationEditionTreeView->header()->setSectionResizeMode ( ColumnMagnitude, QHeaderView::ResizeToContents );
    ui->obsListCreationEditionTreeView->header()->setStretchLastSection ( true );
    ui->obsListCreationEditionTreeView->hideColumn ( ColumnUUID );
    ui->obsListCreationEditionTreeView->hideColumn ( ColumnNameI18n );
    ui->obsListCreationEditionTreeView->hideColumn ( ColumnJD );
    ui->obsListCreationEditionTreeView->hideColumn ( ColumnLocation );
    //Enable the sort for columns
    ui->obsListCreationEditionTreeView->setSortingEnabled ( true );

    QHeaderView * header = ui->obsListCreationEditionTreeView->header();
    connect ( header, SIGNAL ( sectionClicked ( int ) ), this, SLOT ( headerClicked ( int ) ) );

    if ( listUuid_.size() == 0 ) {
        // case of creation mode
        isCreationMode = true;
        ui->stelWindowTitle->setText ( "Observing list creation mode" );
    } else {
        // case of edit mode
        isCreationMode = false;
        ui->stelWindowTitle->setText ( "Observing list modification mode" );
        loadObservingList();
    }
}

/*
 * Retranslate dialog
*/
void ObsListCreateEditDialog::retranslate()
{
    if ( dialog ) {
        ui->retranslateUi ( dialog );
    }
}

/*
 * Style changed
*/
void ObsListCreateEditDialog::styleChanged()
{
    // Nothing for now
}

/*
 * Set the header for the observing list table
 * (obsListTreeVView)
*/
void ObsListCreateEditDialog::setObservingListHeaderNames()
{
    const QStringList headerStrings = {
        "UUID", // Hided column
        q_ ( "Object name" ),
        q_ ( "Object Name I18N" ),
        q_ ( "Type" ),
        q_ ( "Right ascencion" ),
        q_ ( "Declination" ),
        q_ ( "Magnitude" ),
        q_ ( "Constellation" ),
        q_ ( "Date" ), // Hided column
        q_ ( "Location" ) // Hided column
    };

    obsListListModel->setHorizontalHeaderLabels ( headerStrings );
}


/*
 * Add row in the obsListListModel
*/
void ObsListCreateEditDialog::addModelRow ( int number, QString uuid, QString name, QString nameI18n, QString type, QString ra, QString dec, QString magnitude, QString constellation )
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
        ui->obsListCreationEditionTreeView->resizeColumnToContents ( i );
    }
}


/*
 * Slot for button obsListAddObjectButton
*/
void ObsListCreateEditDialog::obsListAddObjectButtonPressed()
{
    const QList<StelObjectP>& selectedObject = objectMgr->getSelectedObject();
    if ( !selectedObject.isEmpty() ) {

        int lastRow = obsListListModel->rowCount();
        QString objectUuid = QUuid::createUuid().toString();

        QString objectName = selectedObject[0]->getEnglishName();
        QString objectNameI18n = selectedObject[0]->getNameI18n();
        if ( selectedObject[0]->getType() =="Nebula" ) {
            objectName = GETSTELMODULE ( NebulaMgr )->getLatestSelectedDSODesignation();
        }

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

        if ( objectName.isEmpty() ) {
            objectName = QString ( "%1, %2" ).arg ( objectRaStr, objectDecStr );
            objectNameI18n = q_ ( "Unnamed object" );
            fov = GETSTELMODULE ( StelMovementMgr )->getCurrentFov();
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
            Location = QString ( "%1, %2" ).arg ( loc.name ).arg ( loc.country );
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
            item.jd = JDs;
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
        qWarning() << "selected object is empty !";
    }
}

/*
 * Slot for button obsListRemoveObjectButton
*/
void ObsListCreateEditDialog::obsListRemoveObjectButtonPressed()
{
    int number = ui->obsListCreationEditionTreeView->currentIndex().row();
    QString uuid = obsListListModel->index ( number, ColumnUUID ).data().toString();
    obsListListModel->removeRow ( number );
    observingListItemCollection.remove ( uuid );
}

/*
 * Save observed object into json file
*/
void ObsListCreateEditDialog::saveObservedObject()
{

    QString listName = ui->nameOfListLineEdit->text();
    if ( observingListJsonPath.isEmpty() || listName.isEmpty() ) {
        qWarning() << "[ObservingList] Error saving observing list";
        return;
    }

    QFile jsonFile ( observingListJsonPath );
    if ( !jsonFile.open ( QIODevice::ReadWrite|QIODevice::Text|QIODevice::ExistingOnly ) ) {
        qWarning() << "[ObservingList] observing list can not be saved. A file can not be open for reading and writing:"
                   << QDir::toNativeSeparators ( observingListJsonPath );
        return;
    }

    try {

        QVariantMap mapFromJsonFile = StelJsonParser::parse ( jsonFile.readAll() ).toMap();
        QVariantMap allListsMap = mapFromJsonFile.value ( QString ( KEY_OBSERVING_LISTS ) ).toMap();


        QVariantMap observingListDataList;

        // Description
        QString description = ui->descriptionLineEdit->text();
        observingListDataList.insert ( QString ( KEY_DESCRIPTION ), description );

        // Julian day
        QString JDString = "";
        double JD = core->getJD();
        JDString = StelUtils::julianDayToISO8601String ( JD + core->getUTCOffset ( JD ) /24. ).replace ( "T", " " );
        observingListDataList.insert ( QString ( KEY_JD ), JDString );

        // Location
        QString Location = "";
        StelLocation loc = core->getCurrentLocation();
        if ( loc.name.isEmpty() ) {
            Location = QString ( "%1, %2" ).arg ( loc.latitude ).arg ( loc.longitude );
        } else {
            Location = QString ( "%1, %2" ).arg ( loc.name ).arg ( loc.country );
        }
        observingListDataList.insert ( QString ( KEY_LOCATION ), Location );

        // Name of the liste
        QString name = ui->nameOfListLineEdit->text();
        observingListDataList.insert ( QString ( KEY_NAME ), name );

        /// List of objects
        QVariantList listOfObjects;
        QHashIterator<QString, observingListItem> i ( observingListItemCollection );
        while ( i.hasNext() ) {
            i.next();

            observingListItem item = i.value();
            QVariantMap obl;
            QString objectName = item.name;
            obl.insert ( QString ( KEY_DESIGNATION ), objectName );
            listOfObjects.push_back ( obl );
        }

        observingListDataList.insert ( QString ( KEY_OBJECTS ), listOfObjects );
        observingListDataList.insert ( QString ( KEY_SORTING ), sorting );

        QString oblListUuid = QUuid::createUuid().toString();
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
 * Slot for button obsListExportListButton
*/
void ObsListCreateEditDialog::obsListExportListButtonPressed()
{
    //TODO
}

/*
 * Slot for button obsListImportListButton
*/
void ObsListCreateEditDialog::obsListImportListButtonPresssed()
{
    //TODO
}

/*
 * Slot for button obsListSaveButton
*/
void ObsListCreateEditDialog::obsListSaveButtonPressed()
{
    saveObservedObject();
}

/*
 * Slot for button obsListExitButton
*/
void ObsListCreateEditDialog::obsListExitButtonPressed()
{
    this->close();
    emit exitButtonClicked();
}


/*
 * Slot for obsListCreationEditionTreeView header
*/
void ObsListCreateEditDialog::headerClicked ( int index )
{
    qDebug() << "Index de la colonne: " << index ;
    switch ( index ) {
    case ColumnName:
        sorting = "name";
        break;
    case ColumnType:
        sorting = "type";
        break;
    case ColumnRa:
        sorting = "right ascension";
        break;
    case ColumnDec:
        sorting = "declination";
        break;
    case ColumnMagnitude:
        sorting = "magnitude";
        break;
    case ColumnConstellation:
        sorting = "constellation";
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
void ObsListCreateEditDialog::loadObservingList()
{
    QVariantMap map;
    QFile jsonFile ( observingListJsonPath );
    if ( !jsonFile.open ( QIODevice::ReadOnly ) ) {
        qWarning() << "[ObservingList] cannot open" << QDir::toNativeSeparators ( JSON_FILE_NAME );

    } else {
        try {
            map = StelJsonParser::parse ( jsonFile.readAll() ).toMap();
            jsonFile.close();

            observingListItemCollection.clear();
            const QString keyUuid = QString::fromStdString ( listUuid_ );
            QVariantMap observingListMap = map.value ( QString ( KEY_OBSERVING_LISTS ) ).toMap().value ( keyUuid ).toMap();
            QVariantList listOfObjects;

            QString listeName = observingListMap.value ( QString ( KEY_NAME ) ).value<QString>();
            ui->nameOfListLineEdit->setText ( listeName );
            QString listDescription = observingListMap.value ( QString ( KEY_DESCRIPTION ) ).value<QString>();
            ui->descriptionLineEdit->setText ( listDescription );

            if ( observingListMap.value ( QString ( KEY_OBJECTS ) ).canConvert<QVariantList>() ) {
                QVariant data = observingListMap.value ( QString ( KEY_OBJECTS ) );
                listOfObjects = data.value<QVariantList>();
            } else {
                qCritical() << "[ObservingList] conversion error";
                return;
            }

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
                                Location = QString ( "%1, %2" ).arg ( loc.name ).arg ( loc.country );
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
                                item.jd = JDs;
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

        } catch ( std::runtime_error &e ) {
            qWarning() << "[ObservingList] File format is wrong! Error: " << e.what();
            return;
        }
    }
}


/*
 * Destructor of singleton
*/
void ObsListCreateEditDialog::kill()
{
    if ( m_instance != nullptr ) {
        delete m_instance;
        m_instance = nullptr;
    }
}







