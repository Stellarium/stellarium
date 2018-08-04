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
#include "CustomObjectMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelJsonParser.hpp"
#include "AngleSpinBox.hpp"
#include "NebulaMgr.hpp"

#include <QFileDialog>
#include <QDir>

#include "BookmarksDialog.hpp"
#include "ui_bookmarksDialog.h"

BookmarksDialog::BookmarksDialog(QObject *parent)
	: StelDialog("Bookmarks", parent)
{
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

void BookmarksDialog::styleChanged()
{
	// Nothing for now
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

	connect(ui->importBookmarksButton, SIGNAL(clicked()), this, SLOT(importBookmarks()));
	connect(ui->exportBookmarksButton, SIGNAL(clicked()), this, SLOT(exportBookmarks()));

	//Initializing the list of bookmarks
	bookmarksListModel->setColumnCount(ColumnCount);
	setBookmarksHeaderNames();

	ui->bookmarksTreeView->setModel(bookmarksListModel);
	ui->bookmarksTreeView->header()->setSectionsMovable(false);
	ui->bookmarksTreeView->header()->setSectionResizeMode(ColumnName, QHeaderView::ResizeToContents);
	ui->bookmarksTreeView->header()->setStretchLastSection(true);
	ui->bookmarksTreeView->hideColumn(ColumnUUID);

	loadBookmarks();
}

void BookmarksDialog::setBookmarksHeaderNames()
{
	QStringList headerStrings;
	headerStrings << "UUID"; // Hide the column
	headerStrings << q_("Object");
	headerStrings << q_("Localized name");	
	headerStrings << q_("Date and Time");	
	headerStrings << q_("Location of observer");

	bookmarksListModel->setHorizontalHeaderLabels(headerStrings);
}

void BookmarksDialog::addModelRow(int number, QString uuid, QString name, QString nameI18n, QString Date, QString Location)
{
	QStandardItem* tempItem = Q_NULLPTR;

	tempItem = new QStandardItem(uuid);
	tempItem->setEditable(false);
	bookmarksListModel->setItem(number, ColumnUUID, tempItem);

	tempItem = new QStandardItem(name);
	tempItem->setEditable(false);
	bookmarksListModel->setItem(number, ColumnName, tempItem);

	tempItem = new QStandardItem(nameI18n);
	tempItem->setEditable(false);
	bookmarksListModel->setItem(number, ColumnNameI18n, tempItem);

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
		QString name	 = selected[0]->getEnglishName();
		QString nameI18n = selected[0]->getNameI18n();
		if (selected[0]->getType()=="Nebula")
			name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();

		QString raStr = "", decStr = "";
		bool visibleFlag = false;
		if (selected[0]->getType()=="CustomObject")
		{
			if (name.isEmpty())
			{
				name = "Unnamed object";
				nameI18n = q_("Unnamed object");
			}
			float ra, dec;
			StelUtils::rectToSphe(&ra, &dec, selected[0]->getJ2000EquatorialPos(core));
			raStr = StelUtils::radToHmsStr(ra, false);
			decStr = StelUtils::radToDmsStr(dec, false);
			if (name.contains("marker", Qt::CaseInsensitive))
				visibleFlag = true;
		}

		bool dateTimeFlag = ui->dateTimeCheckBox->isChecked();
		bool locationFlag = ui->locationCheckBox->isChecked();

		QString JDs = "";
		double JD = -1.;

		if (dateTimeFlag)
		{
			JD = core->getJD();
			JDs = StelUtils::julianDayToISO8601String(JD + core->getUTCOffset(JD)/24.).replace("T", " ");
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
		addModelRow(lastRow, uuid, name, nameI18n, JDs, Location);

		bookmark bm;
		bm.name	= name;
		if (!nameI18n.isEmpty())
			bm.nameI18n = nameI18n;
		if (!raStr.isEmpty())
			bm.ra = raStr;
		if (!decStr.isEmpty())
			bm.dec = decStr;
		if (!JDs.isEmpty())
			bm.jd	= QString::number(JD, 'f', 6);
		if (!Location.isEmpty())
			bm.location = Location;
		if (!visibleFlag)
			bm.isVisibleMarker = visibleFlag;

		bookmarksCollection.insert(uuid, bm);

		saveBookmarks();
	}
}

void BookmarksDialog::removeBookmarkButtonPressed()
{
	int number = ui->bookmarksTreeView->currentIndex().row();
	QString uuid = bookmarksListModel->index(number, ColumnUUID).data().toString();
	bookmarksListModel->removeRow(number);
	bookmarksCollection.remove(uuid);
	saveBookmarks();
}

void BookmarksDialog::clearBookmarksButtonPressed()
{
	bookmarksListModel->clear();
	bookmarksCollection.clear();
	setBookmarksHeaderNames();
	ui->bookmarksTreeView->hideColumn(ColumnUUID);
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

		StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
		objectMgr->unSelect();
		bool status = objectMgr->findAndSelect(bm.name);
		if (!bm.ra.isEmpty() && !bm.dec.isEmpty() && !status)
		{
			Vec3d pos;
			StelUtils::spheToRect(StelUtils::getDecAngle(bm.ra.trimmed()), StelUtils::getDecAngle(bm.dec.trimmed()), pos);
			// Add a custom object on the sky
			GETSTELMODULE(CustomObjectMgr)->addCustomObject(bm.name, pos, bm.isVisibleMarker);
			status = objectMgr->findAndSelect(bm.name);
		}

		if (status)
		{
			const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
			if (!newSelected.empty())
			{
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
		qWarning() << "[Bookmarks] cannot open" << QDir::toNativeSeparators(bookmarksJsonPath);
	else
	{
		try
		{
			map = StelJsonParser::parse(jsonFile.readAll()).toMap();
			jsonFile.close();

			bookmarksCollection.clear();
			QVariantMap bookmarksMap = map.value("bookmarks").toMap();
			int i = 0;
			for (auto bookmarkKey : bookmarksMap.keys())
			{
				QVariantMap bookmarkData = bookmarksMap.value(bookmarkKey).toMap();
				bookmark bm;

				QString JDs = "";

				bm.name = bookmarkData.value("name").toString();
				QString nameI18n = bookmarkData.value("nameI18n").toString();
				if (!nameI18n.isEmpty())
					bm.nameI18n = nameI18n;
				QString JD = bookmarkData.value("jd").toString();
				if (!JD.isEmpty())
				{
					bm.jd = JD;
					JDs = StelUtils::julianDayToISO8601String(JD.toDouble() + core->getUTCOffset(JD.toDouble())/24.).replace("T", " ");
				}
				QString Location = bookmarkData.value("location").toString();
				if (!Location.isEmpty())
					bm.location = Location;
				QString RA = bookmarkData.value("ra").toString();
				if (!RA.isEmpty())
					bm.ra = RA;
				QString Dec = bookmarkData.value("dec").toString();
				if (!Dec.isEmpty())
					bm.dec = Dec;

				bm.isVisibleMarker = bookmarkData.value("isVisibleMarker", false).toBool();

				bookmarksCollection.insert(bookmarkKey, bm);
				addModelRow(i, bookmarkKey, bm.name, bm.nameI18n, JDs, Location);
				i++;
			}

		}
		catch (std::runtime_error &e)
		{
			qDebug() << "[Bookmarks] File format is wrong! Error: " << e.what();
			return;
		}

	}
}

void BookmarksDialog::importBookmarks()
{
	QString originalBookmarksFile = bookmarksJsonPath;

	QString filter = "JSON (*.json)";
	bookmarksJsonPath = QFileDialog::getOpenFileName(0, q_("Import bookmarks"), QDir::homePath(), filter);

	loadBookmarks();

	bookmarksJsonPath = originalBookmarksFile;
	saveBookmarks();
}

void BookmarksDialog::exportBookmarks()
{
	QString originalBookmarksFile = bookmarksJsonPath;

	QString filter = "JSON (*.json)";
	bookmarksJsonPath = QFileDialog::getSaveFileName(0, q_("Export bookmarks as..."), QDir::homePath() + "/bookmarks.json", filter);

	saveBookmarks();

	bookmarksJsonPath = originalBookmarksFile;
}

void BookmarksDialog::saveBookmarks()
{
	if (bookmarksJsonPath.isEmpty())
	{
		qWarning() << "[Bookmarks] Error saving bookmarks";
		return;
	}
	QFile jsonFile(bookmarksJsonPath);
	if(!jsonFile.open(QFile::WriteOnly|QFile::Text))
	{
		qWarning() << "[Bookmarks] bookmarks can not be saved. A file can not be open for writing:"
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
	    if (!sp.nameI18n.isEmpty())
		    bm.insert("nameI18n", sp.nameI18n);
	    if (!sp.ra.isEmpty())
		    bm.insert("ra", sp.ra);
	    if (!sp.dec.isEmpty())
		    bm.insert("dec", sp.dec);
	    if (!sp.jd.isEmpty())
		    bm.insert("jd", sp.jd);
	    if (!sp.location.isEmpty())
		    bm.insert("location", sp.location);
	    if (sp.isVisibleMarker)
		    bm.insert("isVisibleMarker", sp.isVisibleMarker);

	    bookmarksDataList.insert(i.key(), bm);
	}

	QVariantMap bmList;
	bmList.insert("bookmarks", bookmarksDataList);

	//Convert the tree to JSON
	StelJsonParser::write(bmList, &jsonFile);
	jsonFile.flush();
	jsonFile.close();

}

