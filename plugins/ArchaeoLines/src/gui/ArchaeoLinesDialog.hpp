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

#ifndef _ARCHAEOLINESDIALOG_HPP_
#define _ARCHAEOLINESDIALOG_HPP_

#include "StelDialog.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"

#include <QString>
#include <QColor>
#include <QColorDialog>

class Ui_archaeoLinesDialog;
class ArchaeoLines;

//! Main window of the ArchaeoLines plug-in.
class ArchaeoLinesDialog : public StelDialog
{
	Q_OBJECT

public:
	ArchaeoLinesDialog();
	~ArchaeoLinesDialog();

public slots:
	void retranslate();
	// actions to be called by the color dialog
	// These must also forward the colors in float format to the ArchaeoLines object.
	void setEquinoxColor(QColor color){equinoxColor=color;}
	void setSolsticeColor(QColor color){solsticeColor=color;}
	void setCrossquarterColor(QColor color){crossquarterColor=color;}
	void setMajorStandstillColor(QColor color){majorStandstillColor=color;}
	void setMinorStandstillColor(QColor color){minorStandstillColor=color;}
	void setZenithPassageColor(QColor color){zenithPassageColor=color;}
	void setNadirPassageColor(QColor color){nadirPassageColor=color;}
	void setCurrentSunColor(QColor color){currentSunColor=color;}
	void setCurrentMoonColor(QColor color){currentMoonColor=color;}
	void setCurrentPlanetColor(QColor color){currentPlanetColor=color;}
	void setSelectedObjectColor(QColor color){selectedObjectColor=color;}
	void askEquinoxColor();
	void askSolsticeColor();
	void askCrossquarterColor();
	void askMajorStandstillColor();
	void askMinorStandstillColor();
	void askZenithPassageColor();
	void askNadirPassageColor();
	void askCurrentSunColor();
	void askCurrentMoonColor();
	void askCurrentPlanetColor();
	void askSelectedObjectColor();

protected:
	void createDialogContent();

private:
	Ui_archaeoLinesDialog* ui;
	ArchaeoLines* al;
	// These are colors to be settable with a QColorDialog.
	QColor equinoxColor;
	QColor solsticeColor;
	QColor crossquarterColor;
	QColor majorStandstillColor;
	QColor minorStandstillColor;
	QColor zenithPassageColor;
	QColor nadirPassageColor;
	QColor currentSunColor;
	QColor currentMoonColor;
	QColor currentPlanetColor;
	QColor selectedObjectColor;
	QPixmap equinoxColorPixmap;
	QPixmap solsticeColorPixmap;
	QPixmap crossquarterColorPixmap;
	QPixmap majorStandstillColorPixmap;
	QPixmap minorStandstillColorPixmap;
	QPixmap zenithPassageColorPixmap;
	QPixmap nadirPassageColorPixmap;
	QPixmap currentSunColorPixmap;
	QPixmap currentMoonColorPixmap;
	QPixmap currentPlanetColorPixmap;
	QPixmap selectedObjectColorPixmap;
	void setAboutHtml();

private slots:
	void resetArchaeoLinesSettings();
	//! setting planet requires a small function to link Combobox indices to line IDs.
	void setCurrentPlanetFromGUI(int index);

};

#endif /* _ARCHAEOLINESDIALOG_HPP_ */
