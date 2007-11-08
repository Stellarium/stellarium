/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef QTSCRIPTMGR_HPP
#define QTSCRIPTMGR_HPP

#include <QObject>
#include <QtScript>
#include "vecmath.h"

	
//! Provide script API for Stellarium global functions
class StelMainScriptAPI : public QObject
{
	Q_OBJECT
	Q_PROPERTY(double JDay READ getJDay WRITE setJDay)
	Q_PROPERTY(double timeSpeed READ getTimeSpeed WRITE setTimeSpeed)
					
public:
	StelMainScriptAPI(QObject *parent = 0);
	~StelMainScriptAPI();
	
// Theses functions will be available in scripts
public slots:
	//! Set the current date in Julian Day
	//! @param JD the Julian Date
	void setJDay(double JD);
	//! Get the current date in Julian Day
	//! @return the Julian Date
	double getJDay(void) const;
		
	//! Set time speed in JDay/sec
	//! @param ts time speed in JDay/sec
	void setTimeSpeed(double ts);
	//! Get time speed in JDay/sec
	//! @return time speed in JDay/sec
	double getTimeSpeed(void) const;
};
		
//! Manage scripting in Stellarium
class QtScriptMgr : public QObject
{
Q_OBJECT
		
public:
    QtScriptMgr(QObject *parent = 0);
    ~QtScriptMgr();
	
	void test();
	
private:
	QScriptEngine engine;
};

#endif
