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

#include <QSettings>

#include "StelProjector.hpp"
#include "StelNavigator.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "TextUI.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"


void TextUI::init()
{
	return;
}


/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double TextUI::getCallOrder(StelModuleActionName actionName) const
{
	// TODO should always draw last (on top)
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("SolarSystem")->getCallOrder(actionName)+100;
	return 0;
}

void TextUI::update(double)
{
	return;
}


void TextUI::draw(StelCore*)
{
	return;
}

// Stub to hold translation keys
void temporary_translation()
{
	// TUI categories

}
