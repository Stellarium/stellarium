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

#ifndef _CONSTELLATION_H_
#define _CONSTELLATION_H_

#include "hip_star_mgr.h"
#include "stellarium.h"
#include "stel_utility.h"
#include "s_font.h"

class Constellation
{
    friend class Constellation_mgr;
public:
    Constellation();
    virtual ~Constellation();
    int read(FILE *, Hip_Star_mgr * _VouteCeleste);
    void draw(Projector* prj);
    void draw_alone(Projector* prj) const;
    void draw_name(s_font * constfont) const;
	const Constellation* is_star_in(const Hip_Star *) const;
private:
    char * name;
    char short_name[4];
    char * inter;
    Vec3f XYZname;
	Vec3d XYname;
    unsigned int nb_segments;
    Hip_Star ** asterism;
};

#endif // _CONSTELLATION_H_
