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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _STELNOGUI_HPP_
#define _STELNOGUI_HPP_

#include "StelGuiBase.hpp"

//! @class StelNoGui
//! Dummy implementation of StelGuiBase to use when no GUI is used.
class StelNoGui : public StelGuiBase
{
public:
	StelNoGui() {;}
	~StelNoGui() {;}
	virtual void init(QGraphicsWidget* topLevelGraphicsWidget, class StelAppGraphicsWidget* stelAppGraphicsWidget) {;}
	virtual void updateI18n() {;}
	virtual void setStelStyle(const QString& section) {;}
	virtual void setInfoTextFilters(const StelObject::InfoStringGroup& aflags) {dummyInfoTextFilter=aflags;}
	virtual const StelObject::InfoStringGroup& getInfoTextFilters() const {return dummyInfoTextFilter;}
	virtual class QProgressBar* addProgressBar();
	virtual QAction* addGuiActions(const QString& actionName, const QString& text, const QString& shortCut, const QString& helpGroup, bool checkable=true, bool autoRepeat=false) {return NULL;}
	virtual QAction* getGuiActions(const QString& actionName) {return NULL;}
	virtual void forceRefreshGui() {;}	
	virtual void setVisible(bool b) {visible=b;}
	virtual bool getVisible() const {return visible;}
	virtual bool isCurrentlyUsed() const {return false;}
private:
	StelObject::InfoStringGroup dummyInfoTextFilter;
	bool visible;
};

//! An example GUI plugin with an empty GUI.
class StelNoGuiPluginInterface : public QObject, public StelGuiPluginInterface
{
	Q_OBJECT;
	Q_INTERFACES(StelGuiPluginInterface);
public:
	virtual class StelGuiBase* getStelGuiBase() const;
};



#endif // _STELNOGUI_HPP_
