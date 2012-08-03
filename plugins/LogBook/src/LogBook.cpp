/*
 * Copyright (C) 2009 Timothy Reaves
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

#include "LogBook.hpp"
#include "LogBookCommon.hpp"
#include "LogBookConfigDialog.hpp"
#include "ObservationsDialog.hpp"
#include "SessionsDialog.hpp"
#include "TargetsDialog.hpp"

#include "stdexcept"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelObjectType.hpp"
#include "StelProjector.hpp"
#include "StelStyle.hpp"
#include "StelTranslator.hpp"

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QList>
#include <QListIterator>
#include <QString>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRelation>
#include <QSqlRelationalTableModel>
#include <QSqlTableModel>

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark StelModule Interface Methods
#endif
/* ********************************************************************* */
/*************************************************************************
 This method is the one called automatically by the StelModuleMgr just
 after loading the dynamic library
*************************************************************************/
StelModule* LogBookStelPluginInterface::getStelModule() const
{
	return new LogBook();
}

StelPluginInfo LogBookStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(LogBook);

	StelPluginInfo info;
	info.id = "LogBook";
	info.displayedName = "Log Book";
	info.authors = "Timothy Reaves";
	info.contact = "treaves@silverfieldstech.com";
	info.description = "A log book for visual observers.";
	return info;
}

Q_EXPORT_PLUGIN2(LogBook, LogBookStelPluginInterface)


/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Instance Methods
#endif
/* ********************************************************************* */
LogBook::LogBook()
{
	setObjectName("LogBook");
}

LogBook::~LogBook()
{
	delete configDialog;
	delete sessionsDialog;
	delete targetsDialog;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark StelModule Methods
#endif
/* ********************************************************************* */
bool LogBook::configureGui(bool)
{
	return true;
}

void LogBook::draw(StelCore* core, class StelRenderer* renderer)
{
}

double LogBook::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw) {
		return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName)+10.;
	}
	return 0;
}

const StelStyle LogBook::getModuleStyleSheet(const StelStyle& style)
{
	StelStyle pluginStyle(style);
	if (style.confSectionName == "color") {
		pluginStyle.qtStyleSheet.append(normalStyleSheet);
	} else {
		pluginStyle.qtStyleSheet.append(nightStyleSheet);
	}
	return pluginStyle;
}

void LogBook::init()
{
	qDebug() << "LogBook plugin - press Command-L to toggle Log Book view mode. Press ALT-L for configuration.";
	initializeDatabase();
	//Load the module's custom style sheets
	QFile styleSheetFile;
	styleSheetFile.setFileName(":/logbook/normalStyle.css");
	if(styleSheetFile.open(QFile::ReadOnly|QFile::Text)){
		normalStyleSheet = styleSheetFile.readAll();
	}
	styleSheetFile.close();
	styleSheetFile.setFileName(":/logbook/nightStyle.css");
	if(styleSheetFile.open(QFile::ReadOnly|QFile::Text)){
		nightStyleSheet = styleSheetFile.readAll();
	}
	styleSheetFile.close();

	configDialog = new LogBookConfigDialog(tableModels);
	sessionsDialog = new SessionsDialog(tableModels);
	targetsDialog = new TargetsDialog(tableModels);
	flagShowLogBook = false;
	initializeActions();
}

void LogBook::setStelStyle(const QString&)
{
	qDebug() << "====> Here.";
	configDialog->updateStyle();
	sessionsDialog->updateStyle();
	targetsDialog->updateStyle();
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Slots Methods
#endif
/* ********************************************************************* */
void LogBook::enableLogBook(bool b)
{
	sessionsDialog->setVisible(b);
	// Toggle the plugin on & off.  To toggle on, we want to ensure there is a selected object.
	if (!StelApp::getInstance().getStelObjectMgr().getWasSelected()) {
		qDebug() << "====> Nothing selected.";
	} else {
		QList<StelObjectP> selectedObjects = StelApp::getInstance().getStelObjectMgr().getSelectedObject();
		QListIterator<StelObjectP> objectIterator(selectedObjects);
		while (objectIterator.hasNext()) {
			StelObjectP stelObject = objectIterator.next();
			qDebug() << "====> Name: "<< stelObject->getNameI18n();
		}
	}
}

void LogBook::setConfigDialogVisible(bool b)
{
	configDialog->setVisible(b);
}

void LogBook::setTargetsDialogVisible(bool b)
{
	targetsDialog->setVisible(b);
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Private Methods
#endif
/* ********************************************************************* */

void LogBook::initializeActions()
{
	QString group = "LogBook";
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);

	gui->addGuiActions("actionShow_LogBook", N_("Open LogBook"), "Ctrl+L", "Plugin Key Bindings", true);
	gui->getGuiActions("actionShow_LogBook")->setChecked(flagShowLogBook);
	gui->addGuiActions("actionShow_LogBookConfigDialog", N_("Show data config dialog"), "ALT+L", group, true);
	gui->addGuiActions("actionShow_TargetsDialog", N_("Show Targets config dialog"), "ALT+T", group, true);

	connect(gui->getGuiActions("actionShow_LogBook"), SIGNAL(toggled(bool)), this, SLOT(enableLogBook(bool)));
	connect(gui->getGuiActions("actionShow_LogBookConfigDialog"), SIGNAL(toggled(bool)), this, SLOT(setConfigDialogVisible(bool)));
	connect(gui->getGuiActions("actionShow_TargetsDialog"), SIGNAL(toggled(bool)), this, SLOT(setTargetsDialogVisible(bool)));

	// Make a toolbar button
	try {
		pxmapGlow = new QPixmap(":/graphicGui/glow32x32.png");
		pxmapOnIcon = new QPixmap(":/logbook/bt_Logbook_on.png");
		pxmapOffIcon = new QPixmap(":/logbook/bt_Logbook_off.png");

		toolbarButton = new StelButton(NULL,
									   *pxmapOffIcon,
									   *pxmapOnIcon,
									   *pxmapGlow,
									   gui->getGuiActions("actionShow_LogBook"));
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	} catch (std::runtime_error& e) {
		qWarning() << "WARNING: unable create toolbar button for LogBook plugin: "<< e.what();
	}
}

bool LogBook::initializeDatabase()
{
	// Insure the module directory exists
	StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/LogBook");

	bool result = false;
	StelFileMgr::Flags flags = (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable);
	QString dbPath = StelFileMgr::findFile("modules/LogBook/", flags);
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "LogBook");
	db.setDatabaseName(dbPath + "logbook.sqlite");
	if (db.open()) {
		qDebug() << "LogBook opened the database successfully.";
		// See if the tables alreadt exist.
		QStringList tableList = db.tables();
		createDatabaseStructures();

		// Set the table models
		tableModels[BARLOWS] = new QSqlTableModel(this, db);
		tableModels[BARLOWS]->setTable(BARLOWS);
		tableModels[BARLOWS]->setObjectName("Barlows Table Model");
		tableModels[BARLOWS]->setEditStrategy(QSqlTableModel::OnFieldChange);
		tableModels[BARLOWS]->select();

		tableModels[FILTERS] = new QSqlTableModel(this, db);
		tableModels[FILTERS]->setTable(FILTERS);
		tableModels[FILTERS]->setObjectName("Filters Table Model");
		tableModels[FILTERS]->setEditStrategy(QSqlTableModel::OnFieldChange);
		tableModels[FILTERS]->select();

		tableModels[IMAGERS] = new QSqlTableModel(this, db);
		tableModels[IMAGERS]->setTable(IMAGERS);
		tableModels[IMAGERS]->setObjectName("Imagers Table Model");
		tableModels[IMAGERS]->setEditStrategy(QSqlTableModel::OnFieldChange);
		tableModels[IMAGERS]->select();

		tableModels[OBSERVATIONS] = new QSqlTableModel(this, db);
		tableModels[OBSERVATIONS]->setTable(OBSERVATIONS);
		tableModels[OBSERVATIONS]->setObjectName("Observations Table Model");
		tableModels[OBSERVATIONS]->setEditStrategy(QSqlTableModel::OnFieldChange);
		tableModels[OBSERVATIONS]->select();

		tableModels[OBSERVERS] = new QSqlTableModel(this, db);
		tableModels[OBSERVERS]->setTable(OBSERVERS);
		tableModels[OBSERVERS]->setObjectName("Observers Table Model");
		tableModels[OBSERVERS]->setEditStrategy(QSqlTableModel::OnFieldChange);
		tableModels[OBSERVERS]->select();

		tableModels[OCULARS] = new QSqlTableModel(this, db);
		tableModels[OCULARS]->setTable(OCULARS);
		tableModels[OCULARS]->setObjectName("Oculars Table Model");
		tableModels[OCULARS]->setEditStrategy(QSqlTableModel::OnFieldChange);
		tableModels[OCULARS]->select();

		tableModels[OPTICS] = new QSqlTableModel(this, db);
		tableModels[OPTICS]->setTable(OPTICS);
		tableModels[OPTICS]->setObjectName("Optics Table Model");
		tableModels[OPTICS]->setEditStrategy(QSqlTableModel::OnFieldChange);
		tableModels[OPTICS]->select();

		tableModels[OPTICS_TYPE] = new QSqlTableModel(this, db);
		tableModels[OPTICS_TYPE]->setTable(OPTICS_TYPE);
		tableModels[OPTICS_TYPE]->setObjectName("Optics Type Table Model");
		tableModels[OPTICS_TYPE]->setEditStrategy(QSqlTableModel::OnFieldChange);
		tableModels[OPTICS_TYPE]->select();

		tableModels[SESSIONS] = new QSqlTableModel(this, db);
		tableModels[SESSIONS]->setTable(SESSIONS);
		tableModels[SESSIONS]->setObjectName("Sites Table Model");
		tableModels[SESSIONS]->setEditStrategy(QSqlTableModel::OnFieldChange);
		tableModels[SESSIONS]->select();

		tableModels[SITES] = new QSqlTableModel(this, db);
		tableModels[SITES]->setTable(SITES);
		tableModels[SITES]->setObjectName("Sites Table Model");
		tableModels[SITES]->setEditStrategy(QSqlTableModel::OnFieldChange);
		tableModels[SITES]->select();

		tableModels[TARGETS] = new QSqlTableModel(this, db);
		tableModels[TARGETS]->setTable(TARGETS);
		tableModels[TARGETS]->setObjectName("Targets Table Model");
		tableModels[TARGETS]->setEditStrategy(QSqlTableModel::OnFieldChange);
		tableModels[TARGETS]->select();

		tableModels[TARGET_TYPES] = new QSqlTableModel(this, db);
		tableModels[TARGET_TYPES]->setTable(TARGET_TYPES);
		tableModels[TARGET_TYPES]->setObjectName("Target Types Table Model");
		tableModels[TARGET_TYPES]->setEditStrategy(QSqlTableModel::OnFieldChange);
		tableModels[TARGET_TYPES]->select();

		result = true;
	} else {
		qDebug() << "Oculars could not open its database; disabling module.";
		result = false;
	}
	return result;
}

bool LogBook::createDatabaseStructures()
{
	bool result = true;

	QSqlDatabase db = QSqlDatabase::database("LogBook");
	// See if the tables alreadt exist.
	QStringList tableList = db.tables();
	bool systemTableExists = tableList.contains("logbook_system");

	// Get the last record, if it exists
	QSqlQuery query(db);
	if (!systemTableExists || query.exec("SELECT last_script_run FROM logbook_system")) {
		int lastFile = 0;
		if (systemTableExists) {
			while (query.next()) {
				lastFile = query.value(0).toInt();
			}
		}

		QString path = ":/logbook/";
		QDir dir(path);
		QStringList entries = dir.entryList(QDir::Files, QDir::Name);
		QListIterator<QString> entriesIterator(entries);
		QSqlQuery updateQuery(db);
		updateQuery.prepare("UPDATE logbook_system SET last_script_run = :new WHERE last_script_run = :old");
		while (entriesIterator.hasNext() && result) {
			QString fileName = entriesIterator.next();
			int currentFile = fileName.section(".", 0, 0).toInt();
			if (fileName.endsWith("sql") &&  currentFile > lastFile) {
				if (!processSqlFile(QString(path).append(fileName))) {
					result = false;
				} else {
					//If this is the first file, it MUST create logbook_system
					if (!systemTableExists) {
						if(query.exec("SELECT last_script_run FROM logbook_system")){
							if(query.exec("INSERT INTO logbook_system (last_script_run) VALUES (0)")) {
								systemTableExists = true;
							} else {
								result = false;
								qDebug() << "LogBook: Error updateing system table 1.  Error is: " << query.lastError();
							}

						} else {
							result = false;
							qDebug() << "LogBook: Error reading system table 1.  Error is: " << query.lastError();
						}
					}

					// update the record
					updateQuery.bindValue(":new", currentFile);
					updateQuery.bindValue(":old", lastFile);
					QString sql("UPDATE logbook_system SET last_script_run = ");
					sql.append(QVariant(currentFile).toString()).append(" WHERE last_script_run = ").append(QVariant(lastFile).toString());
					if (query.exec(sql)) {
						lastFile = currentFile;
					} else {
						result = false;
						qDebug() << "LogBook: Error updateing system table; bind values are ("
								 << updateQuery.boundValues() << ").  \n\tError is: " << query.lastError()
								 << "\n\tThe query was: " << query.lastQuery() ;
					}
				}
			}
		}
	} else {
		result = false;
		qDebug() << "LogBook: Error reading system table 2.  Error is: " << query.lastError();
	}


	return result;
}

bool LogBook::processSqlFile(QString &fileName)
{
	bool result = true;
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "LogBook: could not process SQL file " << fileName;
		result = false;
	} else {
		QTextStream inStream(&file);
		while (!inStream.atEnd()) {
			QString line = inStream.readLine();
			if (!line.startsWith("--") && !line.trimmed().size() == 0) {
				if (!executeSql(line)) {
					result = false;
				}
			}
		}
	}
	qDebug() << "LogBook: finished processing SQL file  " << fileName;
	return result;
}

bool LogBook::executeSql(QString &sql)
{
	QSqlDatabase db = QSqlDatabase::database("LogBook");
	QSqlQuery query = QSqlQuery(db);
	bool result = query.exec(sql);
	if (!result) {
		qWarning() << "LogBook: error executing SQL: " << query.lastError();
		qWarning() << "LogBook: for SQL: " << sql;
	}
	return result;
}
