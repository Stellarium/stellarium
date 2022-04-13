/*
 * ArchaeoLines plug-in for Stellarium
 *
 * Copyright (C) 2014 Georg Zotti
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

#ifndef ARCHAEOLINESDIALOG_HPP
#define ARCHAEOLINESDIALOG_HPP

#include "StelDialog.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"

#include <QString>
#include <QColor>
#include <QColorDialog>

class Ui_archaeoLinesDialog;
class ArchaeoLines;
class ArchaeoLinesDialogLocations;

//! Main window of the ArchaeoLines plug-in.
//! @ingroup archaeoLines
class ArchaeoLinesDialog : public StelDialog
{
	Q_OBJECT

public:
	ArchaeoLinesDialog();
	~ArchaeoLinesDialog() Q_DECL_OVERRIDE;

public slots:
	void retranslate() Q_DECL_OVERRIDE;

protected:
	void createDialogContent() Q_DECL_OVERRIDE;

private:
	Ui_archaeoLinesDialog* ui;
	ArchaeoLines* al;
	ArchaeoLinesDialogLocations* locationsDialog;

	void setAboutHtml();

private slots:
	void resetArchaeoLinesSettings();
	//! Switch between decimal and DMS inputs
	void setDisplayFormatForSpins(bool flagDecimalDegrees);
	//! assign fields from current selection
	void assignCustomAzimuth1FromSelection();
	void assignCustomAzimuth2FromSelection();
	void assignCustomAltitude1FromSelection();
	void assignCustomAltitude2FromSelection();
	void assignCustomDeclination1FromSelection();
	void assignCustomDeclination2FromSelection();
	//! Connected to a QPushButton with a property "geo" of value 1 or 2
	//void assignGeographicLocationFromList();
};

#endif /* ARCHAEOLINESDIALOG_HPP */
