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

// class used to manage groups of Star

#include "star_mgr.h"
#include "s_texture.h"
#include "s_font.h"

s_texture * starTexture;
s_font * starFont;

Star_mgr::Star_mgr()
{	
    starTexture=NULL;
    starFont=NULL;
    Selectionnee=NULL;
}

Star_mgr::~Star_mgr()
{   
    vector<Star *>::iterator iter;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {   
        delete (*iter);
    }
    if (starTexture) delete starTexture;
    starTexture=NULL;
    if (starFont) delete starFont;
    starFont=NULL;
}

// Load from file ( create the stream and call the Read function )
void Star_mgr::Load(char * NameFichier)
{   
    printf("Loading star data...\n");
    FILE * pFile;
    pFile=fopen(NameFichier,"r");
    if (pFile==NULL)
    {   
        printf("ERROR %s NOT FOUND\n",NameFichier);
        return;
    }
    Star_mgr::Read(pFile);

    fclose(pFile);
    starTexture=new s_texture("etoile16x16Aigrettes");  // Load star texture
}


char * removeEndSpaces(char * str)
{   
	unsigned int i=0;
    while (str[i]!='\0' && str[i]!='\n' && str[i]!='\r') {i++;}
    str[i]='\0';
    i--;
    while(str[i]==' ') {str[i]='\0'; i--;}
    char * result = strdup(str);
    return result;
}

int Star_mgr::Read(FILE * catalog)
{   
    char ligne[210];
    char strTmp[5];
    char strTmp2[210];
    
    strTmp2[0] = 0;
    FILE * pFile;
    pFile=fopen(DATA_DIR "/name.txt","r");
    if (pFile==NULL)
    {   
	    printf("ERROR : " DATA_DIR "/name.txt is an invalid file");
        return 1;
    }
    fgets(ligne,200,pFile);
    ligne[199]='\0';
    strncpy(strTmp,ligne,4);    // HR number
    strTmp[4]='\0';
    unsigned int numHR = atoi(strTmp);
    strcpy(strTmp2,&ligne[4]);
    Star * e = NULL;
    for(int i=1;i<=9110;i++)
    {   
	    e = new Star;
        e->Read(catalog);
        Liste.push_back(e);
        if (numHR==e->HR)
        {   
	        // assign the common name
            e->CommonName = removeEndSpaces(strTmp2);
            int nameSize=strcspn(strTmp2,";");					// copy string before the first ";"
            if (nameSize==strlen(e->CommonName)) nameSize=0;	// no ";" found => no short common name
            strTmp2[nameSize]='\0';
			e->ShortCommonName=strdup(strTmp2);
			if(nameSize) e->haveShortCommonName=true;
            // take the next one
            fgets(ligne,200,pFile);
            ligne[199]='\0';
            strncpy(strTmp,ligne,4);    // HR number
            strTmp[4]='\0';
            numHR = atoi(strTmp);
            strcpy(strTmp2,&ligne[4]);
        }
        else 
        {		
	        e->CommonName=new char[1];
	        e->CommonName[0]=0;
            e->ShortCommonName=new char[1];
            e->ShortCommonName[0]=0;
		    e->haveShortCommonName=false;
        }
    }
    fclose(pFile);

    starFont=new s_font(0.012*global.X_Resolution,"spacefont", DATA_DIR "/spacefont.txt"); // load Font
    if (!starFont)
    {
	    printf("Can't create starFont\n");
        exit(1);
    }
	return 0;
}

// Draw all the stars
void Star_mgr::Draw(void)
{   
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture (GL_TEXTURE_2D, /*Star::*/starTexture->getID());

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

    vector<Star *>::iterator iter;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {   
	gluProject((**iter).XYZ[0],(**iter).XYZ[1],(**iter).XYZ[2],M,P,V,&((**iter).XY[0]),&((**iter).XY[1]),&z);
        if (z<1) 
        {   
	        (**iter).Draw();
            if (global.FlagStarName && (**iter).haveShortCommonName && (**iter).Mag<global.MaxMagStarName) 
            {   
		        (**iter).DrawName();
                glBindTexture (GL_TEXTURE_2D, /*Star::*/starTexture->getID());
            }
        }
    }
    glPopMatrix();
    resetPerspectiveProjection();
}

// Look for a star by XYZ coords
int Star_mgr::Rechercher(vec3_t Pos)
{   
    Pos.Normalize();
    vector<Star *>::iterator iter;
    Star * plusProche;
    float anglePlusProche=3.15;
    for(iter=Liste.begin();iter!=Liste.end();iter++)
    {   
	if ((**iter).XYZ[0]*Pos[0]+(**iter).XYZ[1]*Pos[1]+(**iter).XYZ[2]*Pos[2]>anglePlusProche)
        {   
			anglePlusProche=(**iter).XYZ[0]*Pos[0]+(**iter).XYZ[1]*Pos[1]+(**iter).XYZ[2]*Pos[2];
            plusProche=(*iter);
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

// Look for a star by Harvard Revised Catalog number
Star * Star_mgr::Rechercher(unsigned int HR)
{
    if (HR<0 || HR>Liste.size()) return NULL;
    if (Liste[HR-1]) return Liste[HR-1];
    return NULL;
}
