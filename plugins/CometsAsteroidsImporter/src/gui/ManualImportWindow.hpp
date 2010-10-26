/*
 * Comet and asteroids importer plug-in for Stellarium
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _MANUAL_IMPORT_WINDOW_
#define _MANUAL_IMPORT_WINDOW_

#include <QObject>
#include "StelDialog.hpp"

#include "CAImporter.hpp"

class Ui_manualImportWindow;

/*! \brief Window for manual entry of Solar System object properties.
  \author Bogdan Marinov
*/
class ManualImportWindow : public StelDialog
{
	Q_OBJECT
public:
	ManualImportWindow();
	virtual ~ManualImportWindow();
	void languageChanged();

private slots:


private:
	CAImporter * ssoManager;

protected:
	virtual void createDialogContent();
	Ui_manualImportWindow * ui;
};

#endif //_MANUAL_IMPORT_WINDOW_
