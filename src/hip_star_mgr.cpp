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
#include "navigator.h"

#define RADIUS_STAR 25.

s_texture * hipStarTexture;
s_font * starFont;

Hip_Star_mgr::Hip_Star_mgr() : HipGrid()
{	
    hipStarTexture=NULL;
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
void Hip_Star_mgr::Load(char * font_fileName, char * hipCatFile, char * commonNameFile, char * nameFile)
{   
    printf("Loading Hipparcos star data...\n");
    FILE * hipFile, *cnFile, * nFile;
    hipFile=fopen(hipCatFile,"rb");
    if (!hipFile)
    {
        printf("ERROR %s NOT FOUND\n",hipCatFile);
        return;
    }
    cnFile=fopen(commonNameFile,"r");
    if (!cnFile)
    {   
        printf("ERROR %s NOT FOUND\n",commonNameFile);
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
    
    StarArraySize = catalogSize;//120417;
    // Create the sequential array
    StarArray = new Hip_Star*[StarArraySize];
    
	// Read common names & names catalog
	char ** commonNames = new char*[catalogSize];
	char ** names = new char*[catalogSize];
	
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
	fclose(cnFile);
    fclose(nFile);

	// Read binary file Hipparcos catalog  
    Hip_Star * e = NULL;
    for(unsigned int i=0;i<catalogSize;i++)
    {
	    e = new Hip_Star;
	    e->HP=(unsigned int)i;
        if (!e->Read(hipFile)) 
        {
        	delete e;
        	e=NULL;
        	continue;
        }
 		
        // Set names if any
        if(commonNames[e->HP]) e->CommonName=commonNames[e->HP];
        if(names[e->HP]) e->Name=names[e->HP];
        
        Liste.insert(pair<int, Hip_Star*>(HipGrid.GetNearest(e->XYZ), e));
        StarArray[e->HP]=e;
    }

	delete commonNames;
	delete names;

    fclose(hipFile);

    hipStarTexture=new s_texture("star16x16");  // Load star texture

    starFont=new s_font(10.f,"spacefont", font_fileName); // load Font
    if (!starFont)
    {
	    printf("Can't create starFont\n");
        exit(1);
    }
    
}


// Draw all the stars
void Hip_Star_mgr::Draw(float _star_scale, float _twinkle_amount, int name_ON,
						float maxMagStarName, Vec3f equ_vision, draw_utility * du)
{
	Hip_Star::twinkle_amount = _twinkle_amount;
	Hip_Star::star_scale = _star_scale;

	glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture (GL_TEXTURE_2D, hipStarTexture->getID());

    double z;
    GLdouble M[16];
    GLdouble P[16];
    GLint V[4];
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    glGetDoublev(GL_PROJECTION_MATRIX,P);
    glGetIntegerv(GL_VIEWPORT,V);

    du->set_orthographic_projection();	// set 2D coordinate

    pair<multimap<int,Hip_Star *>::iterator, multimap<int,Hip_Star *>::iterator> p;

	// Find the star zones which are in the screen
	int nbZones=0;
	static int * zoneList;  // WARNING this is almost a memory leak...

	nbZones = HipGrid.Intersect(equ_vision, du->fov*M_PI/180.*1.4, zoneList);

	//printf("nbzones = %d\n",nbZones );

	// Print all the stars of all the selected zones
	for(int i=0;i<nbZones;i++)
	{
    	p = Liste.equal_range(zoneList[i]);
    	for(multimap<int,Hip_Star *>::iterator iter = p.first; iter != p.second; iter++)
    	{
			// If too small, skip
			if ((*iter).second->Mag>6+60./du->fov) continue;

			// Compute the 2D position
	    	gluProject( ((*iter).second)->XYZ[0],((*iter).second)->XYZ[1], ((*iter).second)->XYZ[2],
				M,P,V,&(((*iter).second)->XY[0]), &(((*iter).second)->XY[1]),&z);
        	if (z<1)
        	{
		        ((*iter).second)->Draw(du);
		        if (((*iter).second)->CommonName && name_ON && ((*iter).second)->Mag<maxMagStarName)
            	{
		        	((*iter).second)->DrawName();
                	glBindTexture (GL_TEXTURE_2D, hipStarTexture->getID());
            	}
        	}
	    }
	}

    du->reset_perspective_projection();
}

// Look for a star by XYZ coords
Hip_Star * Hip_Star_mgr::search(vec3_t Pos)
{   
    Pos.normalize();
    Hip_Star * plusProche=NULL;
    float anglePlusProche=0.;
    
    multimap<int,Hip_Star *>::iterator iter;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {
		if 	(((*iter).second)->XYZ[0]*Pos[0] + ((*iter).second)->XYZ[1]*Pos[1] +
			((*iter).second)->XYZ[2]*Pos[2]>anglePlusProche)
    	{
			anglePlusProche = ((*iter).second)->XYZ[0]*Pos[0] +
				((*iter).second)->XYZ[1]*Pos[1]+((*iter).second)->XYZ[2]*Pos[2];
        	plusProche=(*iter).second;
    	}
    }
	printf("%f\n",anglePlusProche);
    if (anglePlusProche>RADIUS_STAR*0.9999)
    {   
	    return plusProche;
    }
    else return NULL;
}

void Hip_Star_mgr::Save(void)
{
	FILE * fic = fopen("cat.fab","wb");
    fwrite((char*)&StarArraySize,4,1,fic);	
	
    for(int i=0;i<StarArraySize;i++)
    {
    	Hip_Star * s = StarArray[i];
    	float RAf=0, DEf=0;
    	unsigned short int mag=0;
    	unsigned short int type=0;

    	if (s)
    	{
    		RAf=s->r; DEf=s->d;
    		mag = s->magp;
    		type = s->typep;   	
    	}
    	fwrite((char*)&RAf,4,1,fic);
    	fwrite((char*)&DEf,4,1,fic);
    	fwrite((char*)&mag,2,1,fic);
    	fwrite((char*)&type,2,1,fic);	
    }
   	fclose(fic);
}

// Search the star by HP number
Hip_Star * Hip_Star_mgr::search(unsigned int _HP)
{
	if (StarArray[_HP] && StarArray[_HP]->HP == _HP)
		return StarArray[_HP];
    return NULL;
}
