/*
 * Stellarium
 * Copyright (C) 2009 Matthew Gates
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

#ifndef _STELMAINSCRIPTAPIPROXY_HPP_
#define _STELMAINSCRIPTAPIPROXY_HPP_

#include <QObject>

//! @class StelMainScriptAPIProxy
//! Because the core API runs in a different thread to the main program, 
//! direct function calls to some classes can cause problems - especially 
//! when images must be loaded, or other non-atomic operations are involved.
//!
//! This class acts as a proxy - running in the Main thread.  Connect signals
//! from the StelMainScriptAPI to the instance of this class running in the
//! main thread and let the slots do the work which is not possible within
//! StelMainScriptAPI itself.
//! 
//! Please follow the following convention:
//! member in StelMainScriptAPI:      someSlot(...)
//! signal in StelMainScriptAPI:      requestSomeSlot(...)
//! member in StelMainScriptAPIProxy: someSlot(...)
//!
//! The dis-advantage of this method is that there is no way to get a return
//! value.  This is because of how the signal/slot mechanism works.
class StelMainScriptAPIProxy : public QObject
{
	Q_OBJECT

public:
	StelMainScriptAPIProxy(QObject* parent=0) : QObject(parent) {;}
	~StelMainScriptAPIProxy() {;}

public slots:
	void setDiskViewport(bool b);

};

#endif // _STELMAINSCRIPTAPIPROXY_HPP_

