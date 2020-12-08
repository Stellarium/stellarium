/*
 * Angle Measure plug-in for Stellarium
 *
 * Copyright (C) 2014 Alexander Wolf
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

#ifndef REMOTECONTROLDIALOG_HPP
#define REMOTECONTROLDIALOG_HPP

#include "StelDialog.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"

#include <QString>

class Ui_remoteControlDialog;
class RemoteControl;

//! Main window of the Angle Measure plug-in.
class RemoteControlDialog : public StelDialog
{
	Q_OBJECT

public:
	RemoteControlDialog();
	~RemoteControlDialog();

public slots:
	void retranslate();

protected:
	void createDialogContent();

private:
	Ui_remoteControlDialog* ui;
	RemoteControl* rc;

	void setAboutHtml();

private slots:
	void requiresRestart();
	void restart();
	void updateIPlabel(bool);
	void restoreDefaults();
};


#endif /* REMOTECONTROLDIALOG_HPP */
