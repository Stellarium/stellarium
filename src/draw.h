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

// Class which manages a grid to display in the sky
class SkyGrid
{
public:
	// Create and precompute positions of a SkyGrid
	SkyGrid(unsigned int _nb_meridian = 24, unsigned int _nb_parallel = 17,
			unsigned int _nb_alt_segment = 18, unsigned int nb_azi_segment = 36);
    virtual ~SkyGrid();
	void draw(draw_utility * du) const;
private:
	unsigned int nb_meridian;
	unsigned int nb_parallel;
	unsigned int nb_alt_segment;
	unsigned int nb_azi_segment;
	Vec3f ** points;
};

void InitMeriParal(void);
void DrawPoint(float X,float Y,float Z);

void DrawCardinaux(draw_utility * du);
void DrawMilkyWay(tone_reproductor * eye);
void DrawMeridiens(void);
void DrawMeridiensAzimut(void);
void DrawParallelsAzimut(void);
void DrawParallels(void);
void DrawEquator(void);
void DrawEcliptic(void);
void DrawFog(float sky_brightness);
void DrawDecor(int, float sky_brightness);
void DrawGround(float sky_brightness);


#endif // __DRAW_H__
