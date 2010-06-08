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

#ifndef _LOGBOOKCONFIGDIALOG_HPP_
#define _LOGBOOKCONFIGDIALOG_HPP_

#include <QObject>
#include "StelDialogLogBook.hpp"
#include "StelStyle.hpp"

#include "ui_LogBookConfigDialog.h"
#include "ui_BarlowsWidget.h"
#include "ui_FiltersWidget.h"
#include "ui_ImagersWidget.h"
#include "ui_ObserversWidget.h"
#include "ui_OcularsWidget.h"
#include "ui_OpticsWidget.h"
#include "ui_SitesWidget.h"

#include <QMap>

class Ui_LogBookConfigDialogForm;
class QSqlTableModel; 

class LogBookConfigDialog : public StelDialogLogBook {
	Q_OBJECT
	
public:
	LogBookConfigDialog(QMap<QString, QSqlTableModel *> theTableModels);
	virtual ~LogBookConfigDialog();
	void languageChanged();
	//! Notify that the application style changed
	void styleChanged();
	void updateStyle();
	
public slots:
	void closeWindow();

signals:
	void currentBarlowChanged();
	void currentFilterChanged();
	void currentImagerChanged();
	void currentObserverChanged();
	void currentOcularChanged();
	void currentOpticChanged();
	void currentSiteChanged();

protected:

	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	void setupListViews();
	//! helper for createDialogContent() that handles the referenced ui widgets. 
	void setupWidgets();

	// Widgets
	Ui_LogBookConfigDialogForm *ui;
	Ui_BarlowsWidget *barlowsWidget;
	Ui_FiltersWidget *filtersWidget;
	Ui_ImagersWidget *imagersWidget;
	Ui_ObserversWidget *observersWidget;
	Ui_OcularsWidget *ocularsWidget;
	Ui_OpticsWidget *opticsWidget;
	Ui_SitesWidget *sitesWidget;
	
	QMap<QString, QSqlTableModel *> tableModels;
	QMap<QString, QWidget *> widgets;
	
	// Selections
	int selectedBarlowRow;
	int selectedFilterRow;
	int selectedImagerRow;
	int selectedObserverRow;
	int selectedOcularRow;
	int selectedOpticRow;
	int selectedSiteRow;
};

#endif // _LOGBOOKCONFIGDIALOG_HPP_
