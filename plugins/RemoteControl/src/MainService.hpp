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

#ifndef MAINSERVICE_HPP
#define MAINSERVICE_HPP

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
class StelProperty;
class StelScriptMgr;
class StelSkyCultureMgr;

//! @ingroup remoteControl
//! Implements the main API services, including the \c status operation which can be repeatedly polled to find the current state of the main program,
//! including time, view, location, StelAction and StelProperty state changes, movement, script status ...
//!
//! @see @ref rcMainService
class MainService : public AbstractAPIService
{
	Q_OBJECT

	Q_ENUMS(SelectionMode)
public:
	enum SelectionMode
	{
		Center,
		Zoom,
		Mark
	};

	MainService(QObject* parent = Q_NULLPTR);

	//! Used to implement move functionality
	virtual void update(double deltaTime) Q_DECL_OVERRIDE;
	virtual QLatin1String getPath() const Q_DECL_OVERRIDE { return QLatin1String("main"); }
	//! @brief Implements the GET operations
	//! @see @ref rcMainServiceGET
	virtual void get(const QByteArray& operation,const APIParameters &parameters, APIServiceResponse& response) Q_DECL_OVERRIDE;
	//! @brief Implements the HTTP POST operations
	//! @see @ref rcMainServicePOST
	virtual void post(const QByteArray &operation, const APIParameters &parameters, const QByteArray &data, APIServiceResponse &response) Q_DECL_OVERRIDE;

private slots:
	StelObjectP getSelectedObject();

	//! Returns the info string of the currently selected object
	QString getInfoString();

	//! Like StelDialog::gotoObject
	bool focusObject(const QString& name, SelectionMode mode);
	void focusPosition(const Vec3d& pos);

	void updateMovement(double x, double y, bool xUpdated, bool yUpdated);
	// Allow azimut/altitude changes. Values must be in Radians.
	void updateView(double az, double alt, bool azUpdated, bool altUpdated);
	void setFov(double fov);

	void actionToggled(const QString& id, bool val);
	void propertyChanged(StelProperty* prop, const QVariant &val);

private:
	StelCore* core;
	StelActionMgr* actionMgr;
	LandscapeMgr* lsMgr;
	StelLocaleMgr* localeMgr;
	StelMovementMgr* mvmgr;
	StelObjectMgr* objMgr;
	StelPropertyMgr* propMgr;
#ifdef ENABLE_SCRIPTING
	StelScriptMgr* scriptMgr;
#endif
	StelSkyCultureMgr* skyCulMgr;

	double moveX,moveY;
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
