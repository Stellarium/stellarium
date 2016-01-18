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

//! Abstract base class for the property proxies. Property proxies allow to react to the
//! StelProperty::changed event using a specific type, instead of the generic QVariant version
//! This is required for some connections such as in the UI
class StelPropertyProxy : public QObject
{
	Q_OBJECT
public:
	StelPropertyProxy(StelProperty* prop,QObject* parent);

protected slots:
	virtual void onPropertyChanged(const QVariant& value) = 0;

protected:
	StelProperty* prop;
};

//! The property proxy for int-based properties
class StelPropertyIntProxy : public StelPropertyProxy
{
	Q_OBJECT
public:
	StelPropertyIntProxy(StelProperty* prop,QObject* parent);
protected slots:
	void onPropertyChanged(const QVariant &value) Q_DECL_OVERRIDE;
signals:
	void propertyChanged(int value);
};

//! The property proxy for bool-based properties
class StelPropertyBoolProxy : public StelPropertyProxy
{
	Q_OBJECT
public:
	StelPropertyBoolProxy(StelProperty* prop,QObject* parent);
protected slots:
	void onPropertyChanged(const QVariant &value) Q_DECL_OVERRIDE;
signals:
	void propertyChanged(bool value);
};

//! The property proxy for double-based properties
class StelPropertyDoubleProxy : public StelPropertyProxy
{
	Q_OBJECT
public:
	StelPropertyDoubleProxy(StelProperty* prop,QObject* parent);
protected slots:
	void onPropertyChanged(const QVariant &value) Q_DECL_OVERRIDE;
signals:
	void propertyChanged(double value);
};

//! Wrapper around a Q_PROPERTY of a specific object.
//! Provides access to the property through a unique ID, similar to how StelAction works with boolean properties - but with arbitrary data.
//! The Q_PROPERTY is required to have at least the READ function and the NOTIFY signal correctly implemented.
//! If a property has the WRITE function (or is defined as MEMBER), its value can be set through this class.
//!
//! The use of this class may allow easier access to non-boolean Stellarium settings
//! using external interfaces, such as through scripting or the RemoteControl plugin.
class StelProperty : public QObject
{
	friend class StelPropertyMgr;
	Q_OBJECT
public:
	//! If this property type can be supported by a property proxy, it is returned.
	//! Use qobject_cast to cast to the specific type.
	StelPropertyProxy* getProxy() const;
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
	StelPropertyProxy* proxy;
};

//! Manages the registration of specific object properties with the StelProperty system.
class StelPropertyMgr : public QObject
{
	Q_OBJECT
public:
	StelPropertyMgr();
	~StelPropertyMgr();

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
