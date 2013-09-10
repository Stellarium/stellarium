
#include "StelActionMgr.hpp"

#include <QVariant>
#include <QDebug>
#include <QMetaProperty>

StelAction::StelAction(const QString& actionId,
					   const QString& groupId,
					   const QString& text,
					   const QString& primaryKey,
					   const QString& altKey,
					   bool checkable,
					   bool autoRepeat,
					   bool global)
	:checkable(false), checked(false), target(NULL), property(NULL)
{
	setObjectName(actionId);
	keySequence = QKeySequence(primaryKey);
}

void StelAction::setChecked(bool value)
{
	Q_ASSERT(checkable);
	if (value == checked)
		return;
	checked = value;
	if (target)
		target->setProperty(property, checked);
	emit toggled(checked);
}

void StelAction::toggle()
{
	setChecked(!checked);
}

void StelAction::trigger()
{
	if (checkable)
		toggle();
	else
		emit triggered();
}

void StelAction::connectToObject(QObject* obj, const char* slot)
{
	QVariant prop = obj->property(slot);
	if (prop.isValid())
	{
		checkable = true;
		target = obj;
		property = slot;
		checked = prop.toBool();
		// Listen to the property notified signal if there is one.
		int propIndex = obj->metaObject()->indexOfProperty(slot);
		QMetaProperty metaProp = obj->metaObject()->property(propIndex);
		int slotIndex = metaObject()->indexOfMethod("propertyChanged(bool)");
		if (metaProp.hasNotifySignal())
			connect(obj, metaProp.notifySignal(), this, metaObject()->method(slotIndex));
	}
	else
	{
		checkable = false;
		int signalIndex = metaObject()->indexOfMethod("triggered()");
		int slotIndex = obj->metaObject()->indexOfMethod(slot);
		connect(this, metaObject()->method(signalIndex), obj, obj->metaObject()->method(slotIndex));
	}
}

void StelAction::propertyChanged(bool value)
{
	if (value == checked)
		return;
	checked	= value;
	emit toggled(checked);
}

StelActionMgr::StelActionMgr()
{
}

StelActionMgr::~StelActionMgr()
{
	
}

StelAction* StelActionMgr::addAction(const QString& id, const QString& groupId, const QString& text,
					  const QString& shortcut, QObject* target, const char* slot)
{
	StelAction* action = new StelAction(id, groupId, text, shortcut);
	action->connectToObject(target, slot);
	action->setParent(this);
	return action;
}

StelAction* StelActionMgr::findAction(const QString& id)
{
	return findChild<StelAction*>(id);
}

StelAction* StelActionMgr::pushKey(int key)
{
	keySequence << key;
	QKeySequence sequence(keySequence.size() > 0 ? keySequence[0] : 0,
						  keySequence.size() > 1 ? keySequence[1] : 0,
						  keySequence.size() > 2 ? keySequence[2] : 0,
						  keySequence.size() > 3 ? keySequence[3] : 0);
	bool hasPartialMatch = false;
	foreach(StelAction* action, findChildren<StelAction*>())
	{
		if (action->keySequence.isEmpty()) continue;
		QKeySequence::SequenceMatch match = action->keySequence.matches(sequence);
		if (match == QKeySequence::ExactMatch)
		{
			keySequence.clear();
			return action;
		}
		hasPartialMatch = hasPartialMatch || match == QKeySequence::PartialMatch;
	}
	if (!hasPartialMatch)
		keySequence.clear();
	return NULL;
}
