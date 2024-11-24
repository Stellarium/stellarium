/*
 * Mosaic Camera Plug-in GUI
 *
 * Copyright (C) 2024 Josh Meyers
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

#ifndef MOSAICCAMERADIALOG_HPP
#define MOSAICCAMERADIALOG_HPP

#include <QObject>
#include "StelDialog.hpp"
#include "MosaicCamera.hpp"

class Ui_mosaicCameraDialog;
class QTimer;
class MosaicCamera;

//! @ingroup mosaicCamera
class MosaicCameraDialog : public StelDialog
{
	Q_OBJECT

public:
	MosaicCameraDialog();
	~MosaicCameraDialog() override;

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	void createDialogContent() override;

public slots:
	void retranslate() override;

private:
	MosaicCamera* msm;
	Ui_mosaicCameraDialog* ui;
	void setAboutHtml(void);
};

#endif // MOSAICCAMERADIALOG_HPP
