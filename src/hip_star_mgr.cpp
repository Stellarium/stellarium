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

// class used to manage groups of Stars

#include "hip_star_mgr.h"
#include "s_texture.h"
#include "grid.h"
#include "bytes.h"

s_texture * hipStarTexture;

Hip_Star_mgr::Hip_Star_mgr() : HipGrid()
{	
    hipStarTexture=NULL;
    Selectionnee=NULL;
}

Hip_Star_mgr::~Hip_Star_mgr()
{   
    multimap<int, Hip_Star *>::iterator iter;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {   
        delete (*iter).second ;
    }

    if (hipStarTexture) delete hipStarTexture;
    hipStarTexture=NULL;
}

// Load from file ( create the stream and call the Read function )
void Hip_Star_mgr::Load(char * NameFichier)
{   
    printf("Loading Hipparcos star data...\n");
    FILE * pFile;
    pFile=fopen(NameFichier,"r");
    if (pFile==NULL)
    {   
        printf("ERROR %s NOT FOUND\n",NameFichier);
        return;
    }
    Hip_Star_mgr::Read(pFile);
    fclose(pFile);
    
    hipStarTexture=new s_texture("etoile16x16Aigrettes");  // Load star texture
}


int Hip_Star_mgr::Read(FILE * catalog)
{   
    unsigned int catalogSize=0;
    fread((char*)&catalogSize,4,1,catalog);
    LE_TO_CPU_INT32(catalogSize,catalogSize);
    
    Hip_Star * e = NULL;
    for(int i=0;i<catalogSize;i++)
    {   
	    e = new Hip_Star;
        e->Read(catalog);
        Liste.insert(pair<int, Hip_Star*>(HipGrid.GetNearest(e->XYZ), e));
    }
	return 0;
}

// Draw all the stars
void Hip_Star_mgr::Draw(void)
{   
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture (GL_TEXTURE_2D, hipStarTexture->getID());

    double z;
    glColor3f(0.5, 1.0, 0.5);
    GLdouble M[16];
    GLdouble P[16];
    GLint V[4];
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    glGetDoublev(GL_PROJECTION_MATRIX,P);
    glGetIntegerv(GL_VIEWPORT,V);

    setOrthographicProjection(global.X_Resolution, global.Y_Resolution);
    glPushMatrix();
    glLoadIdentity();

    pair<multimap<int,Hip_Star *>::iterator, multimap<int,Hip_Star *>::iterator> p;

	int nbZones=0, * zoneList=NULL;
	nbZones = HipGrid.Intersect(global.XYZVision, global.Fov*PI/180, zoneList);
	
for(int i=0;i<nbZones;i++)
{
    p = Liste.equal_range(zoneList[i]);
    for(multimap<int,Hip_Star *>::iterator iter = p.first; iter != p.second; iter++)
    {   
		if ((*iter).second->Mag>6+60./global.Fov) continue;
	    gluProject( ((*iter).second)->XYZ[0],((*iter).second)->XYZ[1],
	        ((*iter).second)->XYZ[2],M,P,V,&(((*iter).second)->XY[0]),
	        &(((*iter).second)->XY[1]),&z);
        if (z<1) 
        {
	        ((*iter).second)->Draw();
        }
    }
}

delete zoneList;
zoneList = NULL;

    glPopMatrix();
    resetPerspectiveProjection();
}

// Look for a star by XYZ coords
int Hip_Star_mgr::Rechercher(vec3_t Pos)
{   
    Pos.Normalize();
    Hip_Star * plusProche;
    float anglePlusProche=3.15;
    
    multimap<int,Hip_Star *>::iterator iter;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {
	if (((*iter).second)->XYZ[0]*Pos[0]+((*iter).second)->XYZ[1]*Pos[1]+((*iter).second)->XYZ[2]*Pos[2]>anglePlusProche)
        {   
			anglePlusProche=((*iter).second)->XYZ[0]*Pos[0]+((*iter).second)->XYZ[1]*Pos[1]+((*iter).second)->XYZ[2]*Pos[2];
            plusProche=(*iter).second;
        }
    }
//  printf("PlusProche : Name:%s HR:%i RA:%4.2f DE:%4.2f Dist:%4.5f\n",&(*plusProche).Name,(*plusProche).HR,(*plusProche).RaRad*12/3.141,(*plusProche).DecRad*180/3.141,anglePlusProche);
    if (anglePlusProche>498)
    {   
	    Selectionnee=plusProche;
        return 0;
    }
    else return 1;
}
