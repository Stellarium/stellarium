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
public:
	StelObjectMgr();
	virtual ~StelObjectMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void draw(StelCore*) {;}
	virtual void update(double) {;}

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
	bool findAndSelect(const StelCore* core, int x, int y, StelModule::StelModuleSelectAction action=StelModule::ReplaceSelection);

	//! Find and select an object from its translated name.
	//! @param action define whether to add to, replace, or remove from the existing selection
	//! @param nameI18n the case sensitive object translated name
	//! @return true if a object with the passed name was found
	bool findAndSelectI18n(const QString &nameI18n, StelModule::StelModuleSelectAction action=StelModule::ReplaceSelection);

	//! Find and select an object from its standard program name.
	//! @param action define whether to add to, replace, or remove from the existing selection
	//! @param name the case sensitive object translated name
	//! @return true if a object with the passed name was found
	bool findAndSelect(const QString &name, StelModule::StelModuleSelectAction action=StelModule::ReplaceSelection);

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
	//! @return true if at least 1 object was sucessfully selected
	bool setSelectedObject(const StelObjectP obj, StelModule::StelModuleSelectAction action=StelModule::ReplaceSelection);

	//! Notify that we want to select the given objects.
	//! @param objs a vector of objects to select
	//! @param action define whether to add to, replace, or remove from the existing selection
	//! @return true if at least 1 object was sucessfully selected
	bool setSelectedObject(const QList<StelObjectP>& objs, StelModule::StelModuleSelectAction action=StelModule::ReplaceSelection);

	//! Get the list of objects which was recently selected by the user.
	const QList<StelObjectP>& getSelectedObject() const {return lastSelectedObjects;}

	//! Return the list objects of type "withType" which was recently selected by the user.
	//! @param type return only objects of the given type
	QList<StelObjectP> getSelectedObject(const QString& type) const;

	//! Set whether a pointer is to be drawn over selected object.
	void setFlagSelectedObjectPointer(bool b) {objectPointerVisibility=b;}
	//! Get whether a pointer is to be drawn over selected object.
	bool getFlagSelectedObjectPointer(void) const {return objectPointerVisibility;}

	//! Find any kind of object by its translated name.
	StelObjectP searchByNameI18n(const QString &name) const;

	//! Find any kind of object by its standard program name.
	StelObjectP searchByName(const QString &name) const;

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

public slots:
	//! Set simulation time to the time of next transit of selected object
	void nextTransit();
	//! Set simulation time to the time of previous transit of selected object
	void previousTransit();
	//! Set simulation time to the time of today's transit of selected object
	void todayTransit();

	//! Set simulation time to the time of next rising of selected object (if applicable)
	void nextRising();
	//! Set simulation time to the time of previous rising of selected object (if applicable)
	void previousRising();
	//! Set simulation time to the time of today's rising of selected object (if applicable)
	void todayRising();

	//! Set simulation time to the time of next setting of selected object (if applicable)
	void nextSetting();
	//! Set simulation time to the time of previous setting of selected object (if applicable)
	void previousSetting();
	//! Set simulation time to the time of today's setting of selected object (if applicable)
	void todaySetting();

signals:
	//! Indicate that the selected StelObjects has changed.
	//! @param action define if the user requested that the objects are added to the selection or just replace it
	void selectedObjectChanged(StelModule::StelModuleSelectAction);

private:
	// The list of StelObjectModule that are referenced in Stellarium
	QList<StelObjectModule*> objectsModules;
	QMap<QString, StelObjectModule*> typeToModuleMap;
	QMap<QString, QString> objModulesMap;

	// The last selected object in stellarium
	QList<StelObjectP> lastSelectedObjects;
	// Should selected object pointer be drawn
	bool objectPointerVisibility;

	//! Find in a "clever" way an object from its equatorial position.
	StelObjectP cleverFind(const StelCore* core, const Vec3d& pos) const;

	//! Find in a "clever" way an object from its screen position.
	StelObjectP cleverFind(const StelCore* core, int x, int y) const;

	// Radius in pixel in which objects will be searched when clicking on a point in sky.
	double searchRadiusPixel;

	// Weight of the distance factor when choosing the best object to select.
	float distanceWeight;
};

#endif // _SELECTIONMGR_HPP
