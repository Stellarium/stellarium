/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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
 
#ifndef STELMODULE_H
#define STELMODULE_H

#include <QString>
#include <QObject>
#include "StelKey.hpp"
#include "StelTypes.hpp"

// Predeclaration
class Projector;
class Navigator;
class ToneReproducer;
class LoadingBar;
class InitParser;

using namespace std;

//! This is the common base class for all the main components of stellarium.
//! Standard StelModules are the one displaying the stars, the constellations, the planets etc..
//! Every new component should herit and implement the methods defined in StelModule.hpp.
//! Once a module is registered into the StelModuleMgr, it is automatically managed by the program.
//! Because StelModules are very generic components, it is also possible to load them dynamically,
//! thus enabling creation of external plug-ins for stellarium.
//! @author Fabien Chereau
class StelModule : public QObject
{
	Q_OBJECT

public:
	StelModule() {;}

	virtual ~StelModule() {;}
	
	//! Initialize itself from a configuration (.ini) file
	//! If the initialization takes significant time, the progress should be displayed on the loading bar.
	virtual void init(const InitParser& conf, LoadingBar& lb) = 0;
	
	//! Execute all the openGL drawing functions for this module.
	//! @return the max squared distance in pixels any single object has moved since the previous update.
	virtual double draw(Projector* prj, const Navigator * nav, ToneReproducer* eye) = 0;
	
	//! Update the module with respect to the time.
	//! @param delta_time the time increment in second since last call.
	virtual void update(double deltaTime) = 0;
	
	//! Update i18n strings from english names according to current global sky and application language.
	//! This method also reload the proper fonts depending on the language.
	//! The translation shall be done using the Translator provided by the StelApp singleton instance.
	virtual void updateI18n() {;}
		   
	//! Update sky culture, i.e. load data if necessary and translate them to current sky language if needed.
	virtual void updateSkyCulture(LoadingBar& lb) {;}	
	
	//! Get the version of the module, default is stellarium main version
	virtual QString getModuleVersion() const;
	
	//! Get the name of the module author
	virtual QString getAuthorName() const {return "Stellarium's Team";}
	
	//! Get the email adress of the module author
	virtual QString getAuthorEmail() const {return "http://www.stellarium.org";}

	//! Handle mouse clicks. Please note that most of the interactions will be done through the GUI module. 
	//! @param x X mouse position in pixels.
	//! @param y Y mouse position in pixels.
	//! @param button the mouse button. Can be Stel_BUTTON_LEFT, Stel_BUTTON_RIGHT, Stel_BUTTON_MIDDLE,
	//!   Stel_BUTTON_WHEELUP, Stel_BUTTON_WHEELDOWN
	//! @param state the state of the button. Can be Stel_MOUSEBUTTONDOWN, Stel_MOUSEBUTTONUP.
	//! @return false if the event was not intercepted, true otherwise.
	virtual bool handleMouseClicks(Uint16 x, Uint16 y, Uint8 button, Uint8 state, StelMod mod) {return false;}
	
	//! Handle mouse moves. Please note that most of the interactions will be done through the GUI module. 
	//! @param x X mouse position in pixels.
	//! @param y Y mouse position in pixels.
	//! @return false if the event was not intercepted, true otherwise.
	virtual bool handleMouseMoves(Uint16 x, Uint16 y, StelMod mod) {return false;}
	
	//! Handle key events. Please note that most of the interactions will be done through the GUI module.
	//! @param key the key code.
	//! @param mod the current mod state, needed to determine whether e.g CTRL or SHIFT key are pressed.
	//! @param unicode the unicode key code.
	//! @param state the press state of the key. Can be Stel_KEYDOWN or Stel_KEYUP.
	//! @return false if the event was not intercepted, true otherwise.
	virtual bool handleKeys(StelKey key, StelMod mod, Uint16 unicode, Uint8 state) {return false;}
	
	//! Indicate that the user requested selection of StelObjects.
	//! The StelModule should then manage by themself how they handle the event
	//! @param added true if the user request that the objects are added to the selection
	virtual void selectedObjectChangeCallBack(bool added=false) {;}
	
	//! Load color scheme from the given ini file and section name
	//! @param conf the iniparser containing the configuration items
	//! @param section the name of the section of the ini file containing the configuration items
	virtual void setColorScheme(const InitParser& conf, const QString& section) {;}
	
	//! This method is called for all StelModules when the GL window is resized
	virtual void glWindowHasBeenResized(int w, int h) {;}
	
	//! Define the possible action for which an order is defined
	enum StelModuleActionName
	{
		ACTION_DRAW,
		ACTION_UPDATE,
		ACTION_HANDLEMOUSECLICKS,
		ACTION_HANDLEMOUSEMOVES,
		ACTION_HANDLEKEYS
	};
	
	//! Return the value defining the order of call for the given action
	//! For example if stars.callOrder[ACTION_DRAW] == 10 and constellation.callOrder[ACTION_DRAW] == 11, 
	//! the stars module will be drawn before the constellations
	//! @param actionName the name of the action for which we want the call order
	//! @return the value defining the order. The closer to 0 the earlier the module's action will be called
	virtual double getCallOrder(StelModuleActionName actionName) const {return 0;}
	
protected:
	friend class StelModuleMgr;
};

#endif
