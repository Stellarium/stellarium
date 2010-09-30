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

#ifndef _IMPORT_WINDOW_
#define _IMPORT_WINDOW_

#include <QObject>
#include "StelDialog.hpp"

#include "CAImporter.hpp"

class Ui_importWindow;

/*! \brief Window for importing orbital elements from various sources.
  \author Bogdan Marinov
*/
class ImportWindow : public StelDialog
{
	Q_OBJECT
public:
	ImportWindow();
	virtual ~ImportWindow();
	void languageChanged();

private slots:
	void parseElements();
	void addObjects();

	void pasteClipboard();
	void selectFile();
	void bookmarkSelected(QString);

private:
	CAImporter * ssoManager;
	QList<CAImporter::SsoElements> candidateObjects;

	void populateCandidateObjects(QList<CAImporter::SsoElements>);

protected:
	virtual void createDialogContent();
	Ui_importWindow * ui;
};

#endif //_IMPORT_WINDOW_
