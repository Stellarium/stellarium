/*
 * Copyright (C) 2015-2016 Florian Schaukowitsch
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

#ifndef STELPROPERTYMGR_HPP
#define STELPROPERTYMGR_HPP

#include <QObject>
#include <QSet>
#include <QMetaProperty>
#include "StelApp.hpp"

class StelProperty;

//! Abstract base class for a StelProperty proxy implementation, which allow reacting to the
//! StelProperty::changed event using a specific type instead of reacting to the QVariant version.
//! This is required for some connections such as in the UI.
//! The intended use is to subclass this class and implement the onPropertyChanged() slot,
//! re-emitting the "changed" event with a type-converted value.
class StelPropertyProxy : public QObject
{
	Q_OBJECT
public:
	StelPropertyProxy(StelProperty* prop,QObject* parent);

protected slots:
	//! This is connected by the constructor to the StelProperty::changed event
	//! of the connected property
	virtual void onPropertyChanged(const QVariant& value) = 0;

protected:
	//! The connected property, set by the constructor
	StelProperty* prop;
};

//! A StelPropertyProxy for int-based properties
class StelPropertyIntProxy : public StelPropertyProxy
{
	Q_OBJECT
public:
	StelPropertyIntProxy(StelProperty* prop,QObject* parent);
protected slots:
	void onPropertyChanged(const QVariant &value) Q_DECL_OVERRIDE;
signals:
	//! Emitted when the property value changes
	void propertyChanged(int value);
};

//! A StelPropertyProxy for bool-based properties
class StelPropertyBoolProxy : public StelPropertyProxy
{
	Q_OBJECT
public:
	StelPropertyBoolProxy(StelProperty* prop,QObject* parent);
protected slots:
	void onPropertyChanged(const QVariant &value) Q_DECL_OVERRIDE;
signals:
	//! Emitted when the property value changes
	void propertyChanged(bool value);
};

//! A StelPropertyProxy for double-based properties
class StelPropertyDoubleProxy : public StelPropertyProxy
{
	Q_OBJECT
public:
	StelPropertyDoubleProxy(StelProperty* prop,QObject* parent);
protected slots:
	void onPropertyChanged(const QVariant &value) Q_DECL_OVERRIDE;
signals:
	//! Emitted when the property value changes
	void propertyChanged(double value);
};

//! A StelPropertyProxy for QString-based properties
class StelPropertyStringProxy : public StelPropertyProxy
{
	Q_OBJECT
public:
	StelPropertyStringProxy(StelProperty* prop,QObject* parent);
protected slots:
	void onPropertyChanged(const QVariant &value) Q_DECL_OVERRIDE;
signals:
	//! Emitted when the property value changes
	void propertyChanged(QString value);
};

//! Wrapper around a Q_PROPERTY (see the [Qt property system](https://doc.qt.io/qt-5/properties.html) for more information) of a specific object, which provides access to the property through a unique ID.
//! A StelProperty basically is a tuple of `<QObject* target, QMetaProperty prop>` identified by a unique string ID
//! that allows to access the property \p prop of the \p target, without the requirement of needing information about the
//! classes, data types and method signatures involved. The StelPropertyMgr acts as a global registry for this information.
//! In some ways, this is similar to how StelAction worked with boolean properties - but with arbitrary data.
//!
//! The use of this class allows easy access to non-boolean Stellarium settings
//! for GUI data-binding (see StelDialog for binding functions) and enables external interfaces, such scripting or
//! the @ref remoteControl plugin to access and change the property dynamically. The main differences to StelAction are:
//! - The most important difference is that StelAction is primarily intended for user actions through keyboard shortcuts and/or GUI buttons.
//!   StelProperty is not intended to be directly used/configurable by users, but rather as a development tool,
//!   to easily access and modify the configurable options of all StelModule objects.
//! - StelProperty always requires a Q_PROPERTY while StelAction can also work directly with
//!   argumentless or boolean slots (i.e. func(), func(bool))
//! - StelAction actually registers and uses a StelProperty internally, if possible. This is the case when
//!   connected to a bool Q_PROPERTY instead of a slot. The created StelProperty has the same ID as the StelAction.
//! - StelProperty uses the NOTIFY handler to find out when the value changes, even if the change originated outside
//!   in non-StelProperty code by calling the WRITE slot directly.
//!   StelAction (if not using a StelProperty, as explained in the previous point) has to track boolean states on its own,
//!   so there may be a mismatch between the state as StelAction sees it and the real state in some cases
//!
//! To register a new StelProperty, use the StelPropertyMgr, which can be retrieved using StelApp::getStelPropertyManager().
//! By default, if you are using a StelModule (and it is registered with the StelModuleMgr), *all* the properties of your object
//! are automatically exposed through StelProperty instances, so in most cases you don't need to do *anything* more than define
//! your Q_PROPERTY correctly.
//!
//! To register all properties of a QObject automatically, you can use StelPropertyMgr::registerObject, which
//! generates a StelProperty with an ID in the format `<objectName>.<propertyName>` for all Q_PROPERTY definitions on the class.
//! Single properties can be registered with StelPropertyMgr::registerProperty.
//!
//! The linked Q_PROPERTY must be valid, that is it has to have at least the READ function and/or a MEMBER definition.
//! Furthermore, it is highly recommended to have the NOTIFY signal correctly implemented, so that interested objects
//! can be notified as soon as the property changes.
//! If a property has the WRITE function (or is defined as MEMBER), its value can be set through this class.
//! For optimal results, when changing the property (e.g. through the WRITE slot), it should be checked if the value
//! really has changed before emitting the NOTIFY signal to prevent unnecessary processing.
//! StelProperty supports all data types that work with Q_PROPERTY and QVariant. When using custom types,
//! Q_DECLARE_METATYPE and/or qRegisterMetaType() may be needed. If custom data types are required to be accessed from
//! outside the program (for example through the [\ref remoteControlDoc](RemoteControl plugin) ), some further work may be required
//! to serialize/deserialize them, but all standard data types (\c bool, \c int, \c double, \c QString ...) work seamlessly out of
//! the box.
//!
//! Each StelProperty is identified by an unique QString ID.
//! Properties registered by StelPropertyMgr::registerObject, which includes all StelModule instances registered with the StelModuleMgr,
//! are identified with an ID in the format `<objectName>.<propertyName>`.
//!
//! A full example how to define a Q_PROPERTY, using a StelModule:
//! \code
//! class MyCustomModule : public StelModule
//! {
//!		Q_OBJECT
//!		Q_PROPERTY(int awesomeProperty READ getAwesomeProperty WRITE setAwesomeProperty NOTIFY awesomePropertyChanged)
//!
//!	public:
//!		MyCustomModule()
//!		{
//!			//this is not even required for a StelModule
//!			//because the base class constructor sets the object name to the class name
//!			//using QMetaObject runtime information
//!			setObjectName("MyCustomModule");
//!		}
//!		...
//!		void init() Q_DECL_OVERRIDE
//!		{
//!			...
//!			// to manually register the property as a StelProperty, this could be used,
//!			// but it is not even required (nor recommended!) because we are in a StelModule!
//!			//registerProperty("my_arbitrary_ID_for_MyCustomModule_awesomeProperty", "awesomeProperty");
//!			...
//!		}
//!
//!		void printProperty()
//!		{
//!			qDebug()<<prop;
//!		}
//!	public slots:
//!		//Returns the current awesomeProperty value
//!		int getAwesomeProperty() const { return prop; }
//!
//!		//Sets the awesomeProperty
//!		//Don't forget to emit the NOTIFY signal if the value actually changed!
//!		void setAwesomeProperty(int val) { if(val!=prop) { prop = val; emit awesomePropertyChanged(val); } }
//!	signals:
//!		void awesomePropertyChanged(int newVal);
//!	private:
//!		int prop;
//! };
//! \endcode
//!
//! After registration, the property can then be used from arbitrary other code, like so:
//! \code
//!	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty("MyCustomModule.awesomeProperty");
//!	prop->setValue(123);  //this performs data type conversion if necessary, using QVariant
//!
//!	//alternatively, use this to skip having to get the StelProperty instance:
//!	//StelApp::getInstance().getStelPropertyManager()->setStelPropertyValue("MyCustomModule.awesomeProperty", 123);
//!     // or even, much shorter, assuming awesomeProperty is an int,
//!     // GETSTELPROPERTYVALUE("MyCustomModule.awesomeProperty").toInt();
//!
//!	//to see the effect
//!	MyCustomModule *module = GETSTELMODULE(MyCustomModule);
//!	module->printProperty(); //prints 123
//! \endcode
//!
//! The changed() signal can be used to detect whenever the property changes. To connect the changed() signal
//! to slots which expect a specific type instead of a QVariant, a StelPropertyProxy implementation can be used.
//! This file defines StelPropertyIntProxy, StelPropertyBoolProxy and StelPropertyDoubleProxy, which use QVariant-based
//! type conversion if necessary.
//!
//! Of course, the whole StelProperty system does not prevent you from directly using/connecting the properties' signals and slots,
//! which is still recommended if you don't need two-way data binding (for example only reading the current value) because of a lower overhead.
//! Other connections using a StelProperty will work seamlessly.
//!
//! StelDialog supports helper methods for two-way binding using common %Qt Widgets and datatypes.
//!
//! @note Good candidates for a StelProperty are properties that do not change too often. This includes most settings
//! configurable through the GUI. Bad examples are properties which potentially change very often (e.g. each frame), such as the
//! current view vector, field of view etc. They may cause considerable overhead if used, and therefore should be avoided.
//! @sa StelPropertyMgr, StelDialog, StelAction
class StelProperty : public QObject
{
	friend class StelPropertyMgr;
	Q_OBJECT

public slots:
	//! Returns the unique ID which is used to identify this property
	QString getId() const { return id; }

	//! Returns the current value of this property as a QVariant
	QVariant getValue() const;

	//! Sets the value of the property. This can only be used if
	//! isReadOnly is false, meaning a WRITE function is defined on the Q_PROPERTY.
	//! Data type conversion is performed, if necessary, using the internal logic of
	//! QVariant. This basically means that QVariant::canConvert() from the given value
	//! to the actual data type of the Q_PROPERTY must be true for the operation to succeed.
	//! @return true if the new value was successfully set
	bool setValue(const QVariant& value) const;

	//! If false, setValue can be used.
	bool isReadOnly() const;

	//! True when this property can be synchronized with external sources.
	//! This is the case when it is writable, and it is marked as \c STORED true (default).
	bool isSynchronizable() const;

	//! If true, the Q_PROPERTY has a NOTIFY signal and the changed() signal can be used
	bool canNotify() const;

	//! Returns the data type of the StelProperty
	QMetaType::Type getType() const;

	//! Returns the actual Q_PROPERTY wrapped by this instance
	QMetaProperty getMetaProp() const { return prop; }

	//! Returns the object to which this property belongs
	QObject* getTarget() const { return target; }
signals:
	//! Emitted when the value of the property changed
	void changed(const QVariant& newValue);
protected slots:
	//! Reacts to NOTIFY signals from the object (ignoring the optional parameter), and emits the changed signal
	void propertyChanged();
protected:
	StelProperty(const QString& id,QObject* target, const QMetaProperty& prop);

	QString id;
	QObject* target;
	QMetaProperty prop;
};

//! Manages the registration of specific object properties with the StelProperty system.
//! A shortcut exists through StelModule::registerProperty.
//! For more information on how to use this system, see the StelProperty class.
class StelPropertyMgr : public QObject
{
	Q_OBJECT
public:
	typedef QMap<QString,StelProperty*> StelPropertyMap;

	//! Use StelApp::getStelPropertyManager to get the global instance
	StelPropertyMgr();
	~StelPropertyMgr();

	//! Manually register a new StelProperty.
	//! @param id The identifier of the property. Must be unique, app will exit otherwise. It should follow
	//! the naming conventions as described in StelProperty.
	//! @param target The QObject which contains the property.
	//! @param propertyName The name of the Q_PROPERTY on the \c target which should be linked
	//! @returns a new StelProperty linking to the property with name \c propertyName on the \c target object
	StelProperty* registerProperty(const QString& id, QObject* target, const char* propertyName);

	//! Registers all Q_PROPERTY definitions on this object as StelProperty.
	//! The object must have an unique QObject::objectName, and this name should never
	//! change after registering. For all properties on this object, a
	//! StelProperty with an ID in the format `<objectName>.<propertyName>` is generated.
	void registerObject(QObject *obj);

	//! Returns the keys of all registered StelProperties
	QStringList getPropertyList() const;
	//! Returns all currently registered StelProperty instances
	QList<StelProperty*> getAllProperties() const;
	//! Returns a map from property IDs to StelProperty objects
	const StelPropertyMap& getPropertyMap() const { return propMap; }

	//! Returns the StelProperty with the specified ID, or Q_NULLPTR if not registered.
	//! If not registered, a warning is written to logfile unless noWarning is true.
	//! This may be useful to suppress warnings about plugin module properties
	//! when these are not loaded, but should else be left true.
	StelProperty* getProperty(const QString& id, const bool noWarning=false) const;

	//! Retrieves the current value of the given StelProperty,
	//! without requiring the retrieval of its object first.
	//! @returns the current value of the StelProperty with the ID \p id,
	//! or an invalid QVariant when no property with the given ID is found.
	//! If not found, a warning is written to logfile unless noWarning is true.
	//! This may be useful to suppress warnings about plugin module properties
	//! when these are not loaded, but should else be left true.
	QVariant getStelPropertyValue(const QString& id, const bool noWarning=false) const;
	//! Sets the value of the given StelProperty,
	//! without requiring the retrieval of its object first.
	//! @returns \c true when the value of the StelProperty with the ID \p id
	//! has been successfully changed, and \c false if the value change failed
	//! or when no property with the given ID is found.
	bool setStelPropertyValue(const QString& id, const QVariant &value) const;
	//! Returns the QMetaProperty information for the given \p id.
	QMetaProperty getMetaProperty(const QString& id) const;
signals:
	//! Emitted when any registered StelProperty has been changed
	//! @param prop The property that was changed
	//! @param value The new value of the property
	void stelPropertyChanged(StelProperty* prop, const QVariant& value);
private slots:
	void onStelPropChanged(const QVariant& val);
private:
	StelProperty* registerProperty(const QString &id, QObject *target, const QMetaProperty& prop);

	QMap<QString,QObject*> registeredObjects;
	StelPropertyMap propMap;
};

#define GETSTELPROPERTYVALUE(pName) StelApp::getInstance().getStelPropertyManager()->getStelPropertyValue(pName, false)

#endif
