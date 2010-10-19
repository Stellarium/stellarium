/*
 * Time zone manager plug-in for Stellarium
 *
 * Copyright (C) 2010 Bogdan Marinov
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

#ifndef _DEFINE_TIME_ZONE_HPP_
#define _DEFINE_TIME_ZONE_HPP_

#include "StelDialog.hpp"

class Ui_defineTimeZoneForm;
class QRegExpValidator;

class DefineTimeZoneWindow : public StelDialog
{
	Q_OBJECT

public:
	DefineTimeZoneWindow();
	~DefineTimeZoneWindow();
	void languageChanged();

signals:
	void timeZoneDefined(QString timeZoneString);

protected:
	void createDialogContent();

private:
	Ui_defineTimeZoneForm * ui;

	QRegExpValidator * timeZoneNameValidator;

	void resetWindowState();
	void populateDateLists();

private slots:
	void useDefinition();
	void updateDstOffset(double normalOffset);
};


#endif //_DEFINE_TIME_ZONE_HPP_
