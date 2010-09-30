/*
 * Comet and asteroids importer plug-in for Stellarium
 *
 * Copyright (C) 2010 Bogdan Marinov
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "CAImporter.hpp"

#include "ImportWindow.hpp"
#include "ui_importWindow.h"

#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "SolarSystem.hpp"

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QList>
#include <QString>

ImportWindow::ImportWindow()
{
	ui = new Ui_importWindow();
	ssoManager = GETSTELMODULE(CAImporter);
}

ImportWindow::~ImportWindow()
{
	delete ui;
}

void ImportWindow::createDialogContent()
{
	ui->setupUi(dialog);

	//Signals
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(ui->pushButtonParse, SIGNAL(clicked()), this, SLOT(parseElements()));
	connect(ui->pushButtonAdd, SIGNAL(clicked()), this, SLOT(addObjects()));

	connect(ui->pushButtonPaste, SIGNAL(clicked()), this, SLOT(pasteClipboard()));
	connect(ui->pushButtonBrowse, SIGNAL(clicked()), this, SLOT(selectFile()));
	connect(ui->comboBoxBookmarks, SIGNAL(currentIndexChanged(QString)), this, SLOT(bookmarkSelected(QString)));

	connect(ui->radioButtonSingle, SIGNAL(toggled(bool)), ui->frameSingle, SLOT(setVisible(bool)));
	connect(ui->radioButtonFile, SIGNAL(toggled(bool)), ui->frameFile, SLOT(setVisible(bool)));
	connect(ui->radioButtonURL, SIGNAL(toggled(bool)), ui->frameURL, SLOT(setVisible(bool)));

	ui->radioButtonSingle->setChecked(true);
	ui->frameFile->setVisible(false);
	ui->frameURL->setVisible(false);
}

void ImportWindow::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void ImportWindow::parseElements()
{
	if (ui->radioButtonSingle->isChecked())
	{
		QString oneLine = ui->lineEditSingle->text();
		//TODO: Move this to the function itself:
		oneLine = oneLine.trimmed();
		if (oneLine.isEmpty())
			return;

		CAImporter::SsoElements object = ssoManager->readMpcOneLineCometElements(oneLine);

		if (object.isEmpty())
			return;

		QList<CAImporter::SsoElements> objects;
		objects << object;
		//Temporary, until the slot/socket mechanism is ready
		populateCandidateObjects(objects);
	}
	else if (ui->radioButtonFile->isChecked())
	{
		QString filePath = ui->lineEditFilePath->text();
		if (filePath.isEmpty())
			return;

		QList<CAImporter::SsoElements> objects = ssoManager->readMpcOneLineCometElementsFromFile(filePath);
		if (objects.isEmpty())
			return;

		//Temporary, until the slot/socket mechanism is ready
		populateCandidateObjects(objects);
	}
	else if (ui->radioButtonURL->isChecked())
	{
		//TODO: Verify and download
	}
	ui->stackedWidget->setCurrentIndex(1);
	//close();
}

void ImportWindow::addObjects()
{
	QList<QString> checkedObjectsNames;

	//Extract the marked objects
	while (ui->listWidgetObjects->count() > 0)
	{
		QListWidgetItem * item = ui->listWidgetObjects->takeItem(0);
		if (item->checkState() == Qt::Checked)
		{
			checkedObjectsNames.append(item->text());
		}
		delete item;
	}
	qDebug() << "Checked:" << checkedObjectsNames;

	QList<CAImporter::SsoElements> approvedObjects;
	for (int i = 0; i < candidateObjects.count(); i++)
	{
		QString name = candidateObjects.at(i).value("name").toString();
		if (checkedObjectsNames.contains(name))
			approvedObjects.append(candidateObjects.at(i));
	}

	//Write to file
	ssoManager->appendToSolarSystemConfigurationFile(approvedObjects);

	//Refresh the Solar System
	GETSTELMODULE(SolarSystem)->reloadPlanets();

	ui->stackedWidget->setCurrentIndex(0);
	close();
}

void ImportWindow::pasteClipboard()
{
	ui->lineEditSingle->setText(QApplication::clipboard()->text());
}

void ImportWindow::selectFile()
{
	QString filePath = QFileDialog::getOpenFileName(NULL, "Select a text file", StelFileMgr::getDesktopDir());
	ui->lineEditFilePath->setText(filePath);
}

void ImportWindow::bookmarkSelected(QString bookmarkTitle)
{
	//TODO: Lookup bookmark
	QString bookmarkUrl = bookmarkTitle;
	ui->lineEditURL->setText(bookmarkUrl);
}

void ImportWindow::populateCandidateObjects(QList<CAImporter::SsoElements> objects)
{
	candidateObjects.clear();

	QListWidget * list = ui->listWidgetObjects;
	list->clear();
	foreach (CAImporter::SsoElements object, objects)
	{
		if (object.contains("name") && !object.value("name").toString().isEmpty())
		{
			QListWidgetItem * item = new QListWidgetItem(list);
			item->setText(object.value("name").toString());
			item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
			item->setCheckState(Qt::Unchecked);

			candidateObjects << object;
		}
	}

	//Select the first item
	if (list->count() > 0)
		list->setCurrentRow(0);
}
