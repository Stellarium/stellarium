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

#ifndef __DRAW_H__
#define __DRAW_H__

#include "stellarium.h"
#include "stel_utility.h"
#include "tone_reproductor.h"
#include "navigator.h"

enum SKY_GRID_TYPE
{
	EQUATORIAL,
	ALTAZIMUTAL
};

// Class which manages a grid to display in the sky
class SkyGrid
{
public:
	// Create and precompute positions of a SkyGrid
	SkyGrid(SKY_GRID_TYPE grid_type = EQUATORIAL, unsigned int _nb_meridian = 24, unsigned int _nb_parallel = 17,
	 double _radius = 1., unsigned int _nb_alt_segment = 18, unsigned int _nb_azi_segment = 50);
    virtual ~SkyGrid();
	void draw(Projector* prj) const;
private:
	unsigned int nb_meridian;
	unsigned int nb_parallel;
	double radius;
	unsigned int nb_alt_segment;
	unsigned int nb_azi_segment;
	bool transparent_top;
	Vec3f color;
	Vec3f** alt_points;
	Vec3f** azi_points;
	bool (Projector::*proj_func)(const Vec3f&, Vec3d&);
};


enum SKY_LINE_TYPE
{
	EQUATOR,
	ECLIPTIC,
	LOCAL
};

// Class which manages a line to display around the sky like the ecliptic line
class SkyLine
{
public:
	// Create and precompute positions of a SkyGrid
	SkyLine(SKY_LINE_TYPE line_type = EQUATOR, double _radius = 1., unsigned int _nb_segment = 24);
    virtual ~SkyLine();
	void draw(Projector* prj) const;
private:
	double radius;
	unsigned int nb_segment;
	Vec3f color;
	Vec3f* points;
	bool (Projector::*proj_func)(const Vec3f&, Vec3d&);
};


void DrawPoint(float X,float Y,float Z);

void DrawCardinaux(Projector * prj);
void DrawMilkyWay(tone_reproductor * eye);
//void DrawEquator(void);
//void DrawEcliptic(void);
void DrawFog(float sky_brightness);
void DrawDecor(int, float sky_brightness);
void DrawGround(float sky_brightness);


#endif // __DRAW_H__
