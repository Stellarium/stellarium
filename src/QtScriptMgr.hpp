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

#ifndef _QTSCRIPTMGR_HPP_
#define _QTSCRIPTMGR_HPP_

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
	
// These functions will be available in scripts
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
	
	//! Pauses the script for t seconds
	void wait(double t);
};
		
//! Manage scripting in Stellarium
class QtScriptMgr : public QObject
{
Q_OBJECT
		
public:
    QtScriptMgr(QObject *parent = 0);
    ~QtScriptMgr();

	//! Run the script located at the given location
	//! @param fileName the location of the file containing the script.
	void runScript(const QString& fileName);
	
private slots:
	//! Called at the end of the running thread
	void scriptEnded();

private:
	QScriptEngine engine;
	
	//! The thread in which scripts are run
	class StelScriptThread* thread;
};

#endif // _QTSCRIPTMGR_HPP_
