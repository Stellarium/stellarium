/*
 * Stellarium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef PSFSTARSDIALOG_HPP
#define PSFSTARSDIALOG_HPP

#include <QObject>
#include "StelDialog.hpp"

class Ui_PsfStarsDialogForm;

class PsfStarsDialog : public StelDialog
{
	Q_OBJECT

public:
	PsfStarsDialog();
	~PsfStarsDialog() override;

public slots:
	void retranslate() override;

protected:
	void createDialogContent() override;
	Ui_PsfStarsDialogForm *ui;
};

#endif // PSFSTARSDIALOG_HPP
