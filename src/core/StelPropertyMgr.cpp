#include "StelPropertyMgr.hpp"
#include "StelApp.hpp"
#include <QDebug>

StelProperty::StelProperty(const QString &id, QObject* target, const char* propId)
	: QObject(StelApp::getInstance().getStelPropertyManager()),
	  target(target), proxy(NULL)
{
	//check if this property name is already defined, print an error if it is
	if(parent()->findChild<StelProperty*>(id,Qt::FindDirectChildrenOnly))
	{
		qFatal("Fatal error: StelProperty '%s' has already been registered!", qUtf8Printable(id));
	}
	setObjectName(id);

	Q_ASSERT(target);
	const QMetaObject* metaObj = target->metaObject();
	int propIdx = metaObj->indexOfProperty(propId);
	if(propIdx==-1)
	{
		qFatal("Fatal error: No Q_PROPERTY '%s' registered on class '%s'", propId, qUtf8Printable(metaObj->className()));
	}

	prop = metaObj->property(propIdx);
	//check if the property is valid and has a NOTIFY signal
	if(!prop.isValid())
	{
		qFatal("Fatal error: Q_PROPERTY '%s' on class '%s' has no READ or MEMBER definition", propId, qUtf8Printable(metaObj->className()));
	}
	if(!prop.hasNotifySignal())
	{
		qFatal("Fatal error: Q_PROPERTY '%s' on class '%s' has no NOTIFY signal", propId, qUtf8Printable(metaObj->className()));
	}
	//qDebug()<<prop.name()<<prop.type()<<prop.typeName();

	//determine if a property proxy can be used
	if(prop.isEnumType())
		proxy = new StelPropertyIntProxy(this,this);
	else
	{
		switch (getType()) {
			case QMetaType::UInt:
			case QMetaType::Int:
				proxy = new StelPropertyIntProxy(this,this);
				break;
			case QMetaType::Double:
			case QMetaType::Float:
				proxy = new StelPropertyDoubleProxy(this,this);
				break;
			case QMetaType::Bool:
				proxy = new StelPropertyBoolProxy(this,this);
				break;
			default:
				//No proxy supported
				break;
		}
	}
	connect(target, prop.notifySignal(), this, metaObject()->method(metaObject()->indexOfSlot("propertyChanged()")));
}

StelPropertyProxy* StelProperty::getProxy() const
{
	return proxy;
}

QVariant StelProperty::getValue() const
{
	return prop.read(target);
}

bool StelProperty::setValue(const QVariant &value) const
{
	return prop.write(target,value);
}

bool StelProperty::isReadOnly() const
{
	return !prop.isWritable();
}

QMetaType::Type StelProperty::getType() const
{
	//Qt is quite funky when it comes to QVariant::Type vs QMetaType::Type, see
	//https://doc.qt.io/qt-5/qvariant.html#type
	//and https://stackoverflow.com/questions/31290606/qmetatypefloat-not-in-qvarianttype
	return static_cast<QMetaType::Type>(prop.type());
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
	StelProperty* prop = new StelProperty(id,target,propertyName);
	//check if the property is valid, crash otherwise
	//this may reveal if a qRegisterMetaType or similar is needed
	QVariant value = prop->getValue();
	if(!value.isValid())
		qFatal("StelProperty %s can not be read. Missing READ or need to register MetaType?",qUtf8Printable(id));

	connect(prop, SIGNAL(changed(QVariant)), this, SLOT(onStelPropChanged(QVariant)));
	qDebug()<<"StelProperty"<<id<<"registered, value"<<value;
	return prop;
}

QList<StelProperty*> StelPropertyMgr::getAllProperties() const
{
	return findChildren<StelProperty*>(QString(),Qt::FindDirectChildrenOnly);
}

StelProperty* StelPropertyMgr::getProperty(const QString &id) const
{
	StelProperty* prop = findChild<StelProperty*>(id,Qt::FindDirectChildrenOnly);
	if(!prop)
		qWarning()<<"StelProperty"<<id<<"not found";
	return prop;
}

void StelPropertyMgr::onStelPropChanged(const QVariant &val)
{
	StelProperty* prop = qobject_cast<StelProperty*>(sender());
	qDebug()<<"StelProperty"<<prop->getId()<<"changed, value"<<val;
	emit stelPropChanged(prop->getId(),val);
}

StelPropertyProxy::StelPropertyProxy(StelProperty *prop, QObject *parent)
	: QObject(parent), prop(prop)
{
	connect(prop, &StelProperty::changed, this, &StelPropertyProxy::onPropertyChanged);
}

StelPropertyIntProxy::StelPropertyIntProxy(StelProperty *prop, QObject *parent)
	: StelPropertyProxy(prop,parent)
{

}

void StelPropertyIntProxy::onPropertyChanged(const QVariant &value)
{
	emit propertyChanged(value.toInt());
}

StelPropertyBoolProxy::StelPropertyBoolProxy(StelProperty *prop, QObject *parent)
	: StelPropertyProxy(prop,parent)
{

}

void StelPropertyBoolProxy::onPropertyChanged(const QVariant &value)
{
	emit propertyChanged(value.toBool());
}

StelPropertyDoubleProxy::StelPropertyDoubleProxy(StelProperty *prop, QObject *parent)
	: StelPropertyProxy(prop,parent)
{

}

void StelPropertyDoubleProxy::onPropertyChanged(const QVariant &value)
{
	emit propertyChanged(value.toDouble());
}
