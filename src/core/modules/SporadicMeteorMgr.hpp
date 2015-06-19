/*
 * Stellarium
 * Copyright (C) 2015 Marcos Cardinot
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

#ifndef _SPORADICMETEORMGR_HPP_
#define _SPORADICMETEORMGR_HPP_

#include "StelModule.hpp"

class SporadicMeteor;

// Factor to convert from zhr to whole earth per second rate (1.6667f / 3600.f)
#define ZHR_TO_WSR 0.00046297222

//! @class SporadicMeteorMgr
//! Simulates a sporadic meteor shower, with a random color and a random radiant.
//! @author Marcos Cardinot <mcardinot@gmail.com>
class SporadicMeteorMgr : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(int ZHR READ getZHR WRITE setZHR NOTIFY zhrChanged)

public:
	//! Construct a SporadicMeteorMgr object.
	//! @param zhr the base zenith hourly rate - i.e. the rate when there is no
	//!            meteor shower in progress.
	//! @param maxv the initial value of the maximum meteor velocity.
	SporadicMeteorMgr(int zhr, int maxv);
	virtual ~SporadicMeteorMgr();

	// Methods defined in the StelModule class
	//! Initialize the MeteorMgr object.
	virtual void init();
	//! Draw meteors.
	virtual void draw(StelCore* core);
	//! Update time-dependent parts of the module.
	//! This function adds new meteors to the list of currently visiable
	//! ones based on the current rate, and removes those which have run their 
	//! course.
	virtual void update(double deltaTime);
	//! Defines the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;

public slots:
	// Methods callable from script and GUI
	//! Get the current zenith hourly rate.
	int getZHR(void);
	//! Set the zenith hourly rate.
	void setZHR(int zhr);

	//! Set flag used to turn on and off meteor rendering.
	inline void setFlagShow(bool b) { flagShow = b; }
	//! Get value of flag used to turn on and off meteor rendering.
	inline bool getFlagShow(void) const { return flagShow; }

	//! Set the maximum velocity in km/s
	void setMaxVelocity(int maxv);

signals:
	void zhrChanged(int);

private:
	QList<SporadicMeteor*> activeMeteors;
	StelTextureSP m_bolideTexture;
	int ZHR;
	int maxVelocity;
	bool flagShow;
};

#endif // _SPORADICMETEORMGR_HPP_
