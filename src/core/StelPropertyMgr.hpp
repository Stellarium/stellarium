/*
 * Copyright (C) 2015 Florian Schaukowitsch
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

#ifndef _STELPROPERTYMGR_HPP_
#define _STELPROPERTYMGR_HPP_

#include <QObject>
#include <QMetaProperty>

class StelProperty;

//! Abstract base class for the property proxies which allow reacting to the
//! StelProperty::changed event using a specific type, instead using the QVariant version.
//! This is required for some connections such as in the UI.
//! The intented use is to subclass this class and implement the onPropertyChanged() slot,
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

//! Wrapper around a Q_PROPERTY of a specific object, which provides access to the property through a unique ID,
//! similar to how StelAction works with boolean properties - but with arbitrary data.
//! The use of this class allows easy access to non-boolean Stellarium settings
//! for GUI data-binding (see StelDialog for binding functions) and enables external interfaces, such scripting or
//! the @ref remoteControl plugin to access and change the property dynamically.
//!
//! To register a new StelProperty, use the StelPropertyMgr, which can be retrieved using StelApp::getStelPropertyManager().
//! In StelModule subclasses, one can also use StelModule::registerProperty for convenience.
//!
//! The linked Q_PROPERTY is required to have at least the READ function and the NOTIFY signal correctly implemented.
//! If a property has the WRITE function (or is defined as MEMBER), its value can be set through this class.
//! For optimal results, when changing the property (e.g. through the WRITE slot), it should be checked if the value
//! really has changed before emitting the NOTIFY signal to prevent unnecessary processing.
//! StelProperty supports all data types that work with Q_PROPERTY and QVariant. When using custom types,
//! Q_DECLARE_METATYPE and/or qRegisterMetaType() may be needed.
//!
//! Each StelProperty is identified by an unique QString ID. The current convention for these IDs is:
//! <tt>prop_<ClassName>_<PropertyName></tt>. "Stel" may be ommitted from the \c ClassName for the core (non-StelModule) classes,
//! for example the StelProperty representing the Q_PROPERTY \ref StelSkyDrawer::absoluteStarScale "absoluteStarScale"
//! of the StelSkyDrawer is identified by \c prop_SkyDrawer_absoluteStarScale.
//!
//! A full example using a StelModule:
//! \code
//! class MyCustomModule : public StelModule
//! {
//!		Q_OBJECT
//!		Q_PROPERTY(int awesomeProperty READ getAwesomeProperty WRITE setAwesomeProperty NOTIFY awesomePropertyChanged)
//!
//!	public:
//!		...
//!		void init() Q_DECL_OVERRIDE
//!		{
//!			...
//!			registerProperty("prop_MyCustomModule_awesomeProperty", "awesomeProperty");
//!			...
//!		}
//!
//!		void printProperty()
//!		{
//!			qDebug()<<prop;
//!		}
//!	public slots:
//!		int getAwesomeProperty() const { return prop; }
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
//!	StelProperty* prop = StelApp::getInstance().getStelPropertyManager()->getProperty("prop_MyCustomModule_awesomeProperty");
//!	prop->setValue(123);
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
//! current view vector, field of view etc. They may cause considerable overhead if used.
//! @sa StelPropertyMgr, StelDialog
class StelProperty : public QObject
{
	friend class StelPropertyMgr;
	Q_OBJECT

public slots:
	//! Returns the unique ID which is used to identify this property
	QString getId() const { return objectName(); }

	//! Returns the current value of this property as a QVariant
	QVariant getValue() const;

	//! Sets the value of the property. This can only be used if
	//! isReadOnly is false, meaning a WRITE function is defined on the Q_PROPERTY.
	//! @return true if the new value was successfully set
	bool setValue(const QVariant& value) const;

	//! If false, setValue can be used.
	bool isReadOnly() const;

	//! Returns the data type of the StelProperty
	QMetaType::Type getType() const;

	//! Returns the actual Q_PROPERTY wrapped by this instance
	QMetaProperty getMetaProp() const { return prop; }

	//! Returns the object to which this property belongs
	QObject* getTarget() const { return target; }
signals:
	//! Emitted when the value of the property changed
	void changed(const QVariant& newValue);
private slots:
	//! Reacts to NOTIFY signals from the object, and emits the changed signal
	void propertyChanged();
private:
	StelProperty(const QString& id,QObject* target, const char* propId);

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
	//! Use StelApp::getStelPropertyManager to get the global instance
	StelPropertyMgr();
	~StelPropertyMgr();

	//! Registers a new StelProperty.
	//! @param id The identifier of the property. Must be unique, app will exit otherwise. It should follow
	//! the naming conventions as described in StelProperty.
	//! @param target The QObject which contains the property.
	//! @param propertyName The name of the Q_PROPERTY on the \c target which should be linked
	//! @returns a new StelProperty linking to the property with name \c propertyName on the \c target object
	StelProperty* registerProperty(const QString& id, QObject* target, const char* propertyName);

	//! Returns all currently registered StelProperty instances
	QList<StelProperty*> getAllProperties() const;
	//! Returns the StelProperty with the specified ID, or NULL if not registered
	StelProperty* getProperty(const QString& id) const;
private slots:
	void onStelPropChanged(const QVariant& val);
signals:
	//! Emitted when any registered StelProperty has been changed
	//! @param id The unique id of the property that was changed
	//! @param value The new value of the property
	void stelPropChanged(const QString& id, const QVariant& value);
};

#endif
