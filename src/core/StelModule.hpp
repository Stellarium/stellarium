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

#ifndef STELMODULE_HPP
#define STELMODULE_HPP

#include <QString>
#include <QObject>
#include <functional>

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
	Q_OBJECT
	// Do not add Q_OBJECT here!!
	// This make this class compiled by the Qt moc compiler and for some unknown reasons makes it impossible to dynamically
	// load plugins on windows.
	Q_ENUMS(StelModuleSelectAction)
	Q_ENUMS(StelModuleActionName)

public:
	StelModule();

	virtual ~StelModule() {;}

	//! Initialize itself.
	//! If the initialization takes significant time, the progress should be displayed on the loading bar.
	virtual void init() = 0;

	//! Called before the module will be delete, and before the openGL context is suppressed.
	//! Deinitialize all openGL texture in this method.
	virtual void deinit() {;}

	//! Return module-specific settings. This can be useful mostly by plugins which may want to keep their settings to their own files.
	//! The default implementation returns a null pointer!
	virtual QSettings *getSettings() {return Q_NULLPTR;}

	//! Execute all the drawing functions for this module.
	//! @param core the core to use for the drawing
	virtual void draw(StelCore* core) {Q_UNUSED(core);}

	//! Update the module with respect to the time.
	//! @param deltaTime the time increment in second since last call.
	virtual void update(double deltaTime) = 0;

	//! Get the version of the module, default is stellarium main version
	virtual QString getModuleVersion() const;

	//! Get the name of the module author
	virtual QString getAuthorName() const {return "Stellarium's Team";}

	//! Get the email adress of the module author
	virtual QString getAuthorEmail() const {return "stellarium@googlegroups.com";}

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

	//! Handle pinch gesture events.
	//! @param scale the value of the pinch gesture.
	//! @param started define whether the pinch has just started.
	//! @return true if the event was intercepted.
	virtual bool handlePinch(qreal scale, bool started) {Q_UNUSED(scale); Q_UNUSED(started); return false;}

	//! Enum used when selecting objects to define whether to add to, replace, or remove from the existing selection list.
	enum StelModuleSelectAction
	{
		AddToSelection,		//!< Add the StelObject to the current list of selected ones.
		ReplaceSelection,	//!< Set the StelObject as the new list of selected ones.
		RemoveFromSelection	//!< Subtract the StelObject from the current list of selected ones.
	};
	#if QT_VERSION >= 0x050500
	Q_ENUM(StelModuleSelectAction)
	#endif
	//! Define the possible action for which an order is defined
	enum StelModuleActionName
	{
		ActionDraw,              //!< Action associated to the draw() method
		ActionUpdate,            //!< Action associated to the update() method
		ActionHandleMouseClicks, //!< Action associated to the handleMouseClicks() method
		ActionHandleMouseMoves,  //!< Action associated to the handleMouseMoves() method
		ActionHandleKeys         //!< Action associated to the handleKeys() method
	};
	#if QT_VERSION >= 0x050500
	Q_ENUM(StelModuleActionName)
	#endif
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

protected:
	//! convenience methods to add an action (call to slot) to the StelActionMgr object.
	//! @param id unique identifier. Should be called actionMy_Action. (i.e., start with "action" and then "Capitalize_Underscore" style.)
	//! @param groupId string to be used in the Help menu. The action will be listed in this group.
	//! @param text short translatable description what the action does.
	//! @param target recipient of the call
	//! @param slot name of slot in target recipient
	//! @param shortcut default shortcut. Can be reconfigured.
	//! @param altShortcut default alternative shortcut. Can be reconfigured.
	class StelAction* addAction(const QString& id, const QString& groupId, const QString& text,
	                            QObject* target, const char* slot,
	                            const QString& shortcut="", const QString& altShortcut="");

	//! convenience methods to add an action (call to own slot) to the StelActionMgr object.
	//! @param id unique identifier. Should be called actionMy_Action. (i.e., start with "action" and then "Capitalize_Underscore" style.)
	//! @param groupId string to be used in the Help menu. The action will be listed in this group.
	//! @param text short translatable description what the action does.
	//! @param slot name of slot in target recipient
	//! @param shortcut default shortcut. Can be reconfigured.
	//! @param altShortcut default alternative shortcut. Can be reconfigured.
	class StelAction* addAction(const QString& id, const QString& groupId, const QString& text,
	                            const char* slot,
	                            const QString& shortcut="", const QString& altShortcut="") {
		return addAction(id, groupId, text, this, slot, shortcut, altShortcut);
	}

	//! convenience methods to add an action (call to Lambda functor) to the StelActionMgr object.
	//! @param id unique identifier. Should be called actionMy_Action. (i.e., start with "action" and then "Capitalize_Underscore" style.)
	//! @param groupId string to be used in the Help menu. The action will be listed in this group.
	//! @param text short translatable description what the action does.
	//! @param contextObject The lambda will only be called if this object exists. Use "this" in most cases.
	//! @param lambda a C++11 Lambda function.
	//! @param shortcut default shortcut. Can be reconfigured.
	//! @param altShortcut default alternative shortcut. Can be reconfigured.
	StelAction* addAction(const QString& id, const QString& groupId, const QString& text,
						QObject* contextObject, std::function<void()> lambda,
						const QString& shortcut="", const QString& altShortcut="");
};

Q_DECLARE_METATYPE(StelModule::StelModuleSelectAction)

#endif // STELMODULE_HPP
