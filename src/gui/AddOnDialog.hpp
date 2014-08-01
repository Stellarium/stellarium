/*
 * Stellarium
 * Copyright (C) 2014 Marcos Cardinot
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#ifndef _ADDONDIALOG_HPP_
#define _ADDONDIALOG_HPP_

#include "StelAddOnMgr.hpp"
#include "StelDialog.hpp"

#include <QCheckBox>
#include <QListWidgetItem>
#include <QNetworkReply>
#include <QObject>
#include <QTableView>

class Ui_addonDialogForm;

class AddOnDialog : public StelDialog
{
Q_OBJECT
public:
    AddOnDialog(QObject* parent);
    virtual ~AddOnDialog();
	//! Notify that the application style changed
	void styleChanged();

public slots:
	void retranslate();

protected:
	Ui_addonDialogForm* ui;
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();

private slots:
	void changePage(QListWidgetItem *current, QListWidgetItem *previous);
	void updateCatalog();
	void downloadError(QNetworkReply::NetworkError);
	void downloadFinished();
	void installSelectedRows();
	void removeSelectedRows();
	void populateTables();
	void slotRowSelected(int row, bool checked);

private:
	//! All categories sorted according to the tab index.
	//! @enum tab
	enum Tab {
		CATALOG,
		LANDSCAPE,
		LANGUAGEPACK,
		SCRIPT,
		STARLORE,
		TEXTURE
	};

	QNetworkReply* m_pUpdateCatalogReply;
	QTableView* m_currentTableView;
	QHash<Tab, QButtonGroup*> m_checkBoxes;
	QList<QPair<int, int> > m_iSelectedAddOns;

	void updateTabBarListWidgetWidth();
	void setUpTableView(QTableView* tableView, QString tableName);
	bool isCompatible(QString first, QString last);
	void insertCheckBoxes(QTableView* tableview, Tab tab);
};

#endif // _ADDONDIALOG_HPP_
