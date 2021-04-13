/*
 * Navigational Stars plug-in for Stellarium
 *
 * Copyright (C) 2016 Alexander Wolf
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

#ifndef NAVSTARSWINDOW_HPP
#define NAVSTARSWINDOW_HPP

#include "StelDialog.hpp"

class Ui_navStarsWindowForm;
class NavStars;

//! Main window of the Navigational Stars plug-in.
//! @ingroup navStars
class NavStarsWindow : public StelDialog
{
	Q_OBJECT

public:
	NavStarsWindow();
	~NavStarsWindow();

public slots:
	void retranslate();

protected:
	void createDialogContent();

private:
	Ui_navStarsWindowForm* ui;
	NavStars* ns;

	void setAboutHtml();
	void populateNavigationalStarsSetDescription();

private slots:
	void saveSettings();
	void resetSettings();
	void populateNavigationalStarsSets();
	void setNavigationalStarsSet(int nsSetID);
};


#endif /* NAVSTARSWINDOW_HPP */
