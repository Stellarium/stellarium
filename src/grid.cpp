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

void moy(vec3_t * p, int a, int b, int c)
{
	p[c] = p[a] + p[b];
	p[c].Normalize();
}

Grid::Grid()
{   
		
	NbPoints = 66;
	Points = new vec3_t[NbPoints];
	
	// Starting 6 points
	Points[0].Set(0,0,1);
	Points[1].Set(1,0,0);
	Points[2].Set(0,1,0);
	moy(Points,0,1,3); moy(Points,0,2,4); moy(Points,1,2,5);
	
	// 12 next subdivs
	moy(Points,0,3,6); moy(Points,0,4,7); moy(Points,0,5,8);
	moy(Points,3,1,9); moy(Points,1,5,10); moy(Points,3,5,11);
	moy(Points,4,5,12); moy(Points,2,5,14); moy(Points,2,4,13);

	Points[0]=Points[14]; Points[7]=Points[12]; Points[4]=Points[11]; Points[2]=Points[10];

	int i,j;
	
	for(i=10;i<40;i++)
	{
		Points[i] = Points[i-10];
		Points[i].RotateZ(PI/2);
	}

	// Top half sphere is done
	j=40;
	for(i=0;i<40;i++)
	{
		if(Points[i][2]!=0)
		{
			Points[j] = Points[i];
			Points[j][2]=-Points[j][2];
			j++;
		}
	}
	Points[j++].Set(0,0,1);
	Points[j++].Set(0,0,-1);
	
    Angle = PI/8*1.4;
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