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
#include "stellarium.h"

s_texture * hipStarTexture;
s_font * starFont;

Hip_Star_mgr::Hip_Star_mgr() : HipGrid()
{	
    hipStarTexture=NULL;
    Selectionnee=NULL;
    StarArray = NULL;
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
    if (starFont) delete starFont;
    starFont=NULL;
    
    if (StarArray) delete StarArray;
}

// Load from file ( create the stream and call the Read function )
void Hip_Star_mgr::Load(char * hipCatFile, char * commonNameFile, char * nameFile)
{   
    printf("Loading Hipparcos star data...\n");
    FILE * hipFile, *cnFile, * nFile;
    hipFile=fopen(hipCatFile,"r");
    if (!hipFile)
    {   
        printf("ERROR %s NOT FOUND\n",hipCatFile);
        return;
    }
    cnFile=fopen(commonNameFile,"r");
    if (!cnFile)
    {   
        printf("ERROR %s NOT FOUND\n",cnFile);
        return;
    }
    nFile=fopen(nameFile,"r");
    if (!nFile)
    {   
        printf("ERROR %s NOT FOUND\n",nameFile);
        return;
    }
    
	// Read number of stars in the Hipparcos catalog
    unsigned int catalogSize=0;
    fread((char*)&catalogSize,4,1,hipFile);
    LE_TO_CPU_INT32(catalogSize,catalogSize);  
    
    StarArraySize = 120417;
    // Create the sequential array
    StarArray = new (Hip_Star *)[StarArraySize];
    
	// Read common names & names catalog
	char ** commonNames = new (char *)[120000];
	char ** names = new (char *)[120000];
	
	int tmp;
	char tmpName[20];
	
	while(!feof(cnFile))
	{
		fscanf(cnFile,"%d|%s\n",&tmp,tmpName);
		commonNames[tmp] = strdup(tmpName);
	}
	while(!feof(nFile))
	{
		fscanf(nFile,"%d|%s\n",&tmp,tmpName);
		names[tmp] = strdup(tmpName);
	}

	// Read binary file Hipparcos catalog  
    Hip_Star * e = NULL;
    for(int i=0;i<catalogSize;i++)
    {   
	    e = new Hip_Star;
        e->Read(hipFile);
        // Set names if any
        if(commonNames[e->HP]) e->CommonName=commonNames[e->HP];
        if(names[e->HP]) e->Name=names[e->HP];
        Liste.insert(pair<int, Hip_Star*>(HipGrid.GetNearest(e->XYZ), e));
        StarArray[e->HP]=e;
    }

	delete commonNames;
	delete names;

    fclose(hipFile);
    fclose(cnFile);
    fclose(nFile);
    
    hipStarTexture=new s_texture("etoile16x16Aigrettes");  // Load star texture
    
    char tempName[255];
    strcpy(tempName,global.DataDir);
    strcat(tempName,"spacefont.txt");
    starFont=new s_font(0.012*global.X_Resolution,"spacefont", tempName); // load Font
    if (!starFont)
    {
	    printf("Can't create starFont\n");
        exit(1);
    }
    
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
		        if (global.FlagStarName && ((*iter).second)->CommonName && ((*iter).second)->Mag<global.MaxMagStarName) 
            	{   
		        	((*iter).second)->DrawName();
                	glBindTexture (GL_TEXTURE_2D, hipStarTexture->getID());
            	}
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

// Search the star by HP number
Hip_Star * Hip_Star_mgr::Rechercher(unsigned int _HP)
{
	if (StarArray[_HP] && StarArray[_HP]->HP == _HP)
		return StarArray[_HP];
    return NULL;
}