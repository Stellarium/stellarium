/*
 * Stellarium
 * Copyright (C) 2013 Guillaume Chereau
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

#include "StelActionMgr.hpp"
#include "StelTranslator.hpp"
#include "StelApp.hpp"

#include <QVariant>
#include <QDebug>
#include <QMetaProperty>
#include <QStringList>
#include <QSettings>

#ifndef USE_QUICKVIEW
#include <QAction>
#include "StelMainView.hpp"
#endif

StelAction::StelAction(const QString& actionId,
		       const QString& groupId,
		       const QString& text,
		       const QString& primaryKey,
		       const QString& altKey,
		       bool global)
	: QObject(StelApp::getInstance().getStelActionManager())
	, group(groupId)
	, text(text)
	, global(global)
	, keySequence(primaryKey)
	, altKeySequence(altKey)
	, defaultKeySequence(primaryKey)
	, defaultAltKeySequence(altKey)
	, target(Q_NULLPTR)
	, boolProperty(Q_NULLPTR)
#ifndef USE_QUICKVIEW
	, qAction(Q_NULLPTR)
#endif
{
	setObjectName(actionId);
	// Check the global conf for custom shortcuts.
	QSettings* conf = StelApp::getInstance().getSettings();
	QString cfgOpt = "shortcuts/" + actionId;
	if (conf->contains(cfgOpt)) // Check existence of shortcut to allow removing shortcuts
	{
		QStringList shortcuts = conf->value(cfgOpt).toString().split(QRegExp("\\s+")); // empty shortcuts allows stay primary and alternative shortcuts as they was defined
		if (shortcuts.size() > 2)
			qWarning() << actionId << ": does not support more than two shortcuts per action";		
		setShortcut(shortcuts[0]);
		if (shortcuts.size() > 1)
			setAltShortcut(shortcuts[1]);		
	}
#ifndef USE_QUICKVIEW
	QWidget* mainView = &StelMainView::getInstance();
	qAction = new QAction(this);
	onChanged();
	mainView->addAction(qAction);
	connect(qAction, SIGNAL(triggered()), this, SLOT(trigger()));
	connect(this, SIGNAL(changed()), this, SLOT(onChanged()));
#endif
}

#ifndef USE_QUICKVIEW
void StelAction::onChanged()
{
	qAction->setShortcuts(QList<QKeySequence>() << keySequence << altKeySequence);
	qAction->setShortcutContext(global ? Qt::ApplicationShortcut : Qt::WidgetShortcut);
}
#endif

void StelAction::setShortcut(const QString& key)
{
	keySequence = QKeySequence(key);
	emit changed();
}

void StelAction::setAltShortcut(const QString& key)
{
	altKeySequence = QKeySequence(key);
	emit changed();
}

QString StelAction::getText() const
{
	return q_(text);
}

void StelAction::setChecked(bool value)
{
	Q_ASSERT(boolProperty);
	if (value == isChecked()) //don't do anything if value would not change
		return;

	boolProperty->setValue(value);
	if(!boolProperty->canNotify())
		emit toggled(value); //if connected to a property without NOTIFY, we have to toggle the event ourselves
}

void StelAction::toggle()
{
	setChecked(!isChecked());
}

void StelAction::trigger()
{
	if (isCheckable())
		toggle();
	else
	{
		//We do not call the slot here, but let Qt handle it through a
		//connection established in ::connectToObject from the triggered() signal
		//to the slot
		//This should still call the target slot first before all other registered slots
		//(because it is registered first, see https://doc.qt.io/qt-4.8/signalsandslots.html#signals).
		//This enables the slot to find out the StelAction that was triggered using sender(),
		//and enables use of QSignalMapper (obsolete), Lambda functions or similar constructs
		emit triggered();
	}
}

void StelAction::connectToObject(QObject* obj, const char* slot)
{
	target = obj;
	QVariant prop = obj->property(slot);
	if (prop.isValid()) // Connect to a bool property, use a StelProperty if possible
	{
		Q_ASSERT(prop.type() == QVariant::Bool);

		// Listen to the property notified signal if there is one.
		int propIndex = obj->metaObject()->indexOfProperty(slot);
		QMetaProperty metaProp = obj->metaObject()->property(propIndex);

		//can use a StelProperty for the connection, property name is action name
		boolProperty = StelApp::getInstance().getStelPropertyManager()->registerProperty(objectName(),obj,slot);
		StelPropertyBoolProxy* prox = new StelPropertyBoolProxy(boolProperty,this);
		connect(prox,SIGNAL(propertyChanged(bool)),this,SLOT(propertyChanged(bool)));

		if(!metaProp.hasNotifySignal())
		{
			//warn about missing notify
			qWarning()<<"[StelAction]"<<getId()<<"is connected to property"<<slot<<"of object"<<obj<<" without a NOTIFY signal";
		}

		return;
	}

	//now, check if its a slot
	int slotIndex = obj->metaObject()->indexOfMethod(slot);
#ifndef NDEBUG
	if (slotIndex<0) qWarning()<<"[StelAction]"<<getId()<<"cannot connect to slot"<<slot<<"of object"<<obj << "- EXIT!";
#endif
	Q_ASSERT(slotIndex>=0);
	// connect to a parameter-less slot.
	this->slot = obj->metaObject()->method(slotIndex);
	Q_ASSERT(this->slot.parameterCount() == 0);
	//let Qt handle invoking the slot when the StelAction is triggered
	int signalIndex = metaObject()->indexOfSignal("triggered()");
	connect(this,metaObject()->method(signalIndex),obj,this->slot);

	emit changed();
}

void StelAction::propertyChanged(bool value)
{
	emit toggled(value);
}

QKeySequence::SequenceMatch StelAction::matches(const QKeySequence& seq) const
{
	Q_ASSERT(QKeySequence::PartialMatch > QKeySequence::NoMatch);
	Q_ASSERT(QKeySequence::ExactMatch > QKeySequence::PartialMatch);
	return qMax((!keySequence.isEmpty() ? keySequence.matches(seq) : QKeySequence::NoMatch),
		    (!altKeySequence.isEmpty() ? altKeySequence.matches(seq) : QKeySequence::NoMatch));
}

StelActionMgr::StelActionMgr() :
	actionsEnabled(true)
{
}

StelActionMgr::~StelActionMgr()
{
}

StelAction* StelActionMgr::addAction(const QString& id, const QString& groupId, const QString& text,
				     QObject* target, const char* slot,
				     const QString& shortcut, const QString& altShortcut,
				     bool global)
{
	StelAction* action = new StelAction(id, groupId, text, shortcut, altShortcut, global);
	connect(action,SIGNAL(toggled(bool)),this,SLOT(onStelActionToggled(bool)));
	action->connectToObject(target, slot);
	return action;
}


StelAction* StelActionMgr::addAction(const QString& id, const QString& groupId, const QString& text,
				      QObject* context, std::function<void()> lambda,
				      const QString& shortcut, const QString& altShortcut,
				      bool global)
{
	StelAction* action = new StelAction(id, groupId, text, shortcut, altShortcut, global);
	connect(action, &StelAction::triggered, context, lambda);
	return action;
}

StelAction* StelActionMgr::findAction(const QString& id)
{
	return findChild<StelAction*>(id);
}

StelAction* StelActionMgr::findActionFromShortcut(const QString& shortcut)
{
	StelAction* ret=Q_NULLPTR;
	for (auto* action : findChildren<StelAction*>())
	{
		if ((action->getShortcut().toString()==shortcut) || (action->getAltShortcut().toString()==shortcut))
			ret=action;
	}
	return ret;
}

bool StelActionMgr::pushKey(int key, bool global)
{
	if (!actionsEnabled)
		return false;
	keySequence << key;
	QKeySequence sequence(keySequence.size() > 0 ? keySequence[0] : 0,
			      keySequence.size() > 1 ? keySequence[1] : 0,
			      keySequence.size() > 2 ? keySequence[2] : 0,
			      keySequence.size() > 3 ? keySequence[3] : 0);
	bool hasPartialMatch = false;
	for (auto* action : findChildren<StelAction*>())
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
	for (auto* action : findChildren<StelAction*>())
	{
		if (!ret.contains(action->group))
			ret.append(action->group);
	}
	return ret;
}

QList<StelAction*> StelActionMgr::getActionList(const QString& group) const
{
	QList<StelAction*> ret;
	for (auto* action : findChildren<StelAction*>())
	{
		if (action->group == group)
			ret.append(action);
	}
	return ret;
}

QList<StelAction*> StelActionMgr::getActionList() const
{
	return findChildren<StelAction*>();
}

QStringList StelActionMgr::getShortcutsList() const
{
	QStringList shortcuts;
	QString shortcut;
	for (const auto* action : getActionList())
	{
		shortcut = action->getShortcut().toString();
		if (!shortcut.isEmpty())
			shortcuts.append(shortcut);
		shortcut = action->getAltShortcut().toString();
		if (!shortcut.isEmpty())
			shortcuts.append(shortcut);
	}
	return shortcuts;
}

void StelActionMgr::saveShortcuts()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("shortcuts");
	conf->remove("");
	for (auto* action : findChildren<StelAction*>())
	{
		if (action->keySequence == action->defaultKeySequence &&
		    action->altKeySequence == action->defaultAltKeySequence)
			continue;
		QString seq = action->keySequence.toString().replace(" ", "");
		if (seq.isEmpty()) // Let's allow remove shortcuts
			seq = "\"\"";
		if (action->altKeySequence != action->defaultAltKeySequence)
			seq += " " + action->altKeySequence.toString().replace(" ", "");
		if (action->altKeySequence.toString()=="")
			seq += " \"\"";		
		conf->setValue(action->objectName(), seq.replace(QRegExp("\\s+")," "));
	}
	conf->endGroup();
	// Apparently shortcuts was changed
	emit shortcutsChanged();
}

void StelActionMgr::restoreDefaultShortcuts()
{
	for (auto* action : findChildren<StelAction*>())
	{
		action->keySequence = action->defaultKeySequence;
		action->altKeySequence = action->defaultAltKeySequence;
		emit action->changed();
	}
	saveShortcuts();
}

void StelActionMgr::restoreDefaultShortcut(StelAction* action)
{
	action->keySequence = action->defaultKeySequence;
	action->altKeySequence = action->defaultAltKeySequence;
	emit action->changed();
	saveShortcuts();
}

void StelActionMgr::onStelActionToggled(bool val)
{
	StelAction* action = qobject_cast<StelAction*>(sender());
	emit actionToggled(action->getId(),val);
}
