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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _METEOR_MGR_H_
#define _METEOR_MGR_H_

#include <vector>
#include "StelModule.hpp"

class Meteor;

//! @class MeteorMgr
//! Simulates meteor showers.
class MeteorMgr : public StelModule
{
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
	//! Takes the meteor rate from the ini parser object.
	//! @param conf the ini parser object.
	//! @param lb the loading bar object.
	virtual void init(const InitParser& conf, LoadingBar& lb);
	
	//! Draw meteors.
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproducer *eye);
	
	//! Update time-dependent parts of the module.
	//! This function adds new meteors to the list of currently visiable
	//! ones based on the current rate, and removes those which have run their 
	//! course.
	virtual void update(double deltaTime);
	
	//! Defines the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
	//! Set the zenith hourly rate.
	void setZHR(int zhr);
	
	//! Get the current zenith hourly rate.
	int getZHR(void);
	
	//! Set the maximum velocity in km/s
	void set_max_velocity(int maxv);
	
private:
	vector<Meteor*> active;		// Vector containing all active meteors
	int ZHR;
	int max_velocity;
	double zhr_to_wsr;  // factor to convert from zhr to whole earth per second rate
	
};


#endif // _METEOR_MGR_H
