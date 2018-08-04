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

//! Main window of the ArchaeoLines plug-in.
//! @ingroup archaeoLines
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
	void setSelectedObjectAzimuthColor(QColor color){selectedObjectAzimuthColor=color;}
	void setSelectedObjectHourAngleColor(QColor color){selectedObjectHourAngleColor=color;}
	void setGeographicLocation1Color(QColor color){geographicLocation1Color=color;}
	void setGeographicLocation2Color(QColor color){geographicLocation2Color=color;}
	void setCustomAzimuth1Color(QColor color){customAzimuth1Color=color;}
	void setCustomAzimuth2Color(QColor color){customAzimuth2Color=color;}
	void setCustomDeclination1Color(QColor color){customDeclination1Color=color;}
	void setCustomDeclination2Color(QColor color){customDeclination2Color=color;}
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
	void askSelectedObjectAzimuthColor();
	void askSelectedObjectHourAngleColor();
	void askGeographicLocation1Color();
	void askGeographicLocation2Color();
	void askCustomAzimuth1Color();
	void askCustomAzimuth2Color();
	void askCustomDeclination1Color();
	void askCustomDeclination2Color();

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
	QColor selectedObjectAzimuthColor;
	QColor selectedObjectHourAngleColor;
	QColor geographicLocation1Color;
	QColor geographicLocation2Color;
	QColor customAzimuth1Color;
	QColor customAzimuth2Color;
	QColor customDeclination1Color;
	QColor customDeclination2Color;
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
	QPixmap selectedObjectAzimuthColorPixmap;
	QPixmap selectedObjectHourAngleColorPixmap;
	QPixmap geographicLocation1ColorPixmap;
	QPixmap geographicLocation2ColorPixmap;
	QPixmap customAzimuth1ColorPixmap;
	QPixmap customAzimuth2ColorPixmap;
	QPixmap customDeclination1ColorPixmap;
	QPixmap customDeclination2ColorPixmap;
	void setAboutHtml();

private slots:
	void resetArchaeoLinesSettings();
	//! setting planet requires a small function to link Combobox indices to line IDs.
	void setCurrentPlanetFromGUI(int index);
	void setCurrentPlanetFromApp();

};

#endif /* ARCHAEOLINESDIALOG_HPP */
