/*
 * Stellarium
 * Copyright (C) 2016 Alexander Wolf
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelLocaleMgr.hpp"
#include "StelLocation.hpp"
#include "StelLocationMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelJsonParser.hpp"
#include "AngleSpinBox.hpp"
#include "NebulaMgr.hpp"

#include "BookmarksDialog.hpp"
#include "ui_bookmarksDialog.h"

BookmarksDialog::BookmarksDialog(QObject *parent)
	: StelDialog(parent)
{
	dialogName = "Bookmarks";
	ui = new Ui_bookmarksDialogForm;
	core = StelApp::getInstance().getCore();
	objectMgr = GETSTELMODULE(StelObjectMgr);
	bookmarksListModel = new QStandardItemModel(0, ColumnCount);
	bookmarksJsonPath = StelFileMgr::findFile("data", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/bookmarks.json";
}

BookmarksDialog::~BookmarksDialog()
{
	delete ui;
	delete bookmarksListModel;
}

void BookmarksDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setBookmarksHeaderNames();		
	}
}

void BookmarksDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(ui->addBookmarkButton, SIGNAL(clicked()), this, SLOT(addBookmarkButtonPressed()));
	connect(ui->removeBookmarkButton, SIGNAL(clicked()), this, SLOT(removeBookmarkButtonPressed()));
	connect(ui->goToButton, SIGNAL(clicked()), this, SLOT(goToBookmarkButtonPressed()));
	connect(ui->clearBookmarksButton, SIGNAL(clicked()), this, SLOT(clearBookmarksButtonPressed()));
	connect(ui->bookmarksTreeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentBookmark(QModelIndex)));

	//Initializing the list of bookmarks
	bookmarksListModel->setColumnCount(ColumnCount);
	setBookmarksHeaderNames();

	ui->bookmarksTreeView->setModel(bookmarksListModel);
	ui->bookmarksTreeView->header()->setSectionsMovable(false);
	ui->bookmarksTreeView->header()->setSectionResizeMode(ColumnName, QHeaderView::ResizeToContents);
	ui->bookmarksTreeView->header()->setStretchLastSection(true);
	ui->bookmarksTreeView->hideColumn(0);

	loadBookmarks();
}

void BookmarksDialog::setBookmarksHeaderNames()
{
	QStringList headerStrings;
	headerStrings << "UUID"; // Hide the column
	headerStrings << q_("English name of object");
	headerStrings << q_("Date and Time");	
	headerStrings << q_("Location of observer");

	bookmarksListModel->setHorizontalHeaderLabels(headerStrings);
}

void BookmarksDialog::addModelRow(int number, QString uuid, QString name, QString Date, QString Location)
{
	QStandardItem* tempItem = 0;

	tempItem = new QStandardItem(uuid);
	tempItem->setEditable(false);
	bookmarksListModel->setItem(number, ColumnUUID, tempItem);

	tempItem = new QStandardItem(name);
	tempItem->setEditable(false);
	bookmarksListModel->setItem(number, ColumnName, tempItem);

	tempItem = new QStandardItem(Date);
	tempItem->setEditable(false);
	bookmarksListModel->setItem(number, ColumnDate, tempItem);

	tempItem = new QStandardItem(Location);
	tempItem->setEditable(false);
	bookmarksListModel->setItem(number, ColumnLocation, tempItem);

	for(int i = 0; i < ColumnCount; ++i)
	{
	    ui->bookmarksTreeView->resizeColumnToContents(i);
	}
}

void BookmarksDialog::addBookmarkButtonPressed()
{
	const QList<StelObjectP>& selected = objectMgr->getSelectedObject();
	if (!selected.isEmpty())
	{
		QString name = selected[0]->getEnglishName();
		if (selected[0]->getType()=="Nebula")
			name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();

		if (!name.isEmpty()) // Don't allow adding objects without name!
		{
			bool dateTimeFlag = ui->dateTimeCheckBox->isChecked();
			bool locationFlag = ui->locationCheckBox->isChecked();

			QString JDs = "";
			double JD = -1.;

			if (dateTimeFlag)
			{
				JD = core->getJD();
				JDs = StelUtils::julianDayToISO8601String(JD + StelUtils::getGMTShiftFromQT(JD)/24.).replace("T", " ");
			}

			QString Location = "";
			if (locationFlag)
			{
				StelLocation loc = core->getCurrentLocation();
				if (loc.name.isEmpty())
					Location = QString("%1, %2").arg(loc.latitude).arg(loc.longitude);
				else
					Location = QString("%1, %2").arg(loc.name).arg(loc.country);
			}

			int lastRow = bookmarksListModel->rowCount();

			QString uuid = QUuid::createUuid().toString();
			addModelRow(lastRow, uuid, name, JDs, Location);

			bookmark bm;
			bm.name	= name;
			if (!JDs.isEmpty())
				bm.jd	= QString::number(JD, 'f', 6);
			if (!Location.isEmpty())
				bm.location = Location;

			bookmarksCollection.insert(uuid, bm);

			saveBookmarks();
		}
	}
}

void BookmarksDialog::removeBookmarkButtonPressed()
{
	int number = ui->bookmarksTreeView->currentIndex().row();
	QString uuid = bookmarksListModel->index(number, 0).data().toString();
	bookmarksListModel->removeRow(number);
	bookmarksCollection.remove(uuid);
	saveBookmarks();
}

void BookmarksDialog::clearBookmarksButtonPressed()
{
	bookmarksListModel->clear();
	bookmarksCollection.clear();
	setBookmarksHeaderNames();
	ui->bookmarksTreeView->hideColumn(0);
	saveBookmarks();
}

void BookmarksDialog::goToBookmarkButtonPressed()
{
	goToBookmark(bookmarksListModel->index(ui->bookmarksTreeView->currentIndex().row(), ColumnUUID).data().toString());
}

void BookmarksDialog::selectCurrentBookmark(const QModelIndex &modelIdx)
{
	goToBookmark(modelIdx.sibling(modelIdx.row(), ColumnUUID).data().toString());
}

void BookmarksDialog::goToBookmark(QString uuid)
{
	if (!uuid.isEmpty())
	{
		bookmark bm = bookmarksCollection.value(uuid);
		if (!bm.jd.isEmpty())
		{
			core->setJD(bm.jd.toDouble());
		}
		if (!bm.location.isEmpty())
		{
			StelLocationMgr* locationMgr = &StelApp::getInstance().getLocationMgr();
			core->moveObserverTo(locationMgr->locationForString(bm.location));
		}

		objectMgr->unSelect();
		if (objectMgr->findAndSelect(bm.name))
		{
			const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
			if (!newSelected.empty())
			{
				StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
				mvmgr->moveToObject(newSelected[0], mvmgr->getAutoMoveDuration());
				mvmgr->setFlagTracking(true);
			}
		}
	}
}

void BookmarksDialog::loadBookmarks()
{
	QVariantMap map;
	QFile jsonFile(bookmarksJsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly))
		qWarning() << "Bookmarks: cannot open" << QDir::toNativeSeparators(bookmarksJsonPath);
	else
	{
		map = StelJsonParser::parse(jsonFile.readAll()).toMap();
		jsonFile.close();
	}

	bookmarksCollection.clear();
	QVariantMap bookmarksMap = map.value("bookmarks").toMap();
	int i = 0;
	foreach(QString bookmarkKey, bookmarksMap.keys())
	{
		QVariantMap bookmarkData = bookmarksMap.value(bookmarkKey).toMap();
		bookmark bm;

		QString JDs = "";

		bm.name		= bookmarkData.value("name").toString();
		QString JD	= bookmarkData.value("jd").toString();
		if (!JD.isEmpty())
		{
			bm.jd	= JD;
			JDs = StelUtils::julianDayToISO8601String(JD.toDouble() + StelUtils::getGMTShiftFromQT(JD.toDouble())/24.).replace("T", " ");
		}
		QString Location = bookmarkData.value("location").toString();
		if (!Location.isEmpty())
			bm.location = Location;

		bookmarksCollection.insert(bookmarkKey, bm);
		addModelRow(i, bookmarkKey, bm.name, JDs, Location);
		i++;
	}
}

void BookmarksDialog::saveBookmarks()
{
	if (bookmarksJsonPath.isEmpty())
	{
		qWarning() << "Bookmarks: Error saving bookmarks";
		return;
	}
	QFile jsonFile(bookmarksJsonPath);
	if(!jsonFile.open(QFile::WriteOnly|QFile::Text))
	{
		qWarning() << "Bookmarks: bookmarks can not be saved. A file can not be open for writing:"
			   << QDir::toNativeSeparators(bookmarksJsonPath);
		return;
	}

	QVariantMap bookmarksDataList;
	QHashIterator<QString, bookmark> i(bookmarksCollection);
	while (i.hasNext())
	{
	    i.next();

	    bookmark sp = i.value();
	    QVariantMap bm;
	    bm.insert("name",	sp.name);
	    if (!sp.jd.isEmpty())
		    bm.insert("jd", sp.jd);
	    if (!sp.location.isEmpty())
		    bm.insert("location", sp.location);

	    bookmarksDataList.insert(i.key(), bm);
	}

	QVariantMap bmList;
	bmList.insert("bookmarks", bookmarksDataList);

	//Convert the tree to JSON
	StelJsonParser::write(bmList, &jsonFile);
	jsonFile.flush();
	jsonFile.close();

}

