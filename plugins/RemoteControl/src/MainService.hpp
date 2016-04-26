/*
 * Stellarium Remote Control plugin
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

#ifndef MAINSERVICE_HPP_
#define MAINSERVICE_HPP_

#include "AbstractAPIService.hpp"

#include "StelObjectType.hpp"
#include "VecMath.hpp"

#include <QContiguousCache>
#include <QJsonObject>
#include <QMutex>

class StelCore;
class StelActionMgr;
class LandscapeMgr;
class StelLocaleMgr;
class StelMovementMgr;
class StelObjectMgr;
class StelPropertyMgr;
class StelScriptMgr;
class StelSkyCultureMgr;

//! Implements the main API services, including the \c status operation which can be repeatedly polled to find the current state of the main program,
//! including time, view, location, StelAction and StelProperty state changes, movement, script status ...
//!
//! ## Service methods
//! @copydetails getImpl()
//! @copydetails postImpl()
class MainService : public AbstractAPIService
{
	Q_OBJECT
public:
	MainService(const QByteArray& serviceName, QObject* parent = 0);

	virtual ~MainService() {}

	//! Used to implement move functionality
	virtual void update(double deltaTime) Q_DECL_OVERRIDE;

protected:
	//! @brief Implements the GET method
	//! @details
	//! ### GET operations
	//! #### status
	//! Parameters: <tt>[actionId (Number)] [propId (Number)]</tt>\n
	//! This operation can be polled every few moments to find out if some primary Stellarium state changed. It returns a JSON object with the following format:
	//! @code{.js}
	//! {
	//!	//current location information, see StelLocation
	//!	location : {
	//!		name,
	//!		role,
	//!		planet,
	//!		latitude,
	//!		longitude,
	//!		altitude,
	//!		country,
	//!		state,
	//!		landscapeKey
	//!	},
	//!	//current time information
	//!	time : {
	//!		jday,		//current Julian day
	//!		deltaT,		//current deltaT as determined by the current dT algorithm
	//!		gmtShift,	//the timezone shift to GMT
	//!		timeZone,	//the timezone name
	//!		utc,		//the time in UTC time zone as ISO8601 time string
	//!		local,		//the time in local time zone as ISO8601 time string
	//!		isTimeNow,	//if true, the Stellarium time equals the current real-world time
	//!		timerate	//the current time rate (in secs)
	//!	},
	//!	script : {
	//!		scriptIsRunning,	//boolean, true if a script is running
	//!		runningScriptId		//the ID of the running script
	//!	},
	//!	selectioninfo, //string that contains the information of the currently selected object, as returned by StelObject::getInfoString
	//!	view : {
	//!		fov,		//current FOV
	//!		projection,	//current projection key
	//!		projectionStr,	//current projection string in current language
	//!		landscape,	//current landscape ID
	//!		skyculture	//current skyculture ID
	//!	},
	//!
	//!	//the following is only inserted if an actionId parameter was given
	//!	//see below for more info
	//!	actionChanges : {
	//!		id, //currently valid action id, the interface should update its own id to this value
	//!		changes : {
	//!			//a list of boolean actions that changed since the actionId parameter
	//!			<actionName> : <actionValue>
	//!		}
	//!	},
	//!	//the following is only inserted if an propId parameter was given
	//!	//see below for more info
	//!	propertyChanges : {
	//!		id, //currently valid prop id, the interface should update its own id to this value
	//!		changes : {
	//!			//a list of properties that changed since the propId parameter
	//!			<propName> : <propValue>
	//!		}
	//!	}
	//! }
	//! @endcode
	//!
	//! The \c actionChanges and \c propertyChanges sections allow a remote interface to track boolean StelAction and/or StelProperty changes.
	//! On the initial poll, you should pass -2 as \p propId and \p actionId. This indicates to the service that you want a full
	//! list of properties/actions and their current values. When receiving the answer, you should set your local \p propId /\p actionId to the id
	//! contained in \c actionChanges and \c propertyChanges, and re-send it with the next request as parameter again.
	//! This allows the MainService to find out which changes must be sent to you (it maintains a queue of action/property changes internally, incrementing
	//! the ID with each change), and you only have to process the differences instead of everything.
	//! #### plugins
	//! Returns the list of all known plugins, as a JSON object of format:
	//! @code{.js}
	//! {
	//!	//list of known plugins, in format:
	//!	<pluginName> : {
	//!		loadAtStartup,	//if to load the plugin at startup
	//!		loaded,		//if the plugin is currently loaded
	//!		//corresponds to the StelPluginInfo of the plugin
	//!		info : {
	//!			authors,
	//!			contact,
	//!			description,
	//!			displayedName,
	//!			startByDefault,
	//!			version
	//!		}
	//!	}
	//! }
	//! @endcode
	virtual void getImpl(const QByteArray& operation,const APIParameters &parameters, APIServiceResponse& response) Q_DECL_OVERRIDE;
	virtual void postImpl(const QByteArray &operation, const APIParameters &parameters, const QByteArray &data, APIServiceResponse &response) Q_DECL_OVERRIDE;

private slots:
	StelObjectP getSelectedObject();

	//! Returns the info string of the currently selected object
	QString getInfoString();

	//! Like StelDialog::gotoObject
	bool focusObject(const QString& name);
	void focusPosition(const Vec3d& pos);

	void updateMovement(int x, int y, bool xUpdated, bool yUpdated);
	void setFov(double fov);

	void actionToggled(const QString& id, bool val);
	void propertyChanged(const QString& id, const QVariant& val);

private:
	StelCore* core;
	StelActionMgr* actionMgr;
	LandscapeMgr* lsMgr;
	StelLocaleMgr* localeMgr;
	StelMovementMgr* mvmgr;
	StelObjectMgr* objMgr;
	StelPropertyMgr* propMgr;
	StelScriptMgr* scriptMgr;
	StelSkyCultureMgr* skyCulMgr;

	int moveX,moveY;
	qint64 lastMoveUpdateTime;

	struct ActionCacheEntry
	{
		ActionCacheEntry(const QString& str,bool val) : action(str),val(val) {}
		QString action;
		bool val;
	};

	//lists the recently toggled actions - this is a pseudo-circular buffer
	QContiguousCache<ActionCacheEntry> actionCache;
	QMutex actionMutex;
	QJsonObject getActionChangesSinceID(int changeId);

	struct PropertyCacheEntry
	{
		PropertyCacheEntry(const QString& str, const QVariant& val) : id(str),val(val) {}
		QString id;
		QVariant val;
	};
	QContiguousCache<PropertyCacheEntry> propCache;
	QMutex propMutex;
	QJsonObject getPropertyChangesSinceID(int changeId);

};



#endif
