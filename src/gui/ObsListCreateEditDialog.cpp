/*
 * * Stellarium
 * Copyright (C) 2020 Jocelyn GIROD
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "ObsListCreateEditDialog.hpp"
#include "ui_obsListCreateEditDialog.h"

ObsListCreateEditDialog::ObsListCreateEditDialog(QObject* parent): StelDialog("Observing list creation/edition", parent)
{
    ui = new Ui_obsListCreateEditDialogForm();

}

ObsListCreateEditDialog::~ObsListCreateEditDialog()
{

}

/*
 * Retranslate dialog
*/
void ObsListCreateEditDialog::retranslate()
{
    if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

/*
 * Slot for button obsListAddObjectButton
*/
void ObsListCreateEditDialog::obsListAddObjectButtonPressed()
{
    //TODO
}

/*
 * Slot for button obsListRemoveObjectButton
*/
void ObsListCreateEditDialog::obsListRemoveObjectButtonPressed()
{
    //TODO
}

/*
 * Slot for button obsListExportListButton
*/
void ObsListCreateEditDialog::obsListExportListButtonPressed()
{
    //TODO
}

/*
 * Slot for button obsListImportListButton
*/
void ObsListCreateEditDialog::obsListImportListButtonPresssed()
{
    //TODO
}

/*
 * Slot for button obsListSaveButton
*/
void ObsListCreateEditDialog::obsListSaveButtonPressed()
{
    //TODO
}

/*
 * Slot for button obsListExitButton
*/
void ObsListCreateEditDialog::obsListExitButtonPressed()
{
    //TODO
}






