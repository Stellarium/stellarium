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
#include "HighlightMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelJsonParser.hpp"
#include "AngleSpinBox.hpp"
#include "NebulaMgr.hpp"
#include "StarMgr.hpp"
#include "LabelMgr.hpp"

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
	labelMgr = GETSTELMODULE(LabelMgr);
	bookmarksListModel = new QStandardItemModel(0, ColumnCount);
	bookmarksJsonPath = StelFileMgr::findFile("data", static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable)) + "/bookmarks.json";
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

	connect(ui->clearHighlightsButton, SIGNAL(clicked()), this, SLOT(clearHighlightsButtonPressed()));
	connect(ui->highlightBookmarksButton, SIGNAL(clicked()), this, SLOT(highlightBookrmarksButtonPressed()));

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

	QString style = "QCheckBox { color: rgb(238, 238, 238); }";
	ui->dateTimeCheckBox->setStyleSheet(style);
	ui->locationCheckBox->setStyleSheet(style);

	loadBookmarks();
}

void BookmarksDialog::setBookmarksHeaderNames()
{
	const QStringList headerStrings = {
		"UUID", // Hide the column
		q_("Object"),
		q_("Localized name"),
		q_("Date and Time"),
		q_("Location of observer")};

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
		double fov = -1.0;

		if (name.isEmpty() || selected[0]->getType()=="CustomObject")
		{
			double ra, dec;
			StelUtils::rectToSphe(&ra, &dec, selected[0]->getJ2000EquatorialPos(core));
			raStr = StelUtils::radToHmsStr(ra, false).trimmed();
			decStr = StelUtils::radToDmsStr(dec, false).trimmed();
			if (name.contains("marker", Qt::CaseInsensitive))
				visibleFlag = true;

			if (name.isEmpty())
			{
				name = QString("%1, %2").arg(raStr, decStr);
				nameI18n = q_("Unnamed star");
				fov = GETSTELMODULE(StelMovementMgr)->getCurrentFov();
			}
		}

		const bool dateTimeFlag = ui->dateTimeCheckBox->isChecked();
		const bool locationFlag = ui->locationCheckBox->isChecked();

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
		if (fov > 0.0)
			bm.fov = fov;

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
	GETSTELMODULE(HighlightMgr)->cleanHighlightList();
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

void BookmarksDialog::highlightBookrmarksButtonPressed()
{
	QList<Vec3d> highlights;
	highlights.clear();
	clearHighlightsButtonPressed(); // Enable fool protection
	int fontSize = StelApp::getInstance().getScreenFontSize();
	HighlightMgr* hlMgr = GETSTELMODULE(HighlightMgr);
	QString color = hlMgr->getColor().toHtmlColor();
	float distance = hlMgr->getMarkersSize();

	for (auto bm : bookmarksCollection)
	{
		QString name	= bm.name;
		QString raStr	= bm.ra.trimmed();
		QString decStr	= bm.dec.trimmed();

		Vec3d pos;
		bool status = false;
		if (!raStr.isEmpty() && !decStr.isEmpty())
		{
			StelUtils::spheToRect(StelUtils::getDecAngle(raStr), StelUtils::getDecAngle(decStr), pos);
			status = true;
		}
		else
		{
			status = objectMgr->findAndSelect(name);
			const QList<StelObjectP>& selected = objectMgr->getSelectedObject();
			if (!selected.isEmpty())
				pos = selected[0]->getJ2000EquatorialPos(core);
		}

		if (status)
			highlights.append(pos);

		objectMgr->unSelect();
		// Add labels for named highlights (name in top right corner)
		highlightLabelIDs.append(labelMgr->labelObject(name, name, true, fontSize, color, "NE", distance));
	}

	hlMgr->fillHighlightList(highlights);
}

void BookmarksDialog::clearHighlightsButtonPressed()
{
	objectMgr->unSelect();
	GETSTELMODULE(HighlightMgr)->cleanHighlightList();
	// Clear labels
	for (auto l : highlightLabelIDs)
	{
		labelMgr->deleteLabel(l);
	}
	highlightLabelIDs.clear();
}

void BookmarksDialog::selectCurrentBookmark(const QModelIndex &modelIdx)
{
	goToBookmark(modelIdx.sibling(modelIdx.row(), ColumnUUID).data().toString());
}

void BookmarksDialog::goToBookmark(QString uuid)
{
	const bool dateTimeFlag = ui->dateTimeCheckBox->isChecked();
	const bool locationFlag = ui->locationCheckBox->isChecked();

	if (!uuid.isEmpty())
	{
		bookmark bm = bookmarksCollection.value(uuid);
		if (!bm.jd.isEmpty() && dateTimeFlag)
		{
			core->setJD(bm.jd.toDouble());
		}
		if (!bm.location.isEmpty() && locationFlag)
		{
			StelLocationMgr* locationMgr = &StelApp::getInstance().getLocationMgr();
			core->moveObserverTo(locationMgr->locationForString(bm.location));
		}

		StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
		objectMgr->unSelect();

		bool status = objectMgr->findAndSelect(bm.name);
		float amd = mvmgr->getAutoMoveDuration();
		if (!bm.ra.isEmpty() && !bm.dec.isEmpty() && !status)
		{
			Vec3d pos;
			StelUtils::spheToRect(StelUtils::getDecAngle(bm.ra.trimmed()), StelUtils::getDecAngle(bm.dec.trimmed()), pos);
			if (bm.name.contains("marker", Qt::CaseInsensitive))
			{
				// Add a custom object on the sky
				GETSTELMODULE(CustomObjectMgr)->addCustomObject(bm.name, pos, bm.isVisibleMarker);
				status = objectMgr->findAndSelect(bm.name);
			}
			else
			{
				// The unnamed stars
				StelObjectP sobj;
				const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
				double fov = 5.0;
				if (bm.fov > 0.0)
					fov = bm.fov;

				mvmgr->zoomTo(fov, 0.0);
				mvmgr->moveToJ2000(pos, mvmgr->mountFrameToJ2000(Vec3d(0., 0., 1.)), 0.0);

				QList<StelObjectP> candidates = GETSTELMODULE(StarMgr)->searchAround(pos, 0.5, core);
				if (candidates.empty()) // The FOV is too big, let's reduce it
				{
					mvmgr->zoomTo(0.5*fov, 0.0);
					candidates = GETSTELMODULE(StarMgr)->searchAround(pos, 0.5, core);
				}

				Vec3d winpos;
				prj->project(pos, winpos);
				double xpos = winpos[0];
				double ypos = winpos[1];
				double best_object_value = 1000.;
				for (const auto& obj : candidates)
				{
					prj->project(obj->getJ2000EquatorialPos(core), winpos);
					double distance = std::sqrt((xpos-winpos[0])*(xpos-winpos[0]) + (ypos-winpos[1])*(ypos-winpos[1]));
					if (distance < best_object_value)
					{
						best_object_value = distance;
						sobj = obj;
					}
				}

				if (sobj)
					status = objectMgr->setSelectedObject(sobj);
			}
		}

		if (status)
		{
			const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
			if (!newSelected.empty())
			{
				mvmgr->moveToObject(newSelected[0], amd);
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
				double fov = bookmarkData.value("fov").toDouble();
				if (fov > 0.0)
					bm.fov = fov;

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
	bookmarksJsonPath = QFileDialog::getOpenFileName(Q_NULLPTR, q_("Import bookmarks"), QDir::homePath(), filter);

	loadBookmarks();
	ui->bookmarksTreeView->hideColumn(ColumnUUID);

	bookmarksJsonPath = originalBookmarksFile;
	saveBookmarks();
}

void BookmarksDialog::exportBookmarks()
{
	QString originalBookmarksFile = bookmarksJsonPath;

	QString filter = "JSON (*.json)";
	bookmarksJsonPath = QFileDialog::getSaveFileName(Q_NULLPTR,
							 q_("Export bookmarks as..."),
							 QDir::homePath() + "/bookmarks.json",
							 filter);

	saveBookmarks();

	bookmarksJsonPath = originalBookmarksFile;
}

void BookmarksDialog::saveBookmarks() const
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
	    bm.insert("name", sp.name);
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
	    if (sp.fov > 0.0)
		    bm.insert("fov", sp.fov);

	    bookmarksDataList.insert(i.key(), bm);
	}

	QVariantMap bmList;
	bmList.insert("bookmarks", bookmarksDataList);

	//Convert the tree to JSON
	StelJsonParser::write(bmList, &jsonFile);
	jsonFile.flush();
	jsonFile.close();
}

