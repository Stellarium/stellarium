/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2015 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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

#ifndef _STOREDVIEWDIALOG_HPP_
#define _STOREDVIEWDIALOG_HPP_

#include "StelDialog.hpp"
#include "ui_storedViewDialog.h"

class Scenery3dMgr;
class StoredViewModel;

class StoredViewDialog : public StelDialog
{
	Q_OBJECT
public:
	StoredViewDialog(QObject* parent = NULL);
	~StoredViewDialog();
public slots:
	void retranslate();
protected:
	void createDialogContent();
private slots:
	void updateViewSelection(const QModelIndex &idx);
	void resetViewSelection();

	void updateCurrentView();

	void loadView();
	void deleteView();
	void addUserView();
private:
	Ui_storedViewDialogForm* ui;

	Scenery3dMgr* mgr;
	StoredViewModel* viewModel;
};

#endif
