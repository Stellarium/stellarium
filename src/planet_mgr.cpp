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

#include "planet_mgr.h"
#include "s_font.h"

s_font * planetNameFont;

static Planet * Sun;
static Planet * Mercury;
static Planet * Venus;
static Planet * Earth;
static Planet * Mars;
static Planet * Jupiter;
static Planet * Saturn;
static Planet * Luna;
static Planet * Uranus;
static Planet * Neptune;
static Planet * Pluto;

// Creation initialisation
Planet_mgr::Planet_mgr()
{   
    Sun = new Planet(SUN);
    Mercury = new Planet(MERCURY);
    Venus = new Planet(VENUS);
    Earth = new Planet(EARTH);
    Mars = new Planet(MARS);
    Jupiter = new Planet(JUPITER);
    Saturn = new Planet(SATURN);
    Luna = new Planet(LUNA);
    Uranus = new Planet(URANUS);
    Neptune = new Planet(NEPTUNE);
    Pluto = new Planet(PLUTO);
    
    Sun->Rayon=1392000./(2*UA);
    Sun->Colour[0]=1.0;
    Sun->Colour[1]=1.0;
    Sun->Colour[2]=0.8;
    
    Mercury->Rayon=4878./(2*UA);
    Mercury->Colour[0]=0.9;
    Mercury->Colour[1]=0.9;
    Mercury->Colour[2]=0.8;
    
    Venus->Rayon=12104./(2*UA);
    Venus->Colour[0]=0.8;
    Venus->Colour[1]=0.8;
    Venus->Colour[2]=0.8;

    Mars->Rayon=6794./(2*UA);
    Mars->Colour[0]=1.0;
    Mars->Colour[1]=0.5;
    Mars->Colour[2]=0.5;

    Jupiter->Rayon=142796./(2*UA);
    Jupiter->Colour[0]=0.7;
    Jupiter->Colour[1]=0.7;
    Jupiter->Colour[2]=0.6;

    Saturn->Rayon=120660./(2*UA);
    Saturn->Colour[0]=0.7;
    Saturn->Colour[1]=0.7;
    Saturn->Colour[2]=0.6;

    Luna->Rayon=3476./(2*UA);
    Luna->Colour[0]=1.0;
    Luna->Colour[1]=1.0;
    Luna->Colour[2]=1.0;

    Uranus->Rayon=51800./(2*UA);
    Uranus->Colour[0]=0.5;
    Uranus->Colour[1]=1.0;
    Uranus->Colour[2]=1.0;

    Neptune->Rayon=49500./(2*UA);
    Neptune->Colour[0]=0.7;
    Neptune->Colour[1]=0.7;
    Neptune->Colour[2]=1.0;

    Pluto->Rayon=2274./(2*UA);
    Pluto->Colour[0]=1.0;
    Pluto->Colour[1]=1.0;
    Pluto->Colour[2]=1.0;
}

void Planet_mgr::loadTextures()
{
    planetNameFont = new s_font(0.014*global.X_Resolution,"spacefont", DATA_DIR "/spacefont.txt"); // load Font
    if (!planetNameFont)
    {
	printf("Can't create planetNameFont\n");
        exit(1);
    }
        
    Sun->planetTexture=new s_texture("sun",TEX_LOAD_TYPE_PNG_SOLID);
    Mercury->planetTexture=new s_texture("mercury",TEX_LOAD_TYPE_PNG_SOLID);
    Venus->planetTexture=new s_texture("venus",TEX_LOAD_TYPE_PNG_SOLID);
    Pluto->planetTexture=new s_texture("lune",TEX_LOAD_TYPE_PNG_SOLID);
    Neptune->planetTexture=new s_texture("neptune",TEX_LOAD_TYPE_PNG_SOLID);
    Uranus->planetTexture=new s_texture("uranus",TEX_LOAD_TYPE_PNG_SOLID);
    Luna->planetTexture=new s_texture("lune",TEX_LOAD_TYPE_PNG_SOLID);
    Saturn->planetTexture=new s_texture("saturn",TEX_LOAD_TYPE_PNG_SOLID);
    Jupiter->planetTexture=new s_texture("jupiter",TEX_LOAD_TYPE_PNG_SOLID);
    Mars->planetTexture=new s_texture("mars",TEX_LOAD_TYPE_PNG_SOLID);
}


Planet_mgr::~Planet_mgr()
{   
    delete planetNameFont;
    planetNameFont=NULL;
    delete Mercury->planetTexture;
    Mercury->planetTexture=NULL;
    delete Venus->planetTexture;
    Venus->planetTexture=NULL;
    delete Mars->planetTexture;
    Mars->planetTexture=NULL;
    delete Jupiter->planetTexture;
    Jupiter->planetTexture=NULL;
    delete Saturn->planetTexture;
    Saturn->planetTexture=NULL;
    delete Sun->planetTexture;
    Sun->planetTexture=NULL;
    delete Luna->planetTexture;
    Luna->planetTexture=NULL;
    delete Uranus->planetTexture;
    Uranus->planetTexture=NULL;
    delete Neptune->planetTexture;
    Neptune->planetTexture=NULL;
    delete Pluto->planetTexture;
    Pluto->planetTexture=NULL;
    delete Mercury;
    delete Venus;
    delete Earth;
    delete Mars;
    delete Jupiter;
    delete Saturn;
    delete Sun;
    delete Luna;
    delete Uranus;
    delete Neptune;
    delete Pluto;
}

// Calc all the datas for the planets
void Planet_mgr::Compute(double date, ObsInfo lieu)
{
    vec3_t tmp(0,0,0);
    Earth->Compute(date, lieu, tmp);
    vec3_t helioPosObs( Earth->HelioCoord);
    Sun->Compute(date, lieu, helioPosObs);
    Mercury->Compute(date, lieu, helioPosObs);
    Venus->Compute(date, lieu, helioPosObs);
    Mars->Compute(date, lieu, helioPosObs);
    Jupiter->Compute(date, lieu, helioPosObs);
    Saturn->Compute(date, lieu, helioPosObs);
    Luna->Compute(date, lieu, helioPosObs);
    Uranus->Compute(date, lieu, helioPosObs);
    Neptune->Compute(date, lieu, helioPosObs);
    Pluto->Compute(date, lieu, helioPosObs);
}

void Planet_mgr::DrawMoonDaylight()
{
    vec3_t coordLight;
    coordLight =  Sun->GeoCoord;
    
//light
    float tmp[4] = {0,0,0,1};
    float tmp2[4] = {0.01,0.01,0.01,1};
    float tmp3[4] = {1,1,1,1};
    float tmp4[4] = {0.5,0.5,0.5,1};
    glLightfv(GL_LIGHT1,GL_AMBIENT,tmp3); 
    glLightfv(GL_LIGHT1,GL_DIFFUSE,tmp3);
    glLightfv(GL_LIGHT1,GL_SPECULAR,tmp);

    glDisable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    
    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT ,tmp2);
    glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE ,tmp3);
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION ,tmp);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS ,tmp);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR ,tmp);

    Luna->DrawSpecialDaylight(coordLight);
}

// Draw the planets
void Planet_mgr::Draw()
{
    vec3_t coordLight;
    coordLight =  Sun->GeoCoord;
    
//light
    float tmp[4] = {0,0,0,1};
    float tmp2[4] = {0.01,0.01,0.01,1};
    float tmp3[4] = {1,1,1,1};
    float tmp4[4] = {0.5,0.5,0.5,1};
    glLightfv(GL_LIGHT1,GL_AMBIENT,tmp3); 
    glLightfv(GL_LIGHT1,GL_DIFFUSE,tmp3);
    glLightfv(GL_LIGHT1,GL_SPECULAR,tmp);

    glDisable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    
    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT ,tmp2);
    glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE ,tmp3);
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION ,tmp);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS ,tmp);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR ,tmp);

    Luna->Draw(coordLight);
    Mercury->Draw(coordLight);
    Venus->Draw(coordLight);
    Sun->Draw(coordLight);
//   Earth->Draw(coordLight);
    Mars->Draw(coordLight);
    Jupiter->Draw(coordLight);
    Saturn->Draw(coordLight);
    Uranus->Draw(coordLight);
    Neptune->Draw(coordLight);
    Pluto->Draw(coordLight);
}


// Look for a planet by XYZ coords
int Planet_mgr::Rechercher(vec3_t Pos)
{
    Pos.Normalize();
    Planet * plusProche;

    float anglePlusProche=0.001;

    Sun->testDistance(Pos,anglePlusProche,plusProche);
    Mercury->testDistance(Pos,anglePlusProche,plusProche);
    Venus->testDistance(Pos,anglePlusProche,plusProche);
    Mars->testDistance(Pos,anglePlusProche,plusProche);
    Jupiter->testDistance(Pos,anglePlusProche,plusProche);
    Saturn->testDistance(Pos,anglePlusProche,plusProche);
    Luna->testDistance(Pos,anglePlusProche,plusProche);
    Uranus->testDistance(Pos,anglePlusProche,plusProche);
    Neptune->testDistance(Pos,anglePlusProche,plusProche);
    Pluto->testDistance(Pos,anglePlusProche,plusProche);

    if (anglePlusProche>0.99995)
    {
	Selectionnee=plusProche;
	return 0;
    }
    else return 1;
}

vec3_t Planet_mgr::GetPos(int num)
{   
    vec3_t PosPlanet;
    if (num==0) RADE_to_XYZ(Sun->RaRad,Sun->DecRad, PosPlanet);
        else if (num==10) RADE_to_XYZ(Luna->RaRad,Luna->DecRad, PosPlanet);
          else exit(1);
    return PosPlanet;
}
