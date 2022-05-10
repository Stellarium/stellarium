/*
 * ArchaeoLines plug-in for Stellarium
 *
 * Copyright (C) 2021 Georg Zotti
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ARCHAEOLINESDIALOGLOCATIONS_HPP
#define ARCHAEOLINESDIALOGLOCATIONS_HPP

#include "StelDialog.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"

#include <QString>
#include <QColor>
#include <QColorDialog>
#include <QStringListModel>
#include <QSortFilterProxyModel>

class Ui_archaeoLinesDialogLocations;
class ArchaeoLines;

//! Auxiliary window of the ArchaeoLines plug-in. Most of this has been borrowed from the LocationDialog.
//! @ingroup archaeoLines
class ArchaeoLinesDialogLocations : public StelDialog
{
	Q_OBJECT

public:
	ArchaeoLinesDialogLocations();
	~ArchaeoLinesDialogLocations() Q_DECL_OVERRIDE;

public slots:
	void retranslate() Q_DECL_OVERRIDE;
	//! call with context 1 or 2 to prepare selection of location 1 or 2.
	void setModalContext(int context);

protected:
	void createDialogContent() Q_DECL_OVERRIDE;

private:
	Ui_archaeoLinesDialogLocations* ui;
	ArchaeoLines* al;
	int modalContext; //<! when set to 1 or 2, these represent the target custom locations to address in the plugin. Other values will not do anything.
	QStringListModel* allModel;
	QStringListModel* pickedModel;
	QSortFilterProxyModel *proxyModel;

private slots:
	//! Called whenever the StelLocationMgr is updated. Taken from LocationDialog.
	void reloadLocations();

	//! Connected to a QPushButton. When called, modalContext should have been set to 1 or 2 for a meaningful result.
	void setLocationFromList();
	//! Connected to the list. A double click on the cell calls this item.
	void setLocationFromList(const QModelIndex& index);
};

#endif /* ARCHAEOLINESDIALOGLOCATIONS_HPP */
