/*
 * Stellarium Location List Editor
 * Copyright (C) 2012  Bogdan Marinov
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
 * Foundation, Inc., 51 Franklin Street,
 * Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "LocationListEditor.hpp"
#include "ui_LocationListEditor.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include "kfilterdev.h" // For compressing binary lists

#include "LocationListModel.hpp"

LocationListEditor::LocationListEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::LocationListEditor),
    locations(0),
    proxyModel(0)
{
	ui->setupUi(this);
	setWindowIcon(QIcon(":/locationListEditor/icon.bmp"));
	
	connect(ui->actionOpenFile, SIGNAL(triggered()),
	        this, SLOT(open()));
	connect(ui->actionOpenProject, SIGNAL(triggered()),
	        this, SLOT(openProjectLocations()));
	connect(ui->actionOpenUser, SIGNAL(triggered()),
	        this, SLOT(openUserLocations()));
	connect(ui->actionSave, SIGNAL(triggered()),
	        this, SLOT(save()));
	connect(ui->actionSaveAs, SIGNAL(triggered()),
	        this, SLOT(saveAs()));
	connect(ui->actionAddNew, SIGNAL(triggered()),
	        this, SLOT(addNew()));
	connect(ui->actionInsertBefore, SIGNAL(triggered()),
	        this, SLOT(insertBefore()));
	connect(ui->actionInsertAfter, SIGNAL(triggered()),
	        this, SLOT(insertAfter()));
	connect(ui->actionCloneSelected, SIGNAL(triggered()),
	        this, SLOT(cloneSelected()));
	connect(ui->actionDelete, SIGNAL(triggered()),
	        this, SLOT(deleteSelected()));
	connect(ui->actionGoToRow, SIGNAL(triggered()),
	        this, SLOT(goToRow()));
	connect(ui->actionNextDuplicate, SIGNAL(triggered()),
	        this, SLOT(goToNextDuplicate()));
	connect(ui->actionExit, SIGNAL(triggered()),
	        this, SLOT(close()));
	connect(ui->actionAbout, SIGNAL(triggered()),
	        this, SLOT(showAboutWindow()));
	connect(ui->actionTest, SIGNAL(triggered()),
	        this, SLOT(test()));
	
	connect(ui->comboBoxFilteredColumn, SIGNAL(currentIndexChanged(int)),
	        this, SLOT(setFilteredColumn(int)));
	
	// Base location list
	QFileInfo projectList(QDir::current(), "../../data/base_locations.txt");
	if (projectList.exists())
		projectFilePath = projectList.absoluteFilePath();
	else
		ui->actionOpenProject->setEnabled(false);
	
	// User location list
	// TODO: What happens if a user file is created while the program is run?
	// TODO: If no file exists, create one?
	QString path = QDir::homePath();
#if defined(Q_OS_WIN)
	// As there is no way to know which version of Windows it is, try both
	path += "\\AppData\\Roaming\\Stellarium\\data\\user_locations.txt";
	QFileInfo fileInfo(path);
	if (!fileInfo.exists())
		path = QDir::homePath() + "\\Application Data\\Stellarium\\data\\user_locations.txt";
#elif defined(Q_OS_MAC)
	path += "/Library/Application Support/Stellarium/data/user_locations.txt";
#else
	path += "/.stellarium/data/user_locations.txt";
#endif
	QFileInfo userList(path);
	//qDebug() << userList.absoluteFilePath() << QDir::homePath();
	if (userList.exists())
		userFilePath = userList.absoluteFilePath();
	else
		ui->actionOpenUser->setEnabled(false);
	
	// Status bar and enabled menus.
	if (checkIfFileIsLoaded())
		ui->statusBar->showMessage("Ready.");
	else
	{
		setWindowFilePath("No list");
		ui->statusBar->showMessage("No location list is loaded.");
	}
	
	fileFilters = "Stellarium location lists (*locations.txt);;All files (*.*)";
}

LocationListEditor::~LocationListEditor()
{
	delete ui;
}

void LocationListEditor::changeEvent(QEvent* event)
{
	QMainWindow::changeEvent(event);
	switch (event->type()) {
		case QEvent::LanguageChange:
			ui->retranslateUi(this);
			break;
		default:
			break;
	}
}

void LocationListEditor::closeEvent(QCloseEvent* event)
{
	if (checkIfFileIsSaved())
	{
		event->accept();
	}
	else // "Cancel" has been chosen.
		event->ignore();
}

bool LocationListEditor::checkIfFileIsLoaded()
{
	Q_ASSERT(ui);
	
	bool enabled = (locations != 0);
	ui->menuEdit->setEnabled(enabled);
	ui->menuNavigation->setEnabled(enabled);
	ui->actionSave->setEnabled(enabled);
	ui->actionSaveAs->setEnabled(enabled);
	ui->actionBinary->setEnabled(enabled);
	ui->actionNativeEolChars->setEnabled(enabled);
	ui->tableView->setVisible(enabled);
	ui->frameFilter->setVisible(enabled);
	
	return enabled;
}

bool LocationListEditor::checkIfFileIsSaved()
{
	if (locations && locations->isModified())
	{
		QString message = "The list has been modified.\n"
		                  "Do you want to save your changes?";
		QMessageBox::StandardButton answer;
		answer = QMessageBox::warning(this, "Location List Editor",
		                              message,
		                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		if (answer == QMessageBox::Save)
			return save();
		else if (answer == QMessageBox::Cancel)
			return false;
	}
	return true;
}

bool LocationListEditor::loadFile(const QString& path)
{
	if (path.isEmpty())
		return false;
	
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::warning(this, 
		                     qApp->applicationName(),
		                     QString("Cannot open file %1:\n%2.")
		                     .arg(path)
		                     .arg(file.errorString()));
		return false;
	}
	LocationListModel* newLocations = LocationListModel::load(&file);
	file.close();
	
	if (newLocations->rowCount() < 1)
	{
		QMessageBox::warning(this, 
		                     qApp->applicationName(),
		                     QString("Error reading file %1").arg(path));
		return false;
	}
	
	// Something has been read
	newLocations->setParent(this);
	openFilePath = path;
	setWindowFilePath(path);
	setWindowModified(false);
	
	
	QSortFilterProxyModel* newProxy = new QSortFilterProxyModel(this);
	newProxy->setSourceModel(newLocations);
	ui->tableView->setModel(newProxy);
	
	if (locations)
	{
		delete locations;
	}
	locations = newLocations;
	connect(locations, SIGNAL(modified(bool)),
	        this, SLOT(setWindowModified(bool)));
	if (proxyModel)
	{
		delete proxyModel;
	}
	proxyModel = newProxy;
	connect(ui->lineEditFilter, SIGNAL(textEdited(QString)),
	        proxyModel, SLOT(setFilterWildcard(QString)));
	setFilterCaseSensitivity(false);
	connect(ui->checkBoxCaseSensitive, SIGNAL(toggled(bool)),
	        this, SLOT(setFilterCaseSensitivity(bool)));
	
	if (checkIfFileIsLoaded())
		ui->statusBar->showMessage("Loaded " + path);
	
	if (!locations->loadingLog.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.setWindowTitle(qApp->applicationName());
		msgBox.setText(path + " was loaded succsessfully, but there were some errors.");
		msgBox.setInformativeText("Click \"Show details\" to see the log or \"Save\" to save it to file.");
		msgBox.setDetailedText(locations->loadingLog);
		msgBox.setStandardButtons(QMessageBox::Save|QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.setEscapeButton(QMessageBox::Ok);
		msgBox.setIcon(QMessageBox::Information);
		
		int ret = msgBox.exec();
		if (ret == QMessageBox::Save)
		{
			QString path = QFileDialog::getSaveFileName(this,
			                                            "Save file as...",
			                                            "log.txt");
			
			QFile file(path);
			if (file.open(QFile::WriteOnly|QFile::Text))
			{
				QTextStream out(&file);
				out << locations->loadingLog;
				file.close();
			}
			else
			{
				QMessageBox::warning(this,
				                     qApp->applicationName(),
				                     QString("Cannot save to file %1:\n%2. Dumping log contents to stderr.")
				                     .arg(path)
				                     .arg(file.errorString()));
				qWarning() << locations->loadingLog;
			}
		}
	}

	ui->comboBoxFilteredColumn->clear();
	for (int i = 0; i < locations->columnCount(); i++)
	{
		QString column = locations->headerData(i, Qt::Horizontal).toString();
		ui->comboBoxFilteredColumn->addItem(column);
	}
	
	return true;
}

bool LocationListEditor::saveFile(const QString& path)
{
	if (path.isEmpty())
		return false;
	
	QFile::OpenMode flags = QFile::WriteOnly;
	if (ui->actionNativeEolChars->isChecked())
		flags |= QFile::Text;
	// QFile::Text saves with Windows line-endings on Windows,
	// which creates artificial differences when diff-ing base_locations.txt
		
	QFile file(path);
	if (!file.open(flags))
	{
		QMessageBox::warning(this,
		                     qApp->applicationName(),
		                     QString("Cannot save to file %1:\n%2.")
		                     .arg(path)
		                     .arg(file.errorString()));
		return false;
	}
	bool ok = locations->save(&file);
	file.close();
	
	if (!ok)
	{
		ui->statusBar->showMessage("Error saving list!");
		return false;
	}
	
	locations->setModified(false);
	
	QString message;
	
	// Save the list also in binary format if necessary.
	if (ui->actionBinary->isChecked())
	{
		if (saveBinary(path))
			message = "List saved, with binary, to " + path;
		else
			message = "Error saving as binary! But list saved to " + path;
	}
	else
	{
		message = "List saved to " + path;
	}
	
	ui->statusBar->showMessage(message);
	return true;
}

bool LocationListEditor::saveBinary(const QString& path)
{
	if (path.isEmpty())
		return false;
	
	QFileInfo fileInfo(path);
	QString binPath = fileInfo.path();
	binPath += '/';
	binPath += fileInfo.baseName();
	binPath += ".bin.gz";
	qDebug() << "Binary path:" << binPath;
	
	QFile file(binPath);
	if (!file.open(QFile::WriteOnly))
	{
		QMessageBox::warning(this,
		                     qApp->applicationName(),
		                     QString("Cannot save to binary file %1:\n%2.")
		                     .arg(path)
		                     .arg(file.errorString()));
		return false;
	}
	QIODevice* device = KFilterDev::device(&file, "application/x-gzip", false);
	device->open(QIODevice::WriteOnly);
	bool ok = locations->saveBinary(device);
	device->close();
	delete device;
	file.close();
	
	return ok;
}

QList<int> LocationListEditor::getIndexesOfSelection()
{
	QList<int> rows;
	
	QItemSelectionModel* selectionModel = ui->tableView->selectionModel();
	if (!selectionModel->hasSelection())
		return rows;
	
	QModelIndexList list = selectionModel->selectedRows();
	foreach (const QModelIndex& index, list)
		rows.append(proxyModel->mapToSource(index).row());
	qSort(rows);
	
	return rows;
}

int LocationListEditor::getIndexOfCurrentRow()
{
	QItemSelectionModel* selectionModel = ui->tableView->selectionModel();
	QModelIndex index = selectionModel->currentIndex();
	if (index.isValid())
		return index.row();
	else
		return -1;
}

void LocationListEditor::setCurrentlyEditedLocation(int row)
{
	QModelIndex index = locations->index(row, 0);
	if (!index.isValid())
		return;
	QModelIndex proxyIndex = proxyModel->mapFromSource(index);
	//QItemSelectionModel* selectionModel = ui->tableView->selectionModel();
	//selectionModel->setCurrentIndex(proxyIndex,
	//                                QItemSelectionModel::NoUpdate);
	ui->tableView->setCurrentIndex(proxyIndex);
	ui->tableView->edit(proxyIndex);
}

void LocationListEditor::goToRow(int row)
{
	QModelIndex index = locations->index(row, 0);
	if (!index.isValid())
		return;
	QModelIndex proxyIndex = proxyModel->mapFromSource(index);
	ui->tableView->setCurrentIndex(proxyIndex);
	ui->tableView->scrollTo(proxyIndex, QTableView::PositionAtCenter);	
}


void LocationListEditor::open()
{
	if (checkIfFileIsSaved())
	{
		QString filePath = QFileDialog::getOpenFileName(this,
		                                                "Open file...",
		                                                QString(),
		                                                fileFilters);
		if (!filePath.isEmpty())
			loadFile(filePath);
	}
}

void LocationListEditor::openProjectLocations()
{
	if (checkIfFileIsSaved())
	{
		if (loadFile(projectFilePath))
		{
			ui->actionBinary->setChecked(true);
			ui->actionNativeEolChars->setChecked(false);
		}
	}
}

void LocationListEditor::openUserLocations()
{
	if (checkIfFileIsSaved())
	{
		if (loadFile(userFilePath))
		{
			ui->actionBinary->setChecked(false);
			ui->actionNativeEolChars->setChecked(true);
		}
	}
}

bool LocationListEditor::save()
{
	if (openFilePath.isEmpty())
		return saveAs(); //Really unlikely
	else
		return saveFile(openFilePath);
}

bool LocationListEditor::saveAs()
{
	QString path = QFileDialog::getSaveFileName(this,
	                                            "Save file as...",
	                                            openFilePath,
	                                            fileFilters);
	if (path.isEmpty())
		return false;
	
	return saveFile(path);
}


void LocationListEditor::addNew()
{
	Location* loc = new Location();
	loc->stelName = loc->name = "New Location";
	locations->insertLocation(locations->rowCount(), loc);
	setCurrentlyEditedLocation(locations->rowCount() - 1);
}

void LocationListEditor::insertBefore()
{
	int row = getIndexOfCurrentRow();
	if (row < 0)
		return;

	Location* loc = new Location();
	loc->stelName = loc->name = "New Location";
	locations->insertLocation(row, loc);
	setCurrentlyEditedLocation(row);
}

void LocationListEditor::insertAfter()
{
	int row = getIndexOfCurrentRow();
	if (row < 0)
		return;
	row += 1;
	
	Location* loc = new Location();
	loc->stelName = loc->name = "New Location";
	locations->insertLocation(row, loc);
	setCurrentlyEditedLocation(row);
}

void LocationListEditor::cloneSelected()
{
	QList<int> rows = getIndexesOfSelection();
	if (rows.isEmpty())
		return;
	
	bool appendAtEnd = ui->actionClonedAtTheEnd->isChecked();
	QList<int>::iterator i = rows.end();
	while (i != rows.begin())
		locations->cloneLocation(*(--i), appendAtEnd);
}

void LocationListEditor::deleteSelected()
{
	QList<int> rows = getIndexesOfSelection();
	if (rows.isEmpty())
		return;
	
	QList<int>::iterator i = rows.end();
	while (i != rows.begin())
		locations->removeLocation(*(--i));
}

void LocationListEditor::goToRow()
{
	bool ok = false;
	int row = QInputDialog::getInt(this,
	                               "Go to row...",
	                               "Row number:",
	                               1, 0, locations->rowCount(), 1,
	                               &ok);
	row -= 1; // Visible rows start counting from 1.
	if (ok && row >= 0 && row < locations->rowCount())
	{
		goToRow(row);
	}
}

void LocationListEditor::goToNextDuplicate()
{
	int currentRow = getIndexOfCurrentRow();
	bool wrapAround = ui->actionDuplicateWrapAround->isChecked();
	int duplicateRow = locations->findNextDuplicateRow(currentRow, wrapAround);
	if (duplicateRow >= 0)
		goToRow(duplicateRow);
}


void LocationListEditor::showAboutWindow()
{
	QString s;
	s += "<h1>Stellarium Location List Editor</h1>";
	s += QString("Version: %1<br/>").arg(qApp->applicationVersion());
	s += "Copyright (c) 2012 Bogdan Marinov (and others)<br/><br/>";
	s += "The <b>Location List Editor</b> is a tool created to ease the maintenance of Stellarium location list files - both the base location file and user-defined lists of locations.";
	QMessageBox::about(this, "About the program", s);
}

void LocationListEditor::test()
{
	//
}

void LocationListEditor::setFilteredColumn(int column)
{
	if (proxyModel && column >= 0 && column < proxyModel->columnCount())
		proxyModel->setFilterKeyColumn(column);
}

void LocationListEditor::setFilterCaseSensitivity(bool sensitive)
{
	if (proxyModel)
	{
		if (sensitive)
			proxyModel->setFilterCaseSensitivity(Qt::CaseSensitive);
		else
			proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	}
}
