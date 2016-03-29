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

#include "BookmarksDialog.hpp"
#include "ui_bookmarksDialog.h"

BookmarksDialog::BookmarksDialog(QObject *parent)
	: StelDialog(parent)
{
	dialogName = "Bookmarks";
	ui = new Ui_bookmarksDialogForm;
	core = StelApp::getInstance().getCore();
	objectMgr = GETSTELMODULE(StelObjectMgr);
}

BookmarksDialog::~BookmarksDialog()
{
	delete ui;
}

void BookmarksDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setBookmarksHeaderNames();
		populateBookmarksList();
		currentBookmarks();
	}
}

void BookmarksDialog::createDialogContent()
{
	ui->setupUi(dialog);

#ifdef Q_OS_WIN
	// Kinetic scrolling for tablet pc and pc
	QList<QWidget *> addscroll;
	addscroll << ui->bookmarksTreeWidget;
	installKineticScrolling(addscroll);
#endif
	
	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	initListBookmarks();
	populateBookmarksList();

	/*
	double JD = core->getJD() + StelUtils::getGMTShiftFromQT(core->getJD())/24;
	ui->dateFromDateTimeEdit->setDateTime(StelUtils::jdToQDateTime(JD));
	*/

	// bug #1350669 (https://bugs.launchpad.net/stellarium/+bug/1350669)
	connect(ui->bookmarksTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		ui->bookmarksTreeWidget, SLOT(repaint()));

	connect(ui->bookmarksTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentBookmark(QModelIndex)));

}

void BookmarksDialog::initListBookmarks()
{
	ui->bookmarksTreeWidget->clear();
	ui->bookmarksTreeWidget->setColumnCount(ColumnCount);
	setBookmarksHeaderNames();
	ui->bookmarksTreeWidget->header()->setSectionsMovable(false);
}

void BookmarksDialog::setBookmarksHeaderNames()
{
	QStringList headerStrings;
	//TRANSLATORS: name of bookmark
	headerStrings << q_("Name");
	//TRANSLATORS: description of bookmark
	//headerStrings << q_("Description");
	//TRANSLATORS: right ascension
	headerStrings << q_("RA (J2000)");
	//TRANSLATORS: declination
	headerStrings << q_("Dec (J2000)");
	//TRANSLATORS: date and time
	headerStrings << q_("Date and Time");
	//TRANSLATORS: location
	headerStrings << q_("Location");
	ui->bookmarksTreeWidget->setHeaderLabels(headerStrings);

	// adjust the column width
	for(int i = 0; i < ColumnCount; ++i)
	{
	    ui->bookmarksTreeWidget->resizeColumnToContents(i);
	}
}

void BookmarksDialog::currentBookmarks()
{
	float ra, dec;

	initListBookmarks();

	/*
	double JD = StelApp::getInstance().getCore()->getJD();
	ui->positionsTimeLabel->setText(q_("Positions on %1").arg(StelUtils::jdToQDateTime(JD + StelUtils::getGMTShiftFromQT(JD)/24).toString("yyyy-MM-dd hh:mm:ss")));

	foreach (const PlanetP& planet, allPlanets)
	{
		if (planet->getPlanetType()!=Planet::isUNDEFINED && planet->getEnglishName()!="Sun" && planet->getEnglishName()!=core->getCurrentPlanet()->getEnglishName())
		{
			StelUtils::rectToSphe(&ra,&dec,planet->getJ2000EquatorialPos(core));
			ACTreeWidgetItem *treeItem = new ACTreeWidgetItem(ui->planetaryPositionsTreeWidget);
			treeItem->setText(ColumnName, planet->getNameI18n());
			treeItem->setText(ColumnRA, StelUtils::radToHmsStr(ra));
			treeItem->setTextAlignment(ColumnRA, Qt::AlignRight);
			treeItem->setText(ColumnDec, StelUtils::radToDmsStr(dec, true));
			treeItem->setTextAlignment(ColumnDec, Qt::AlignRight);
			treeItem->setText(ColumnMagnitude, QString::number(planet->getVMagnitudeWithExtinction(core), 'f', 2));
			treeItem->setTextAlignment(ColumnMagnitude, Qt::AlignRight);
			treeItem->setText(ColumnType, q_(planet->getPlanetTypeString()));
		}
	}
	*/

	// adjust the column width
	for(int i = 0; i < ColumnCount; ++i)
	{
	    ui->bookmarksTreeWidget->resizeColumnToContents(i);
	}

	// sort-by-name
	ui->bookmarksTreeWidget->sortItems(ColumnName, Qt::AscendingOrder);
}

void BookmarksDialog::selectCurrentBookmark(const QModelIndex &modelIndex)
{
	// Find the object
	QString name = modelIndex.sibling(modelIndex.row(), ColumnName).data().toString();

	/*
	if (objectMgr->findAndSelectI18n(nameI18n) || objectMgr->findAndSelect(nameI18n))
	{
		const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
		if (!newSelected.empty())
		{
			// Can't point to home planet
			if (newSelected[0]->getEnglishName()!=core->getCurrentLocation().planetName)
			{
				StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
				mvmgr->moveToObject(newSelected[0], mvmgr->getAutoMoveDuration());
				mvmgr->setFlagTracking(true);
			}
			else
			{
				GETSTELMODULE(StelObjectMgr)->unSelect();
			}
		}
	}
	*/
}

void BookmarksDialog::populateBookmarksList()
{

}
