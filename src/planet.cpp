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

#include "planet.h"

planet::Planet(char * _name, int _flagHalo, double _radius, vec3_t _color,
				s_texture * _planetTexture, s_texture * _haloTexture,
				void (*_coord_func)(double JD, struct ln_rect_posn * position) :
					flagHalo(_flagHalo), radius(_radius), color(_color),
					planetTexture(_planetTexture), haloTexture(_haloTexture),
					coord_func(_coord_func), parent(NULL)
{
	planetRing=NULL;
	planetBigHalo=NULL;
	helioCoord=vec3_t(0.,0.,0.);
	name=strdup(_name);
}

planet::~planet()
{
	if (name) free name;
	name=NULL;
	if (planetRing) delete planetRing;
	planetRing=NULL;
	if (planetBigHalo) delete planetBigHalo;
	planetBigHalo=NULL;
}


void planet::computePosition(double date)
{
	ln_rect_posn tempPos;
	coord_func(date, &tempPos);
	helioPos = vec3_t(tempPos->X, tempPos->Y, tempPos->Z);
}

// Get a matrix which converts from local to the parent's coordinates
Mat4d planet::computeMatrix(double date)
{
    double tempAscendingNode = re.ascendingNode + re.precessionRate * (when - astro::J2000);

	transMat = 	Mat4d::xrotation(-re.obliquity) *
				Mat4d::yrotation(-tempAscendingNode) *
        		Mat4d::translation(helioCoord);

    if (parent != NULL && system->getPrimaryBody() != NULL)
        frame = frame * system->getPrimaryBody()->getLocalToHeliocentric(when);
}

// Return the y rotation to use from Equatorial to geographic coordinates
double getGeographicRotation(double when)
{
    double t = when - re.epoch;
    double rotations = t / (double) re.period;
    double wholeRotations = floor(rotations);
    double remainder = rotations - wholeRotations;

    // Add an extra half rotation because of the convention in all
    // planet texture maps where zero deg long. is in the middle of
    // the texture.
    remainder += 0.5;

	return -remainder * 360. - re.offset;
}

vec3_t planet::getHelioPos()
{
	return helioPos;
}

void planet::addSatellite(planet*p)
{
	satellites.push_back(p);
	p->parent=this;
}

void planet::draw(void)
{
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glPushMatrix();
            if (num==10)
            {   glPushMatrix();
                glRotatef(RaRad*180./PI-60,0,0,-1);
            }
            glRotatef(-60,0,0,1);
            glColor3f(1.0f, 1.0f, 1.0f); //*SkyBrightness
            glDisable(GL_BLEND);
            glBindTexture(GL_TEXTURE_2D, planetTexture->getID());

            GLUquadricObj * ThePlanet=gluNewQuadric();
            gluQuadricTexture(ThePlanet,GL_TRUE);
            gluQuadricOrientation(ThePlanet, GLU_OUTSIDE);
            gluSphere(ThePlanet,Rayon*DIST_PLANET/Distance_to_obs,40,40);
            gluDeleteQuadric(ThePlanet);
           if (num==10)
            {   glPopMatrix();
            }
            glDisable(GL_LIGHTING);
        glPopMatrix();
        glDisable(GL_CULL_FACE);
    }

// Draw the planet (with special cases for saturn rings, sun, moon, etc..)
void Planet::Draw(vec3_t coordLight)
{
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);

    vec3_t normGeoCoord=GeoCoord;
    normGeoCoord.Normalize();
    glTranslatef(normGeoCoord[0]*DIST_PLANET,normGeoCoord[1]*DIST_PLANET,normGeoCoord[2]*DIST_PLANET);


    //Light
    vec3_t posSun = coordLight-GeoCoord;
    posSun.Normalize();

    float posLight[4] = {10000*posSun[0],10000*posSun[1],10000*posSun[2]};
    glLightfv(GL_LIGHT1,GL_POSITION,posLight);

    float rmag;
    if (num==0)                                                                     // Sun
    {   glBindTexture (GL_TEXTURE_2D, texIds[48]->getID());
        rmag=25*Rayon*DIST_PLANET/Distance_to_obs*(global.Fov/60);                  // Rayon du halo
        glColor3fv(Colour);
    }
    else
    {   glBindTexture (GL_TEXTURE_2D, texIds[25]->getID());
        glColor3fv(Colour*(1.8-global.SkyBrightness));                              // Calcul de la couleur
        rmag=300*Rayon*DIST_PLANET/Distance_to_obs*global.Fov/60;                   // Rayon de "l'eclat"
        if (num==5) rmag=150*Rayon*DIST_PLANET/Distance_to_obs*global.Fov/60;
    }

    if (num!=10)                                // != luna
    {   glEnable(GL_BLEND);
                                                // Draw a light point like a star for naked eye simulation
        glPushMatrix();
        glRotatef(RaRad*180/PI,0,1,0);
        glRotatef(DecRad*180/PI,-1,0,0);
        glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2i(1,0);                  //Bas Droite
            glVertex3f(rmag,-rmag,0.0f);
            glTexCoord2i(0,0);                  //Bas Gauche
            glVertex3f(-rmag,-rmag,0.0f);
            glTexCoord2i(1,1);                  //Haut Droit
            glVertex3f(rmag,rmag,0.0f);
            glTexCoord2i(0,1);                  //Haut Gauche
            glVertex3f(-rmag,rmag,0.0f);
        glEnd ();
        glPopMatrix();      
    }


    if (num==10)
    {   glBindTexture (GL_TEXTURE_2D, texIds[50]->getID());
        rmag=20*Rayon*DIST_PLANET/Distance_to_obs*(global.Fov/60);                  // Lune : Rayon du halo
        glColor3f(0.07*(1-posSun.Dot(normGeoCoord)),0.07*(1-posSun.Dot(normGeoCoord)),0.07*(1-posSun.Dot(normGeoCoord)));
        glEnable(GL_BLEND);
                                        // Draw a light point like a star for naked eye simulation
        glPushMatrix();
        glRotatef(RaRad*180/PI,0,1,0);
        glRotatef(DecRad*180/PI,-1,0,0);
        glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2i(1,0);                  //Bas Droite
            glVertex3f(rmag,-rmag,0.0f);
            glTexCoord2i(0,0);                  //Bas Gauche
            glVertex3f(-rmag,-rmag,0.0f);
            glTexCoord2i(1,1);                  //Haut Droit
            glVertex3f(rmag,rmag,0.0f);
            glTexCoord2i(0,1);                  //Haut Gauche
            glVertex3f(-rmag,rmag,0.0f);
        glEnd ();
        glPopMatrix();  
    }

    if (global.FlagPlanetsHintDrawing)
    // Draw the name, and the circle
    // Thanks to Nick Porcino for this addition
    {   
        double screenX, screenY, screenZ;
        Project(0, 0, 0, screenX, screenY, screenZ);

        if (screenZ < 1)
        {
            screenY = global.Y_Resolution - screenY;

            setOrthographicProjection(global.X_Resolution, global.Y_Resolution);    // 2D coordinate
            glPushMatrix();
            glLoadIdentity();
            
            glColor3f(1,0.5,0.5);
            float tmp = 50000*atan(Rayon/Distance_to_obs)/global.Fov;
            planetNameFont->print((int)(screenX - 55-tmp),(int)(screenY-tmp), Name);

            // Draw the circle
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_BLEND);
            glBegin(GL_LINE_STRIP);
                float offX, offY;
                for (float r = 0; r < 6.28; r += 0.2)
                {   
                    offX = 8. * sin(r);
                    offY = 8. * cos(r);
                    glVertex3f(screenX + offX, screenY + offY, 0);
                }
            glEnd();

            glPopMatrix();
            glEnable(GL_TEXTURE_2D);
            resetPerspectiveProjection();                           // Restore the other coordinate
        }
    }

    glRotatef(90,1,0,0);


    if (num==6)                             // Draw saturn rings 1/2
    {   double rAn=2.5*Rayon*DIST_PLANET/Distance_to_obs;
        glColor3f(1.0f, 1.0f, 1.0f);
        glBindTexture (GL_TEXTURE_2D, texIds[47]->getID());
        glEnable(GL_BLEND);
        glPushMatrix();

        glRotatef(RaRad*180./PI,0,0,1);
            glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2f(1,0);              //Bas Droite
            glVertex3f(rAn,-rAn,0.0f);
            glTexCoord2f(0,0);              //Bas Gauche
            glVertex3f(-rAn,-rAn,0.0f);
            glTexCoord2f(1,0.5);            //Haut Droit
            glVertex3f(rAn,0.0f,0.0f);
            glTexCoord2f(0,0.5);            //Haut Gauche
            glVertex3f(-rAn,0.0f,0.0f);
        glEnd ();
        glPopMatrix();
    }

    if (asin(Rayon*2/Distance_to_obs)*180./PI>3*global.Fov/global.Y_Resolution)     //Draw the sphere if big enough


    if (num==6)                                 // Draw saturn rings 2/2
    {   double rAn=2.5*Rayon*DIST_PLANET/Distance_to_obs;
        glColor3f(1.0f, 1.0f, 1.0f); //*SkyBrightness
        glBindTexture (GL_TEXTURE_2D, texIds[47]->getID());
        glEnable(GL_BLEND);
        glPushMatrix();
        glRotatef(RaRad*180./PI+180,0,0,1);
        glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2f(1,0);                  //Bas Droite
            glVertex3f(rAn,-rAn,0.0f);
            glTexCoord2f(0,0);                  //Bas Gauche
            glVertex3f(-rAn,-rAn,0.0f);
            glTexCoord2f(1,0.5);                //Haut Droit
            glVertex3f(rAn,0.0f,0.0f);
            glTexCoord2f(0,0.5);                //Haut Gauche
            glVertex3f(-rAn,0.0f,0.0f);
        glEnd ();
        glPopMatrix();
    }

    glPopMatrix();
}

void Planet::testDistance(vec3_t Pos,float &anglePlusProche, Planet * &plusProche)
{   vec3_t PosPlanet;
    RADE_to_XYZ(RaRad,DecRad, PosPlanet);
    PosPlanet.Normalize();
    if (PosPlanet[0]*Pos[0]+PosPlanet[1]*Pos[1]+PosPlanet[2]*Pos[2]>anglePlusProche)
        {   anglePlusProche=PosPlanet[0]*Pos[0]+PosPlanet[1]*Pos[1]+PosPlanet[2]*Pos[2];
            plusProche=this;
        }
}
