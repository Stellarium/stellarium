/*
 * Stellarium
 * This file Copyright (C) 2004-2008 Robert Spearman
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

#ifndef _TEXTUI_HPP_
#define _TEXTUI_HPP_

#include "StelModule.hpp"

//! @class TextUI
//! Text user interface stub.
class TextUI : public StelModule
{
	Q_OBJECT

public:
	//! Construct a TextUI object.
	TextUI(void) { 	setObjectName("TextUI"); }
	virtual ~TextUI() {;}
 
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the TextUI object.
	virtual void init();
	
	//! Draw
	virtual void draw(StelCore* core);
	
	//! Update time-dependent parts of the module.
	virtual void update(double deltaTime);
	
	//! Defines the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
//public slots:
	///////////////////////////////////////////////////////////////////////////
	// Method callable from script and GUI
	
private:

	// temporary location to store strings for translating
	void temporary_translation();  

	
};


#endif // _TEXTUI_HPP_
