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

#include "grid.h"
#include "s_utility.h"
#include "draw.h"


Grid::Grid(int NbPoint_t) : NbPoints(NbPoint_t)
{   
    int resolution = 20;
    NbPoints = resolution*resolution;
    Points = new vec3_t[NbPoints];
    
    for(int i=0;i<resolution;i++)
        for(int j=0;j<resolution;j++)
        {
            RADE_to_XYZ((double)i*2.*PI/resolution ,(double)j*PI/resolution-PI/2 , Points[i*resolution+j]);
            Points[i*resolution+j].Normalize();
        }

    Angle = 2*PI/resolution;
   
}

Grid::~Grid()
{
    delete Points;
}

int Grid::GetNearest(vec3_t v)
{
    int bestI = -1;
    float bestDot = -2.;

	vec3_t v2(v);
	v2.Normalize();
    for(int i=0;i<NbPoints;i++)
    {
        if (v2.Dot(Points[i])>bestDot)
        {
            bestI = i;
            bestDot = v2.Dot(Points[i]);
        }
    }
    return bestI;
}

void Grid::Draw(void)
{
	for(int i=0;i<NbPoints;i++)
    {
    	DrawPoint(Points[i][0],Points[i][1],Points[i][2]);
	}
}

int Grid::Intersect(vec3_t pos, float fieldAngle, int * &result)
{
	if (result!=NULL) delete result;
	result = new int[NbPoints];

	float max = cos(Angle/2+fieldAngle/2);

	int nbResult=0;
	for(int i=0;i<NbPoints;i++)
	{
		if(pos.Dot(Points[i]) > max)
		{
			result[nbResult]=i;
			nbResult++;
		}
	}
	return nbResult;
}