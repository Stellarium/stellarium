
#include "StelActionMgr.hpp"

#include <QVariant>
#include <QDebug>
#include <QMetaProperty>
#include <QStringList>

StelAction::StelAction(const QString& actionId,
					   const QString& groupId,
					   const QString& text,
					   const QString& primaryKey,
					   bool global):
	checkable(false),
	checked(false),
	group(groupId),
	text(text),
	global(global),
	target(NULL),
	property(NULL)
{
	setObjectName(actionId);
	keySequence = QKeySequence(primaryKey);
}

void StelAction::setShortcut(const QString& key)
{
	keySequence = QKeySequence(key);
}

void StelAction::setAltShortcut(const QString& key)
{
	altKeySequence = QKeySequence(key);
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
	if (prop.isValid()) // Connect to a property.
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
		return;
	}
	int slotIndex = obj->metaObject()->indexOfMethod(slot);
	if (obj->metaObject()->method(slotIndex).parameterCount() == 1)
	{
		// connect to a boolean slot.
		Q_ASSERT(obj->metaObject()->method(slotIndex).parameterType(0) == QMetaType::Bool);
		checkable = true;
		int signalIndex = metaObject()->indexOfMethod("toggled(bool)");
		connect(this, metaObject()->method(signalIndex), obj, obj->metaObject()->method(slotIndex));
	}
	else
	{
		// connect to a slot.
		Q_ASSERT(obj->metaObject()->method(slotIndex).parameterCount() == 0);
		checkable = false;
		int signalIndex = metaObject()->indexOfMethod("triggered()");
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

QKeySequence::SequenceMatch StelAction::matches(const QKeySequence& seq) const
{
	Q_ASSERT(QKeySequence::PartialMatch > QKeySequence::NoMatch);
	Q_ASSERT(QKeySequence::ExactMatch > QKeySequence::PartialMatch);
	return qMax((!keySequence.isEmpty() ? keySequence.matches(seq) : QKeySequence::NoMatch),
				(!altKeySequence.isEmpty() ? altKeySequence.matches(seq) : QKeySequence::NoMatch));
}

StelActionMgr::StelActionMgr()
{
}

StelActionMgr::~StelActionMgr()
{
	
}

StelAction* StelActionMgr::addAction(const QString& id, const QString& groupId, const QString& text,
					  const QString& shortcut, QObject* target, const char* slot, bool global)
{
	StelAction* action = new StelAction(id, groupId, text, shortcut, global);
	action->connectToObject(target, slot);
	action->setParent(this);
	return action;
}

StelAction* StelActionMgr::findAction(const QString& id)
{
	return findChild<StelAction*>(id);
}

bool StelActionMgr::pushKey(int key, bool global)
{
	keySequence << key;
	QKeySequence sequence(keySequence.size() > 0 ? keySequence[0] : 0,
						  keySequence.size() > 1 ? keySequence[1] : 0,
						  keySequence.size() > 2 ? keySequence[2] : 0,
						  keySequence.size() > 3 ? keySequence[3] : 0);
	bool hasPartialMatch = false;
	foreach(StelAction* action, findChildren<StelAction*>())
	{
		if (global && !action->global) continue;
		QKeySequence::SequenceMatch match = action->matches(sequence);
		if (match == QKeySequence::ExactMatch)
		{
			keySequence.clear();
			action->trigger();
			return true;
		}
		hasPartialMatch = hasPartialMatch || match == QKeySequence::PartialMatch;
	}
	if (!hasPartialMatch)
		keySequence.clear();
	return false;
}

QStringList StelActionMgr::getGroupList() const
{
	QStringList ret;
	foreach(StelAction* action, findChildren<StelAction*>())
	{
		if (!ret.contains(action->group))
			ret.append(action->group);
	}
	return ret;
}

QList<StelAction*> StelActionMgr::getActionList(const QString& group) const
{
	QList<StelAction*> ret;
	foreach(StelAction* action, findChildren<StelAction*>())
	{
		if (action->group == group)
			ret.append(action);
	}
	return ret;
}
