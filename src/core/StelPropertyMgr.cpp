#include "StelPropertyMgr.hpp"
#include <QtDebug>
#include <QApplication>

StelProperty::StelProperty(const QString &id, QObject *target, const QMetaProperty& prop)
	: id(id), target(target), prop(prop)
{
	setObjectName(id);
	if(prop.hasNotifySignal())
		connect(target,prop.notifySignal(),this,metaObject()->method(this->metaObject()->indexOfSlot("propertyChanged()")));
}

QVariant StelProperty::getValue() const
{
	return prop.read(target);
}

bool StelProperty::setValue(const QVariant &value) const
{
	//check if the property already has the specified value
	//preventing some unnecessary events
	if(getValue() == value)
		return true;
	return prop.write(target,value);
}

bool StelProperty::isReadOnly() const
{
	return !prop.isWritable();
}

bool StelProperty::isSynchronizable() const
{
	return prop.isWritable() && prop.isStored();
}

bool StelProperty::canNotify() const
{
	return prop.hasNotifySignal();
}

QMetaType::Type StelProperty::getType() const
{
	//Qt is quite funky when it comes to QVariant::Type vs QMetaType::Type, see
	//https://doc.qt.io/qt-5/qvariant.html#type
	//and https://stackoverflow.com/questions/31290606/qmetatypefloat-not-in-qvarianttype
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	return static_cast<QMetaType::Type>(prop.typeId());
#else
	return static_cast<QMetaType::Type>(prop.type());
#endif
}

void StelProperty::propertyChanged()
{
	QVariant val = prop.read(target);
	Q_ASSERT(val.isValid());
	emit changed(val);
}

StelPropertyMgr::StelPropertyMgr()
{
}

StelPropertyMgr::~StelPropertyMgr()
{
}

StelProperty* StelPropertyMgr::registerProperty(const QString& id, QObject* target, const char* propertyName)
{
	int idx = target->metaObject()->indexOfProperty(propertyName);
	if(idx<0)
		qFatal("Property '%s' missing on object '%s' for StelProperty '%s'",propertyName, qPrintable(target->objectName()), qPrintable(id));

	return registerProperty(id,target,target->metaObject()->property(idx));
}

StelProperty* StelPropertyMgr::registerProperty(const QString &id, QObject *target,const QMetaProperty &prop)
{
	//check if the ID is already existing
	if(propMap.contains(id))
		qFatal("StelProperty with id '%s' already existing, please fix...",qPrintable(id));

	StelProperty* stelProp = new StelProperty(id,target,prop);
	stelProp->setParent(this);

	//react to the property changed event
	connect(stelProp,SIGNAL(changed(QVariant)),this,SLOT(onStelPropChanged(QVariant)));

	//check if the property is valid, crash otherwise
	//this may reveal if a qRegisterMetaType or similar is needed
	QVariant value = stelProp->getValue();
	if(!value.isValid())
		qFatal("StelProperty %s can not be read. Missing READ or need to register MetaType?",id.toUtf8().constData());

#if 0
	QString debugStr=(stelProp->isReadOnly()?"readonly":"readwrite");
	if(stelProp->canNotify())
		debugStr.append(" notify");
	qDebug()<<"StelProperty"<<id<<"registered, properties:"<<debugStr<<", value:"<<value;
#endif

	propMap.insert(id,stelProp);
	return stelProp;
}

QList<StelProperty*> StelPropertyMgr::getAllProperties() const
{
	return propMap.values();
}

StelProperty* StelPropertyMgr::getProperty(const QString &id, const bool noWarning) const
{
	StelProperty* prop = propMap.value(id);
	if((!prop) && (!noWarning))
		qWarning()<<"StelProperty"<<id<<"not found";
	return prop;
}

void StelPropertyMgr::onStelPropChanged(const QVariant &val)
{
	StelProperty* prop = qobject_cast<StelProperty*>(sender());
#ifndef NDEBUG
	if (qApp->property("verbose") == true)
		qDebug()<<"StelProperty"<<prop->getId()<<"changed, value"<<val;
#endif
	emit stelPropertyChanged(prop, val);
}

QStringList StelPropertyMgr::getPropertyList() const
{
	return propMap.keys();
}

void StelPropertyMgr::registerObject(QObject *obj)
{
	const QString name = obj->objectName();
	//make sure an object name is set, and does not contain a dot
	if(name.isEmpty() || name.contains('.'))
		qFatal("[StelPropertyMgr] Object name '%s' is invalid, must be non-empty and without a dot", qPrintable(name));

	if(!registeredObjects.contains(name))
	{
		//just a sanity check that we dont have the same object registered under a different name
		Q_ASSERT(registeredObjects.key(obj).isEmpty());

#ifndef NDEBUG
		qDebug()<<"[StelPropertyMgr] Registering object"<<name;
#endif

		//iterate and store the static properties, dynamic ones are skipped
		const QMetaObject* metaObj = obj->metaObject();
		for(int i = metaObj->propertyOffset(); i < metaObj->propertyCount(); ++i)
		{
			QMetaProperty metaProp = metaObj->property(i);
			QString propName = name +'.'+metaProp.name();
			registerProperty(propName,obj,metaProp);
		}

		registeredObjects.insert(name,obj);
	}
}

QVariant StelPropertyMgr::getStelPropertyValue(const QString &id, const bool noWarning) const
{
	StelProperty* prop = getProperty(id, noWarning);
	if(prop)
	{
		return prop->getValue();
	}
	//return an invalid qvariant
	return QVariant();
}

bool StelPropertyMgr::setStelPropertyValue(const QString &id, const QVariant &value) const
{
	StelProperty* prop = getProperty(id);
	if(prop)
	{
		return prop->setValue(value);
	}
	return false;
}

QMetaProperty StelPropertyMgr::getMetaProperty(const QString &id) const
{
	StelProperty* prop = getProperty(id);
	if(prop)
	{
		return prop->getMetaProp();
	}
	//return an invalid metaprop
	return QMetaProperty();
}

StelPropertyProxy::StelPropertyProxy(StelProperty *prop, QObject *parent)
	: QObject(parent), prop(prop)
{
	connect(prop, &StelProperty::changed, this, &StelPropertyProxy::onPropertyChanged);
}

StelPropertyIntProxy::StelPropertyIntProxy(StelProperty *pr, QObject *parent)
	: StelPropertyProxy(pr,parent)
{
}

void StelPropertyIntProxy::onPropertyChanged(const QVariant &value)
{
	emit propertyChanged(value.toInt());
}

StelPropertyBoolProxy::StelPropertyBoolProxy(StelProperty *pr, QObject *parent)
	: StelPropertyProxy(pr,parent)
{
}

void StelPropertyBoolProxy::onPropertyChanged(const QVariant &value)
{
	emit propertyChanged(value.toBool());
}

StelPropertyDoubleProxy::StelPropertyDoubleProxy(StelProperty *pr, QObject *parent)
	: StelPropertyProxy(pr,parent)
{
}

void StelPropertyDoubleProxy::onPropertyChanged(const QVariant &value)
{
	emit propertyChanged(value.toDouble());
}

StelPropertyStringProxy::StelPropertyStringProxy(StelProperty *pr, QObject *parent)
	: StelPropertyProxy(pr,parent)
{
}

void StelPropertyStringProxy::onPropertyChanged(const QVariant &value)
{
	emit propertyChanged(value.toString());
}
