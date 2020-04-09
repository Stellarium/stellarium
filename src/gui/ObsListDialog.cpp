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

using namespace std;


ObsListDialog::ObsListDialog ( QObject* parent ) : StelDialog ( "Observing list", parent )
{
    ui = new Ui_obsListDialogForm();
    core = StelApp::getInstance().getCore();
    objectMgr = GETSTELMODULE ( StelObjectMgr );
    labelMgr = GETSTELMODULE ( LabelMgr );
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
    connect ( ui->obsListExitButton, SIGNAL ( clicked() ), this, SLOT ( obsListExitButtonPressed() ) );
    connect ( ui->obsListTreeView, SIGNAL ( doubleClicked ( QModelIndex ) ), this, SLOT ( selectAndGoToObject ( QModelIndex ) ) );

    //BookmarksListCombo
    connect ( ui->obsListComboBox, SIGNAL ( activated ( int ) ), this, SLOT ( loadSelectedObservingList ( int ) ) );

    //Initializing the list of observing list
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

    loadListsName();
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
        "UUID", // Hided column
        q_ ( "Object name" ),
        q_ ( "Object Name I18N" ), // Hided column
        q_ ( "Type" ),
        q_ ( "Right ascencion" ),
        q_ ( "Declination" ),
        q_ ( "Magnitude" ),
        q_ ( "Constellation" ),
        q_ ( "Date" ), // Hided column
        q_ ( "Location" ) // Hided column
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
    QList<Vec3d> highlights;
    highlights.clear();
    obsListClearHighLightButtonPressed(); // Enable fool protection
    int fontSize = StelApp::getInstance().getScreenFontSize();
    HighlightMgr* hlMgr = GETSTELMODULE ( HighlightMgr );
    QString color = StelUtils::vec3fToHtmlColor ( hlMgr->getColor() );
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
            for ( i = observingListsMap.begin(); i != observingListsMap.end(); ++i ) {
                QString listUuid = i.key();

                if ( i.value().canConvert<QVariantMap>() ) {
                    QVariant var = i.value();
                    QVariantMap data = var.value<QVariantMap>();
                    QString listName = data.value ( KEY_NAME ).value<QString>();
                    ui->obsListComboBox->addItem ( listName, listUuid );
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
void ObsListDialog::loadObservingList ( QString listUuid )
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


            // Clear model
            obsListListModel->removeRows ( 0,obsListListModel->rowCount() );

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
    qDebug() << "Selected index: " << selectedIndex;

    QString listUuid = ui->obsListComboBox->itemData ( selectedIndex ).toString();
    loadObservingList ( listUuid );

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
    ObsListCreateEditDialog::kill();
    createEditDialog_instance = Q_NULLPTR;
}





