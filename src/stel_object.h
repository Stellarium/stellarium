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

#ifndef _STEL_OBJECT_H_
#define _STEL_OBJECT_H_

#include "vecmath.h"

#define STEL_OBJECT_STAR 1
#define STEL_OBJECT_PLANET 2
#define STEL_OBJECT_NEBULA 3

class stel_object
{
public:
	stel_object();
    virtual ~stel_object();
	virtual void update(void) {return;}
	virtual void draw_pointer(int delta_time);
	// find and select the "nearest" object and retrieve his informations
	static stel_object * find_stel_object(int x, int y);
	static stel_object * find_stel_object(Vec3d);
	virtual void get_info_string(char * s);
	virtual unsigned char get_type(void)=0;
	virtual Vec3d get_equ_pos(void)=0;
	virtual vec3_t get_RGB(void) {return vec3_t(0.,0.,0.);}
private:
};


extern stel_object * selected_object; // Selection instance used in stellarium

#endif // _STEL_OBJECT_H_
