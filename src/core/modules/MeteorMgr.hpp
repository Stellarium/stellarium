/*
 * Stellarium
 * This file Copyright (C) 2004 Robert Spearman
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

#ifndef _METEORMGR_HPP_
#define _METEORMGR_HPP_

#include <vector>
#include "StelModule.hpp"

class Meteor;

//! @class MeteorMgr
//! Simulates a meteor shower.
class MeteorMgr : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(int ZHR
		   READ getZHR
		   WRITE setZHR
		   NOTIFY zhrChanged)

public:
	//! Construct a MeteorMgr object.
	//! @param zhr the base zenith hourly rate - i.e. the rate when there is no
	//!            meteor shower in progress.
	//! @param maxv the initial value of the maximum meteor velocity.
	MeteorMgr(int zhr, int maxv); 
	virtual ~MeteorMgr();
 
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the MeteorMgr object.
	virtual void init();
	
	//! Draw meteors.
	virtual void draw(StelCore* core, class StelRenderer* renderer);
	
	//! Update time-dependent parts of the module.
	//! This function adds new meteors to the list of currently visiable
	//! ones based on the current rate, and removes those which have run their 
	//! course.
	virtual void update(double deltaTime);
	
	//! Defines the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
public slots:
	///////////////////////////////////////////////////////////////////////////
	// Method callable from script and GUI
	//! Get the current zenith hourly rate.
	int getZHR(void);
	//! Set the zenith hourly rate.
	void setZHR(int zhr);
	
	//! Set flag used to turn on and off meteor rendering.
	void setFlagShow(bool b) { flagShow = b; }
	//! Get value of flag used to turn on and off meteor rendering.
	bool getFlagShow(void) const { return flagShow; }
	
	//! Set the maximum velocity in km/s
	void setMaxVelocity(int maxv);
	
signals:
	void zhrChanged(int);
	
private:
	std::vector<Meteor*> active;		// Vector containing all active meteors
	int ZHR;
	int maxVelocity;
	double zhrToWsr;  // factor to convert from zhr to whole earth per second rate
	bool flagShow;
};


#endif // _METEORMGR_HPP_
