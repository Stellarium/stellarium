/* 
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
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

#ifndef _NAVIGATION_H_
#define _NAVIGATION_H_

#include "planet_mgr.h"

// *******  Calc the Zenith position for the location and Time  ********
void ComputeZenith(void);
void Update_variables(void);
void Update_time(Planet_mgr &_SolarSystem);
void Switch_to_altazimutal(void);
void Switch_to_equatorial(void);
void Move_To(vec3_t _aim);

#endif //_NAVIGATION_H_
