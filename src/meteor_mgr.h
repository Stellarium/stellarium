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
#include <functional>

#include "projector.h"
#include "navigator.h"
#include "meteor.h"
#include "stelmodule.h"

class MeteorMgr : public StelModule
{

 public:
  MeteorMgr(int zhr, int maxv );  // base_zhr is zenith hourly rate sans meteor shower
  virtual ~MeteorMgr();
  virtual string getModuleID() const { return "meteors"; }
  void set_ZHR(int zhr);   // set zenith hourly rate
  int get_ZHR(void);   
  void set_max_velocity(int maxv);   // set maximum meteoroid velocity km/s
  virtual void update(double delta_time);          // update positions
  virtual double draw(Projector* prj, const Navigator * nav, ToneReproductor* eye);

 private:
  vector<Meteor*> active;		// Vector containing all active meteors
  int ZHR;
  int max_velocity;
  double zhr_to_wsr;  // factor to convert from zhr to whole earth per second rate
};


#endif // _METEOR_MGR_H
