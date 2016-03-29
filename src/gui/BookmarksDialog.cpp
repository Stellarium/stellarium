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

	connect(ui->currentObjectPushButton, SIGNAL(clicked()), this, SLOT(getCurrentObjectInfo()));
	connect(ui->centerScreenPushButton, SIGNAL(clicked()), this, SLOT(getCenterInfo()));

	connect(ui->addBookmarkButton, SIGNAL(clicked()), this, SLOT(addBookmarkButtonPressed()));
	connect(ui->removeBookmarkButton, SIGNAL(clicked()), this, SLOT(removeBookmarkButtonPressed()));
	connect(ui->goToButton, SIGNAL(clicked()), this, SLOT(goToBookmarkButtonPressed()));

	//Initializing the list of bookmarks
	bookmarksListModel->setColumnCount(ColumnCount);
	setBookmarksHeaderNames();

	ui->bookmarksTreeView->setModel(bookmarksListModel);
	ui->bookmarksTreeView->header()->setSectionsMovable(false);
	ui->bookmarksTreeView->header()->setSectionResizeMode(ColumnSlot, QHeaderView::ResizeToContents);
	ui->bookmarksTreeView->header()->setStretchLastSection(true);

	ui->raSpinBox->setDisplayFormat(AngleSpinBox::HMSLetters);
	ui->decSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);

	loadBookmarks();
}

void BookmarksDialog::setBookmarksHeaderNames()
{
	QStringList headerStrings;
	//TRANSLATORS: Symbol for "number"
	headerStrings << q_("#");
	//TRANSLATORS: name of bookmark
	headerStrings << q_("Name");	
	//TRANSLATORS: right ascension
	headerStrings << q_("RA (J2000)");
	//TRANSLATORS: declination
	headerStrings << q_("Dec (J2000)");
	//TRANSLATORS: date and time
	headerStrings << q_("Date and Time");
	//TRANSLATORS: location
	headerStrings << q_("Location");

	bookmarksListModel->setHorizontalHeaderLabels(headerStrings);
}

void BookmarksDialog::getCurrentObjectInfo()
{
	const QList<StelObjectP>& selected = objectMgr->getSelectedObject();
	if (!selected.isEmpty())
	{
		double dec_j2000 = 0;
		double ra_j2000 = 0;
		StelUtils::rectToSphe(&ra_j2000,&dec_j2000, selected[0]->getJ2000EquatorialPos(core));
		ui->raSpinBox->setRadians(ra_j2000);
		ui->decSpinBox->setRadians(dec_j2000);
		ui->bookmarkName->setText(selected[0]->getNameI18n());
	}
}

void BookmarksDialog::getCenterInfo()
{
	const StelProjectorP projector = core->getProjection(StelCore::FrameEquinoxEqu);
	Vec3d centerPosition;
	Vec2f center = projector->getViewportCenter();
	projector->unProject(center[0], center[1], centerPosition);
	double dec_j2000 = 0;
	double ra_j2000 = 0;
	StelUtils::rectToSphe(&ra_j2000,&dec_j2000,core->equinoxEquToJ2000(centerPosition));
	ui->raSpinBox->setRadians(ra_j2000);
	ui->decSpinBox->setRadians(dec_j2000);

	int lastRow = bookmarksListModel->rowCount();
	ui->bookmarkName->setText(q_("Bookmark %1").arg(lastRow+1));
	ui->bookmarkName->setFocus();
}

void BookmarksDialog::addModelRow(int number, QString name, QString RA, QString Dec, QString Date, QString Location)
{
	QStandardItem* tempItem = 0;

	tempItem = new QStandardItem(QString::number(number+1));
	tempItem->setEditable(false);
	bookmarksListModel->setItem(number, ColumnSlot, tempItem);

	tempItem = new QStandardItem(name);
	tempItem->setEditable(false);
	bookmarksListModel->setItem(number, ColumnName, tempItem);

	tempItem = new QStandardItem(RA);
	tempItem->setEditable(false);
	bookmarksListModel->setItem(number, ColumnRA, tempItem);

	tempItem = new QStandardItem(Dec);
	tempItem->setEditable(false);
	bookmarksListModel->setItem(number, ColumnDec, tempItem);

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
	double radiansRA  = ui->raSpinBox->valueRadians();
	double radiansDec = ui->decSpinBox->valueRadians();
	QString name      = ui->bookmarkName->text().trimmed();
	bool dateTimeFlag = ui->dateTimeCheckBox->isChecked();
	bool locationFlag = ui->locationCheckBox->isChecked();

	if (name.isEmpty())
		return;

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
		Location = QString("%1, %2").arg(loc.name, loc.country);
	}

	int lastRow = bookmarksListModel->rowCount();

	addModelRow(lastRow, name, ui->raSpinBox->text(), ui->decSpinBox->text(), JDs, Location);

	ui->raSpinBox->setRadians(0.);
	ui->decSpinBox->setRadians(0.);
	ui->bookmarkName->setText("");
	ui->bookmarkName->setFocus();

	QString JDv = "";
	if (!JDs.isEmpty())
		JDv = QString::number(JD, 'f', 6);

	bookmark bm;
	bm.number	= lastRow;
	bm.name		= name;
	bm.RA		= radiansRA;
	bm.Dec		= radiansDec;
	if (!JDs.isEmpty())
		bm.jd	= JD;

	if (!Location.isEmpty())
		bm.location = Location;

	QString key = QString::number(lastRow);
	bookmarksCollection.insert(key, bm);

	saveBookmarks();
}

void BookmarksDialog::removeBookmarkButtonPressed()
{
	int number = ui->bookmarksTreeView->currentIndex().row();
	bookmarksListModel->removeRow(number);

	QString key = QString::number(number);
	bookmarksCollection.remove(key);

	saveBookmarks();
}

void BookmarksDialog::goToBookmarkButtonPressed()
{
	int number = ui->bookmarksTreeView->currentIndex().row();

	bookmark bm = bookmarksCollection.value(QString::number(number));
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

	Vec3d pos;

	mvmgr->setViewUpVector(Vec3d(0., 0., 1.));
	Vec3d aimUp=mvmgr->getViewUpVectorJ2000();
	StelMovementMgr::MountMode mountMode=mvmgr->getMountMode();
	StelUtils::spheToRect(bm.RA, bm.Dec, pos);
	if ( (mountMode==StelMovementMgr::MountEquinoxEquatorial) && (fabs(bm.Dec)> (0.9*M_PI/2.0)) )
	{
		mvmgr->setViewUpVector(Vec3d(-cos(bm.RA), -sin(bm.RA), 0.) * (bm.Dec>0. ? 1. : -1. ));
		aimUp=mvmgr->getViewUpVectorJ2000();
	}

	mvmgr->setFlagTracking(false);
	mvmgr->moveToJ2000(pos, aimUp, 0.05);
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
	foreach(QString bookmarkKey, bookmarksMap.keys())
	{
		QVariantMap bookmarkData = bookmarksMap.value(bookmarkKey).toMap();
		bookmark bm;

		QString JDs = "";

		bm.number	= bookmarkKey.toInt();
		bm.name		= bookmarkData.value("name").toString();
		bm.RA		= bookmarkData.value("ra").toDouble();
		bm.Dec		= bookmarkData.value("dec").toDouble();
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
		addModelRow(bm.number, bm.name, StelUtils::radToHmsStr(bm.RA), StelUtils::radToDmsStr(bm.Dec), JDs, Location);
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
	    bm.insert("number", sp.number);
	    bm.insert("name",	sp.name);
	    bm.insert("ra",     sp.RA);
	    bm.insert("dec",    sp.Dec);
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
	jsonFile.flush();//Is this necessary?
	jsonFile.close();
}

