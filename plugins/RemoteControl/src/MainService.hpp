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
class NebulaMgr;
class StelLocaleMgr;
class StelMovementMgr;
class StelObjectMgr;
class StelScriptMgr;
class StelSkyCultureMgr;

class MainService : public AbstractAPIService
{
	Q_OBJECT
public:
	MainService(const QByteArray& serviceName, QObject* parent = 0);

	virtual ~MainService() {}

	virtual void update(double deltaTime) Q_DECL_OVERRIDE;

protected:

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

private:
	StelCore* core;
	StelActionMgr* actionMgr;
	LandscapeMgr* lsMgr;
	NebulaMgr* nebMgr;
	StelLocaleMgr* localeMgr;
	StelMovementMgr* mvmgr;
	StelObjectMgr* objMgr;
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

};



#endif
