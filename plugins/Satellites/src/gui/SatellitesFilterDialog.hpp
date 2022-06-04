/*
 * Stellarium Satellites Plug-in: satellites custom filter feature
 * Copyright (C) 2022 Alexander Wolf
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef FILTERSATELLITESWINDOW_HPP
#define FILTERSATELLITESWINDOW_HPP

#include "StelDialog.hpp"
#include "Satellites.hpp"

class Ui_satellitesFilterDialog;

//! @ingroup satellites
class SatellitesFilterDialog : public StelDialog
{
	Q_OBJECT
	
public:
	SatellitesFilterDialog();
	~SatellitesFilterDialog() Q_DECL_OVERRIDE;
	
public slots:
	void retranslate() Q_DECL_OVERRIDE;
	void setVisible(bool visible = true) Q_DECL_OVERRIDE;

private slots:
	void populateTexts();
	void updateMinMaxInclination(bool state);
	void updateMinMaxApogee(bool state);
	void updateMinMaxPerigee(bool state);
	void updateMinMaxPeriod(bool state);
	void updateMinMaxEccentricity(bool state);
	void updateMinMaxRCS(bool state);
	
private:
	void createDialogContent() Q_DECL_OVERRIDE;
	Ui_satellitesFilterDialog* ui;
};

#endif // FILTERSATELLITESWINDOW_HPP
