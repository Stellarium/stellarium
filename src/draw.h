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

#ifndef __AFFICHAGEH__
#define __AFFICHAGEH__

#include "stellarium.h"
#include "stel_utility.h"
#include "tone_reproductor.h"

void InitMeriParal(void);
void drawIntro(void);
void DrawPoint(float X,float Y,float Z);
void DrawPointer(Vec3d,float,vec3_t,int);

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
//void DrawAtmosphere(void);
void DrawGround(float sky_brightness);
void DrawMenu(void);
void DrawInfo(void);

void Oriente(void);

void DrawHelp(void);

#endif
