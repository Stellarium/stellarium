/*
 * Time Navigator plug-in for Stellarium
 *
 * Copyright (C) 2026 Atque
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

#ifndef TIMENAVIGATOR_DIALOG_HPP
#define TIMENAVIGATOR_DIALOG_HPP

#include "StelDialog.hpp"

class Ui_timeNavigatorDialog;
class StelCore;
class SpecificTimeMgr;
class StelObjectMgr;

//! @class TimeNavigatorDialog
//! @ingroup timeNavigator
//! Main window of the Time Navigator plug-in.
class TimeNavigatorDialog : public StelDialog
{
	Q_OBJECT

public:
	TimeNavigatorDialog();
	~TimeNavigatorDialog() override;

public slots:
	void retranslate() override;

protected:
	void createDialogContent() override;

private slots:
	void restoreDefaults();
	void saveSettings();
	void updateSelectedObjectState();

private:
	Ui_timeNavigatorDialog* ui;

	StelCore*        core;
	SpecificTimeMgr* specMgr;
	StelObjectMgr*   objMgr;

	void setAboutHtml();
	void connectTwilightAltitudeSpinBox();
};

#endif /* TIMENAVIGATOR_DIALOG_HPP */
