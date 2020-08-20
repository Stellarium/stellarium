/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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
 
#include "StelModule.hpp"
#include "StelApp.hpp"
#include "StelActionMgr.hpp"

StelModule::StelModule()
{
	// Attempt to set the default object name to the class name
	// However, this does not work, and every derived class MUST call setObjectName(className) in its constructor by itself.
	setObjectName(metaObject()->className()); // lgtm [cpp/virtual-call-in-constructor]
}

/*************************************************************************
 Get the version of the module, default is stellarium main version
*************************************************************************/
QString StelModule::getModuleVersion() const
{
	return PACKAGE_VERSION;
}

class StelAction* StelModule::addAction(const QString& id, const QString& groupId, const QString& text,
                                        QObject* target, const char* slot,
                                        const QString& shortcut, const QString& altShortcut)
{
	StelActionMgr* mgr = StelApp::getInstance().getStelActionManager();
	return mgr->addAction(id, groupId, text, target, slot, shortcut, altShortcut);
}

 StelAction* StelModule::addAction(const QString& id, const QString& groupId, const QString& text,
					QObject* contextObject, std::function<void()> lambda,
					const QString& shortcut, const QString& altShortcut)
{
	StelActionMgr* mgr = StelApp::getInstance().getStelActionManager();
	return mgr->addAction(id, groupId, text, contextObject, lambda, shortcut, altShortcut);
}
