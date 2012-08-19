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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _STELMODULE_HPP_
#define _STELMODULE_HPP_

#include <QString>
#include <QObject>

// Predeclaration
class StelCore;
class QSettings;

//! This is the common base class for all the main components of stellarium.
//! Standard StelModules are the one displaying the stars, the constellations, the planets etc..
//! Every new module derived class should implement the methods defined in StelModule.hpp.
//! Once a module is registered into the StelModuleMgr, it is automatically managed by the program.
//! It can be called using the GETSTELMODULE macro, passing as argument its name, which is also the QObject name
//! Because StelModules are very generic components, it is also possible to load them dynamically,
//! thus enabling creation of external plug-ins for stellarium.
//! @sa StelObjectModule for StelModule managing collections of StelObject.
//! @sa @ref plugins for documentation on how to develop external plugins.
//!
//! There are several signals that StelApp emits that subclasses may be interested in:
//! laguageChanged()
//!	Update i18n strings from english names according to current global sky and application language.
//!	This method also reload the proper fonts depending on the language.
//!	The translation shall be done using the StelTranslator provided by the StelApp singleton instance.
//! skyCultureChanged(const QString&)
//!	Update sky culture, i.e. load data if necessary and translate them to current sky language if needed.
//! colorSchemeChanged(const QString&)
//!	Load the given color style
class StelModule : public QObject
{
	// Do not add Q_OBJECT here!!
	// This make this class compiled by the Qt moc compiler and for some unknown reasons makes it impossible to dynamically
	// load plugins on windows.
public:
	StelModule() {;}

	virtual ~StelModule() {;}

	//! Initialize itself.
	//! If the initialization takes significant time, the progress should be displayed on the loading bar.
	virtual void init() = 0;

	//! Called before the module will be deleted, and before the renderer is destroyed.
	//! Deinitialize all textures in this method.
	virtual void deinit() {;}

	//! Execute all the drawing functions for this module.
	//! @param core     the core to use for the drawing
	//! @param renderer Renderer to draw with.
	virtual void draw(StelCore* core, class StelRenderer* renderer) 
	{
		Q_UNUSED(core);
		Q_UNUSED(renderer);
	}

	//! Iterate through the drawing sequence.
	//! This allow us to split the slow drawing operation into small parts,
	//! we can then decide to pause the painting for this frame and used the cached image instead.
	//! @return true if we should continue drawing (by calling the method again)
	virtual bool drawPartial(StelCore* core, class StelRenderer* renderer);

	//! Update the module with respect to the time.
	//! @param deltaTime the time increment in second since last call.
	virtual void update(double deltaTime) = 0;

	//! Get the version of the module, default is stellarium main version
	virtual QString getModuleVersion() const;

	//! Get the name of the module author
	virtual QString getAuthorName() const {return "Stellarium's Team";}

	//! Get the email adress of the module author
	virtual QString getAuthorEmail() const {return "http://www.stellarium.org";}

	//! Handle mouse clicks. Please note that most of the interactions will be done through the GUI module.
	//! @return set the event as accepted if it was intercepted
	virtual void handleMouseClicks(class QMouseEvent*) {;}

	//! Handle mouse wheel. Please note that most of the interactions will be done through the GUI module.
	//! @return set the event as accepted if it was intercepted
	virtual void handleMouseWheel(class QWheelEvent*) {;}

	//! Handle mouse moves. Please note that most of the interactions will be done through the GUI module.
	//! @return true if the event was intercepted
	virtual bool handleMouseMoves(int x, int y, Qt::MouseButtons b) {Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(b); return false;}

	//! Handle key events. Please note that most of the interactions will be done through the GUI module.
	//! @param e the Key event
	//! @return set the event as accepted if it was intercepted
	virtual void handleKeys(class QKeyEvent* e) {Q_UNUSED(e);}

	//! Enum used when selecting objects to define whether to add to, replace, or remove from the existing selection list.
	enum StelModuleSelectAction
	{
		AddToSelection,     //!< Add the StelObject to the current list of selected ones.
		ReplaceSelection,	//!< Set the StelObject as the new list of selected ones.
		RemoveFromSelection //!< Subtract the StelObject from the current list of selected ones.
	};

	//! Define the possible action for which an order is defined
	enum StelModuleActionName
	{
		ActionDraw,              //!< Action associated to the draw() method
		ActionUpdate,            //!< Action associated to the update() method
		ActionHandleMouseClicks, //!< Action associated to the handleMouseClicks() method
		ActionHandleMouseMoves,  //!< Action associated to the handleMouseMoves() method
		ActionHandleKeys         //!< Action associated to the handleKeys() method
	};

	//! Return the value defining the order of call for the given action
	//! For example if stars.callOrder[ActionDraw] == 10 and constellation.callOrder[ActionDraw] == 11,
	//! the stars module will be drawn before the constellations
	//! @param actionName the name of the action for which we want the call order
	//! @return the value defining the order. The closer to 0 the earlier the module's action will be called
	virtual double getCallOrder(StelModuleActionName actionName) const {Q_UNUSED(actionName); return 0;}

	//! Detect or show the configuration GUI elements for the module.  This is to be used with
	//! plugins to display a configuration dialog from the plugin list window.
	//! @param show if true, make the configuration GUI visible.  If false, hide the config GUI if there is one.
	//! @return true if the module has a configuration GUI, else false.
	virtual bool configureGui(bool show=true) {Q_UNUSED(show); return false;}
};

#endif // _STELMODULE_HPP_
