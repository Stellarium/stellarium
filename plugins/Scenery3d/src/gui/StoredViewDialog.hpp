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

#ifndef STOREDVIEWDIALOG_HPP
#define STOREDVIEWDIALOG_HPP

#include "StelDialog.hpp"
#include "ui_storedViewDialog.h"

class Scenery3d;
class StoredViewModel;

class StoredViewDialog : public StelDialog
{
	Q_OBJECT
public:
	StoredViewDialog(QObject* parent = Q_NULLPTR);
	virtual ~StoredViewDialog() Q_DECL_OVERRIDE;
public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;
protected:
	virtual void createDialogContent() Q_DECL_OVERRIDE;
private slots:
	void updateViewSelection(const QModelIndex &idx);
	void resetViewSelection();

	void updateCurrentView();

	void loadView();
	void deleteView();
	void addUserView();
private:
	Ui_storedViewDialogForm* ui;

	Scenery3d* mgr;
	StoredViewModel* viewModel;
};

#endif
