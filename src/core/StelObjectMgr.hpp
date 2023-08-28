/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
 * Copyright (C) 2016 Marcos Cardinot
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

#ifndef STELOBJECTMGR_HPP
#define STELOBJECTMGR_HPP

#include "VecMath.hpp"
#include "StelModule.hpp"
#include "StelObject.hpp"

#include <QList>
#include <QString>

class StelObjectModule;
class StelCore;

//! @class StelObjectMgr
//! Manage the selection and queries on one or more StelObjects.
//! Each module is then free to manage object selection as it wants.
class StelObjectMgr : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool objectPointerVisibility
		   READ getFlagSelectedObjectPointer
		   WRITE setFlagSelectedObjectPointer
		   NOTIFY flagSelectedObjectPointerChanged)

public:
	StelObjectMgr();
	virtual ~StelObjectMgr() Q_DECL_OVERRIDE;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init() Q_DECL_OVERRIDE;

	///////////////////////////////////////////////////////////////////////////
	//! Add a new StelObject manager into the list of supported modules.
	//! Registered modules can have selected objects
	void registerStelObjectMgr(StelObjectModule* m);

	//! Find and select an object near given equatorial J2000 position.
	//! @param core the StelCore instance to use for computations
	//! @param pos the direction vector around which to search in equatorial J2000
	//! @param action define whether to add to, replace, or remove from the existing selection
	//! @return true if a object was found at position (this does not necessarily means it is selected)
	bool findAndSelect(const StelCore* core, const Vec3d& pos, StelModule::StelModuleSelectAction action=StelModule::ReplaceSelection);

	//! Find and select an object near given screen position.
	//! @param core the StelCore instance to use for computations
	//! @param x the x screen position in pixel
	//! @param y the y screen position in pixel
	//! @param action define whether to add to, replace, or remove from the existing selection
	//! @return true if a object was found at position (this does not necessarily means it is selected)
	//! @note If aberration is computed, this first applies aberration backwards and then searches for an object.
	bool findAndSelect(const StelCore* core, int x, int y, StelModule::StelModuleSelectAction action=StelModule::ReplaceSelection);

	//! Find and select an object from its translated name.
	//! @param action define whether to add to, replace, or remove from the existing selection
	//! @param nameI18n the case sensitive object translated name
	//! @return true if a object with the passed name was found
	bool findAndSelectI18n(const QString &nameI18n, StelModule::StelModuleSelectAction action=StelModule::ReplaceSelection);

	//! Find and select an object from its translated name and object type name (i.e., the Stellarium class name).
	//! @param action define whether to add to, replace, or remove from the existing selection
	//! @param nameI18n the case sensitive object translated name
	//! @param objtype the type of the object (i.e., the Stellarium class name)
	//! @return true if a object with the passed name was found
	bool findAndSelectI18n(const QString &nameI18n, const QString &objtype, StelModule::StelModuleSelectAction action=StelModule::ReplaceSelection);

	//! Find and select an object from its standard program name.
	//! @param action define whether to add to, replace, or remove from the existing selection
	//! @param name the case sensitive object translated name
	//! @return true if a object with the passed name was found
	bool findAndSelect(const QString &name, StelModule::StelModuleSelectAction action=StelModule::ReplaceSelection);

	//! Find and select an object from its standard program name and object type name (i.e., the Stellarium class name).
	//! @param action define whether to add to, replace, or remove from the existing selection
	//! @param name the case sensitive object translated name
	//! @param objtype the type of the object (i.e., the Stellarium class name)
	//! @return true if a object with the passed name was found
	bool findAndSelect(const QString &name, const QString &objtype, StelModule::StelModuleSelectAction action=StelModule::ReplaceSelection);

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names.
	//! @param useStartOfWords the autofill mode for returned objects names
	//! @return a list of matching object names by order of relevance, or an empty list if nothing match
	QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const;

	QStringList listAllModuleObjects(const QString& moduleId, bool inEnglish) const;
	QMap<QString, QString> objectModulesMap() const;

	//! Return whether an object was selected during last selection related event.
	bool getWasSelected(void) const {return !lastSelectedObjects.empty();}

	//! Notify that we want to unselect any object.
	void unSelect(void);

	//! Notify that we want to select the given object.
	//! @param obj the StelObject to select
	//! @param action action define whether to add to, replace, or remove from the existing selection
	//! @return true if at least 1 object was successfully selected
	bool setSelectedObject(const StelObjectP obj, StelModule::StelModuleSelectAction action=StelModule::ReplaceSelection);

	//! Notify that we want to select the given objects.
	//! @param objs a vector of objects to select
	//! @param action define whether to add to, replace, or remove from the existing selection
	//! @return true if at least 1 object was successfully selected
	bool setSelectedObject(const QList<StelObjectP>& objs, StelModule::StelModuleSelectAction action=StelModule::ReplaceSelection);

	//! Get the list of objects which was recently selected by the user.
	const QList<StelObjectP>& getSelectedObject() const {return lastSelectedObjects;}

	//! Return the list objects of type "type" which was recently selected by the user.
	//! @param type return only objects of the given type
	QList<StelObjectP> getSelectedObject(const QString& type) const;

	//! Find any kind of object by its translated name.
	StelObjectP searchByNameI18n(const QString &name) const;

	//! Find any kind of object by its translated name and its object type name.
	StelObjectP searchByNameI18n(const QString &name, const QString &objType) const;

	//! Find any kind of object by its standard program name.
	StelObjectP searchByName(const QString &name) const;

	//! Find any kind of object by its standard program name and its object type name.
	StelObjectP searchByName(const QString &name, const QString &objType) const;

	//! Find an object of the given type and ID
	//! @param type the type of the object as given by StelObject::getType()
	//! @param id the ID of the object as given by StelObject::getID()
	//! @return an null/invalid pointer when nothing is found, the given object otherwise.
	//! @note
	//! a StelObject may be found by multiple IDs (different catalog numbers, etc),
	//! so StelObject::getID() of the returned object may not be the same as the query parameter \p id.
	StelObjectP searchByID(const QString& type, const QString& id) const;

	//! Set the radius in pixel in which objects will be searched when clicking on a point in sky.
	void setObjectSearchRadius(double radius) {searchRadiusPixel=radius;}

	//! Set the weight of the distance factor when choosing the best object to select.
	//! Default to 1.
	void setDistanceWeight(float newDistanceWeight) {distanceWeight=newDistanceWeight;}

	//! Return a QMap of data about the object (calls obj->getInfoMap()).
	//! If obj is valid, add an element ["found", true].
	//! If obj is Q_NULLPTR, returns a 1-element map [["found", false]]
	static QVariantMap getObjectInfo(const StelObjectP obj);

	//! Return a list of enabled fields (custom info strings)
	StelObject::InfoStringGroup getCustomInfoStrings();

	//! Retrieve an (unsorted) QStringList of all extra info strings that match flags.
	//! Normally the order matches the order of addition, but this cannot be guaranteed.
	QStringList getExtraInfoStrings(const StelObject::InfoStringGroup& flags) const;

public slots:

	//! @note These functions were copied over from StelObject. Given that setExtraInfoString is non-const and some functions where these methods are useful are const, we can use the StelObjectMgr as "carrier object".
	//! Allow additions to the Info String. Can be used by plugins to show extra info for the selected object, or for debugging.
	//! Hard-set this string group to a single str, or delete all messages when str==""
	//! @note This should be used with caution. Usually you want to use addToExtraInfoString().
	virtual void setExtraInfoString(const StelObject::InfoStringGroup& flags, const QString &str);

	//! Add str to the extra string. This should be preferable over hard setting.
	//! Can be used by plugins to show extra info for the selected object, or for debugging.
	//! The strings will be shown in the InfoString for the selected object, below the default fields per-flag.
	//! Additional coordinates not fitting into one of the predefined coordinate sets should be flagged with OtherCoords,
	//! and must be adapted to table or non-table layout as required.
	//! The line ending must be given explicitly, usually just end a line with "<br/>", except when it may end up in a Table or appended to a line.
	//! See getCommonInfoString() or the respective getInfoString() in the subclasses for details of use.
	virtual void addToExtraInfoString(const StelObject::InfoStringGroup& flags, const QString &str);

	//! Remove the extraInfoStrings with the given flags.
	//! This is a finer-grained removal than just extraInfoStrings.remove(flags), as it allows a combination of flags.
	//! After display, InfoPanel::setTextFromObjects() auto-clears the strings of the selected object using the AllInfo constant.
	//! extraInfoStrings having been set with the DebugAid and Script flags have to be removed by separate calls of this method.
	//! Those which have been set by scripts have to persist at least as long as the selection remains active.
	//! The behaviour of DebugAid texts depends on the use case.
	void removeExtraInfoStrings(const StelObject::InfoStringGroup& flags);

	//! Set whether a pointer is to be drawn over selected object.
	void setFlagSelectedObjectPointer(bool b)
	{
		objectPointerVisibility=b;
		emit flagSelectedObjectPointerChanged(b);

	}
	//! Get whether a pointer is to be drawn over selected object.
	bool getFlagSelectedObjectPointer(void) { return objectPointerVisibility; }

signals:
	//! Indicate that the selected StelObjects has changed.
	//! @param action define if the user requested that the objects are added to the selection or just replace it
	void selectedObjectChanged(StelModule::StelModuleSelectAction);
	//! Indicate that the visibility of pointer for selected StelObjects has changed.
	void flagSelectedObjectPointerChanged(bool b);

private:
	// The list of StelObjectModule that are referenced in Stellarium
	QList<StelObjectModule*> objectsModules;
	QMap<QString, StelObjectModule*> typeToModuleMap;
	QMap<QString, QString> objModulesMap;

	// The last selected object in stellarium
	QList<StelObjectP> lastSelectedObjects;
	// Should selected object pointer be drawn
	bool objectPointerVisibility;

	//! Find in a "clever" way an object from its equatorial J2000.0 position.
	StelObjectP cleverFind(const StelCore* core, const Vec3d& pos) const;

	//! Find in a "clever" way an object from its screen position.
	//! @note If aberration is computed, this first applies aberration backwards and then searches for an object.
	StelObjectP cleverFind(const StelCore* core, int x, int y) const;

	//! Radius in pixel in which objects will be searched when clicking on a point in sky.
	double searchRadiusPixel;

	//! Weight of the distance factor when choosing the best object to select.
	float distanceWeight;

	//! Location for additional object info that can be set for special purposes (at least for debugging, but maybe others), even via scripting.
	//! Modules are allowed to add new strings to be displayed in the various getInfoString() methods of subclasses.
	//! This helps avoiding screen collisions if a plugin wants to display some additional object information.
	//! This string map gets cleared by InfoPanel::setTextFromObjects(), with the exception of strings with Script or DebugAid flags,
	//! which have been injected by scripts or for debugging (take care of those yourself!).
	QMultiMap<StelObject::InfoStringGroup, QString> extraInfoStrings;
};

#endif // _SELECTIONMGR_HPP
