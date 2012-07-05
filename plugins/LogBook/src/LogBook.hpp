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

#ifndef LOGBOOK_HPP_
#define LOGBOOK_HPP_

#include "StelModule.hpp"
#include <QMap>

class LogBookConfigDialog;
class ObservationsDialog;
class SessionsDialog;
class TargetsDialog;

class QKeyEvent;
class QMouseEvent;
class QPixmap;
class QSqlDatabase;
class QSqlTableModel;
class QSqlQuery;

class StelButton;
class StelStyle;

//! This is an example of a plug-in which can be dynamically loaded into stellarium
class LogBook : public StelModule
{
	Q_OBJECT
public:
	LogBook();
	virtual ~LogBook();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual bool configureGui(bool show=true);
	virtual void draw(StelCore* core, class StelRenderer* renderer);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	//! Returns the module-specific style sheet.
	//! The main StelStyle instance should be passed.
	const StelStyle getModuleStyleSheet(const StelStyle& style);
	virtual void init();
	virtual void setStelStyle(const QString& section);
	virtual void update(double) {;}

public slots:
	//! This method is called with we detect that our hot key is pressed.  It handles
	//! determining if we should do anything - based on a selected object - and painting
	//! labes to the screen.
	void enableLogBook(bool b);
	void setConfigDialogVisible(bool b);
	void setTargetsDialogVisible(bool b);

private:
	//! Creates database structures. This is done by process every file that ends with .sql that is
	//! in the LogBook resources, in order. So 001.sql will be processed before 002.sql.  Each file
	//! needs to have one SQL statement per line.
	//! @return true if every line in the file was executed successfully.
	bool createDatabaseStructures();
	
	//! executes a single SQL statement.
	//! @param the SQL string.
	//! @return true if there was no error, false if there was an error.
	bool executeSql(QString &sql);
	
	void initializeActions();
	
	//! Insures that the database tables exist.  It not, calls createDatabaseStructures() to create them.
	bool initializeDatabase();
	
	//Styles
	QByteArray normalStyleSheet;
	QByteArray nightStyleSheet;

	//! reads a file line by line, and calls executeSql() with each line. Blank lines or SQL comments are ignored.
	//! @param the file name that contains the SQL.
	//! @return true if there was no error, false if there was an error.
	bool processSqlFile(QString &fileName);

	LogBookConfigDialog *configDialog;
	SessionsDialog *sessionsDialog;
	TargetsDialog *targetsDialog;

	//! flag used to track if we are in log book mode.
	bool flagShowLogBook;
	QMap<QString, QSqlTableModel *> tableModels;

	// for toolbar button
	QPixmap* pxmapGlow;
	QPixmap* pxmapOnIcon;
	QPixmap* pxmapOffIcon;
	StelButton* toolbarButton;
};


#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class LogBookStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*LOGBOOK_HPP_*/
