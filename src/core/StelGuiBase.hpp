/*
 * Stellarium
 * Copyright (C) 2009 Fabien Chereau
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef STELGUIBASE_HPP
#define STELGUIBASE_HPP

#include "StelObject.hpp"

class QGraphicsWidget;
class QAction;

//! @class StelGuiBase
//! Abstract class defining the base interface for all GUIs.
class StelGuiBase
{
public:
	StelGuiBase();
	virtual ~StelGuiBase() {;}

	virtual void init(QGraphicsWidget *atopLevelGraphicsWidget);

	//! Load color scheme matching the section name.
	virtual void setStelStyle(const QString& section) =0;

	//! Get a pointer on the info panel used to display selected object info
	virtual void setInfoTextFilters(const StelObject::InfoStringGroup& aflags) =0;
	virtual const StelObject::InfoStringGroup& getInfoTextFilters() const =0;

	virtual void forceRefreshGui() {;}

	//! Show whether the GUI is visible.
	//! @param b when true, GUI will be shown, else it will be hidden.
	virtual void setVisible(bool b) =0;
	//! Get the current visible status of the GUI.
	virtual bool getVisible() const =0;
	//! Show wether the Gui is currently used.
	//! This can then be used to optimize the rendering to increase reactivity.
	virtual bool isCurrentlyUsed() const =0;
};

#endif // STELGUIBASE_HPP
