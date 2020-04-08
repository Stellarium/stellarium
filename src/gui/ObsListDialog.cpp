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
#include "StelFileMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelUtils.hpp"
#include "ObsListDialog.hpp"

#include "ui_obsListDialog.h"

using namespace std;


ObsListDialog::ObsListDialog ( QObject* parent ) : StelDialog ( "Observing list", parent )
{
    ui = new Ui_obsListDialogForm();
    core = StelApp::getInstance().getCore();
    objectMgr = GETSTELMODULE ( StelObjectMgr );
    obsListListModel = new QStandardItemModel ( 0,ColumnCount );
    observingListJsonPath = StelFileMgr::findFile ( "data", ( StelFileMgr::Flags ) ( StelFileMgr::Directory|StelFileMgr::Writable ) ) + "/" + QString ( JSON_FILE_NAME );
    createEditDialog_instance = Q_NULLPTR;
}


ObsListDialog::~ObsListDialog()
{
    delete ui;
    delete obsListListModel;
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

    //Initializing the list of observing list
    obsListListModel->setColumnCount ( ColumnCount );
    setObservingListHeaderNames();

    ui->obsListTreeView->setModel ( obsListListModel );
    ui->obsListTreeView->header()->setSectionsMovable ( false );
    ui->obsListTreeView->header()->setSectionResizeMode ( ColumnName, QHeaderView::ResizeToContents );
    ui->obsListTreeView->header()->setStretchLastSection ( true );
    ui->obsListTreeView->hideColumn ( ColumnUUID );
    //Enable the sort for columns
    ui->obsListTreeView->setSortingEnabled ( true );
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
        "UUID", // Hided the column
        q_ ( "Object name" ),
        q_ ( "Type" ),
        q_ ( "Right ascencion" ),
        q_ ( "Declination" ),
        q_ ( "Magnitude" ),
        q_ ( "Constellation" )
    };

    obsListListModel->setHorizontalHeaderLabels ( headerStrings );;
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
    //TODO
}

/*
 * Slot for button obsListClearHighLightButton
*/
void ObsListDialog::obsListClearHighLightButtonPressed()
{
    //TODO
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
    //TODO: delete after - only for debug
    selectedObservingListUuid = "{c93719b6-7489-4403-8f4b-b898498c17f2}";

    if ( !selectedObservingListUuid.empty() ) {
        invokeObsListCreateEditDialog ( selectedObservingListUuid );
    } else {
        qWarning() << "The selected observing list name is empty";
    }
}


/**
 * Open the observing list create/edit dialog
*/
void ObsListDialog::invokeObsListCreateEditDialog ( string listUuid )
{
    createEditDialog_instance = ObsListCreateEditDialog::Instance ( listUuid );
    connect ( createEditDialog_instance, SIGNAL ( exitButtonClicked() ), this, SLOT ( obsListCreateEditDialogClosed() ) );
    createEditDialog_instance->setVisible ( true );
}


/*
 * Load the lists names for populate the combo box
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
            QVariantMap observingListsMap = map.value ( QString ( KEY_OBSERVING_LISTS ) ).toMap();
            
            QMap<QString, QVariant>::iterator i;
            for (i = observingListsMap.begin(); i != observingListsMap.end(); ++i){
                QString listUuid = i.key();
                
                if(i.value().canConvert<QVariantMap>()){
                    QVariant var = i.value();
                    QVariantMap data = var.value<QVariantMap>();
                    QString listName = data.value(KEY_NAME).value<QString>();
                    ui->obsListComboBox->addItem(listName, listUuid);
                }
                
                
            }

        } catch ( std::runtime_error &e ) {
            qWarning() << "[ObservingList] File format is wrong! Error: " << e.what();
            return;
        }

    }
}


/*
 * Load selectted observing list
*/
void ObsListDialog::loadSelectedObservingList ( QString listUuid )
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
            QVariantMap observingListMap = map.value ( QString ( KEY_OBSERVING_LISTS ) ).toMap().value ( listUuid ).toMap();
            QVariantList listOfObjects;

            QString listDescription = observingListMap.value ( QString ( KEY_DESCRIPTION ) ).value<QString>();
            ui->obsListDescriptionTextEdit->setPlainText ( listDescription );

            if ( observingListMap.value ( QString ( KEY_OBJECTS ) ).canConvert<QVariantList>() ) {
                QVariant data = observingListMap.value ( QString ( KEY_OBJECTS ) );
                listOfObjects = data.value<QVariantList>();
            } else {
                qCritical() << "[ObservingList] conversion error";
                return;
            }

            obsListListModel->clear();
            setObservingListHeaderNames();

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
 * Slot to manage the close of obsListCreateEditDialog
*/
void ObsListDialog::obsListCreateEditDialogClosed()
{
    ObsListCreateEditDialog::kill();
    createEditDialog_instance = Q_NULLPTR;
}





