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

/* 18/03/2002
 * Thanks to Sylvain Ferey who sends me optimisation code for the 
 * drawing of the grids.
 */

#include "draw.h"
#include "stellarium.h"
#include "vislimit.h"
#include "s_texture.h"
#include "navigator.h"
#include "solarsystem.h"
#include "stellastro.h"

extern s_texture * texIds[200];            // Common Textures
float DmeriParal[51][2];                   // For grids drawing optimisation

float DmeriParalCos[18][51][2];
float sinTable[18];

int skyResolution = 64;
static vec3_t ** tabSky;                           // For Atmosphere calculation
static    GLdouble M[16]; 
static    GLdouble P[16];
static    GLdouble objx[1];
static    GLdouble objy[1];
static    GLdouble objz[1];
static    GLint V[4];
brightness_data b;


// precalculation of the grid points
void InitMeriParal(void)
{
        float Pi_Over_24 = (float)(M_PI / 24.0f);
        for (register int j = 0; j < 51; ++j){
                DmeriParal[j][0] = (float)(20.0f * cos(j * Pi_Over_24));
                DmeriParal[j][1] = (float)(20.0f * sin(j * Pi_Over_24));
        }


        float Pi_Over_18 = (float)(M_PI / 18.0f);
        for (register int k = 0; k < 18; ++k){
                sinTable[k] = (float)(20.0f * sin((k - 9) * Pi_Over_18));
                register float _cos = (float)(cos((k - 9) * Pi_Over_18));
                for (int j = 0; j < 51; ++j){
                        DmeriParalCos[k][j][0] = DmeriParal[j][0] * _cos;
                        DmeriParalCos[k][j][1] = DmeriParal[j][1] * _cos;
                }
        }
}


void InitAtmosphere(void)
{
    vec3_t * linTabSky;
    tabSky = new vec3_t*[skyResolution+1];
    for (int k=0; k<skyResolution+1 ;k++)
    {
		linTabSky = new vec3_t[skyResolution+1];
        tabSky[k] = linTabSky;
    }
}

// Draw the realistic atmosphere
void CalcAtmosphere(void)
{
	Vec3d temp(0.,0.,0.);
    Vec3d sunPos = navigation.helio_to_local(&temp);

	double lat, lng;
	rect_to_sphe(&lng, &lat, &sunPos);

    b.moon_elongation = 0;//acos(sunPos.Dot(lunaPos));

    b.zenith_ang_moon = 0;
    b.zenith_ang_sun = M_PI_2-lat;

    b.ht_above_sea_in_meters = navigation.get_altitude();
    b.latitude = navigation.get_latitude();

	ln_date d;
	get_date(navigation.get_JDay(),&d);
    b.year = d.years;
    b.month = d.months;

    b.temperature_in_c = (M_PI_2-(float)fabs(b.latitude*100.)/100.)*20.+15.*cos(b.zenith_ang_sun)-0.01*b.ht_above_sea_in_meters;
    //printf("%f\n",b.temperature_in_c);
    b.relative_humidity = 30.;

    navigation.switch_to_local();

    b.mask = 14;                //31 for all 5 bands
    set_brightness_params( &b);
    //compute_extinction(&b);
    // Convert x,y screen pos in 3D vector
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    glGetDoublev(GL_PROJECTION_MATRIX,P);
    glGetIntegerv(GL_VIEWPORT,V);
    int resX = global.X_Resolution;
    float stepX = (float)resX / skyResolution;
    int resY = global.Y_Resolution;
    float stepY = (float)resY / skyResolution;
    int limY;
    
    // Don't calc if under the ground
    if (global.FlagGround)
    {
        gluProject(1,0,0,M,P,V,objx,objy,objz);
        limY = (int)(skyResolution-(float)(*objy)/stepY+3.);
        if (!(limY<skyResolution+1)) limY = skyResolution+1;
    }
    else
    {
        limY = skyResolution+1;
    }

    for (int x=0; x<skyResolution+1; x++)
    {   
        for(int y=0; y<limY; y++)
        {   
            gluUnProject(x*stepX,resY-y*stepY,1,M,P,V,objx,objy,objz);
            Vec3d point(*objx,*objy,*objz);
            point.normalize();
            b.zenith_angle = M_PI_2-asin(point[1]);
            b.dist_sun = acos(point.dot(sunPos));
            if (b.dist_sun<0) b.dist_sun=-b.dist_sun;
            compute_sky_brightness( &b);
            tabSky[x][y].set(sqrt(b.brightness[3]),sqrt(b.brightness[2]*1.5),sqrt(b.brightness[1])*1.2);
            tabSky[x][y]*=330;
        }
    }
}

void DrawAtmosphere2(void)
{
    // to include : optimisation with the ground
    float stepX = (float)global.X_Resolution / skyResolution;
    float stepY = (float)global.Y_Resolution / skyResolution;
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    setOrthographicProjection(global.X_Resolution, global.Y_Resolution);    // 2D coordinate
    glPushMatrix();
    glLoadIdentity();
    for (int y2=0; y2<skyResolution-1+1; y2++)
    {
        glBegin(GL_TRIANGLE_STRIP);
            for(int x2=0; x2<skyResolution+1; x2++)
            {
                glColor4f(tabSky[x2][y2][0],tabSky[x2][y2][1],tabSky[x2][y2][2],tabSky[x2][y2][1]*4);
                glVertex2i((int)(x2*stepX),(int)(y2*stepY));
                glColor4f(tabSky[x2][y2+1][0],tabSky[x2][y2+1][1],tabSky[x2][y2+1][2],tabSky[x2][y2+1][1]*4);
                glVertex2i((int)(x2*stepX),(int)((y2+1)*stepY));
            }
        glEnd();
    }
    glPopMatrix();
    resetPerspectiveProjection();
}

// Draw the cardinals points : N S E O and Z (Zenith) N (Nadir)
void DrawCardinaux(void)
{   float rayon=navigation.get_fov()/60;
    glPushMatrix();
    glColor3f(0.8f, 0.1f, 0.1f);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D,texIds[6]->getID());     //North
    glTranslatef(0.0f,0.0f,30.0f);
    glBegin(GL_QUADS );
        glTexCoord2f(0.0f,0.0f);    glVertex3f( rayon,-rayon,0.0f);   // Bottom Left
        glTexCoord2f(1.0f,0.0f);    glVertex3f(-rayon,-rayon,0.0f);   // Bottom Right
        glTexCoord2f(1.0f,1.0f);    glVertex3f(-rayon, rayon,0.0f);   // Top Right
        glTexCoord2f(0.0f,1.0f);    glVertex3f( rayon, rayon,0.0f);   // Top Left
    glEnd ();

    glBindTexture(GL_TEXTURE_2D, texIds[7]->getID());    //South
    glTranslatef(0.0f,0.0f,-60.0f);
    glBegin(GL_QUADS );
        glTexCoord2f(0.0f,0.0f);    glVertex3f(-rayon,-rayon,0.0f);   // Bottom Left
        glTexCoord2f(1.0f,0.0f);    glVertex3f( rayon,-rayon,0.0f);   // Bottom Right
        glTexCoord2f(1.0f,1.0f);    glVertex3f( rayon, rayon,0.0f);   // Top Right
        glTexCoord2f(0.0f,1.0f);    glVertex3f(-rayon, rayon,0.0f);   // Top Left
    glEnd ();

    glBindTexture(GL_TEXTURE_2D, texIds[9]->getID());    //Ouest
    glTranslatef(30.0f,0.0f,30.0f);
    glBegin(GL_QUADS );
        glTexCoord2f(0.0f,0.0f);    glVertex3f(0.0f,-rayon,-rayon);   // Bottom Left
        glTexCoord2f(1.0f,0.0f);    glVertex3f(0.0f,-rayon, rayon);   // Bottom Right
        glTexCoord2f(1.0f,1.0f);    glVertex3f(0.0f, rayon, rayon);   // Top Right
        glTexCoord2f(0.0f,1.0f);    glVertex3f(0.0f, rayon,-rayon);   // Top Left
    glEnd ();

    glBindTexture(GL_TEXTURE_2D, texIds[8]->getID());    //Est
    glTranslatef(-60.0f,0.0f,0.0f);
    glBegin(GL_QUADS );
        glTexCoord2f(0.0f,0.0f);    glVertex3f(0.0f,-rayon, rayon);   // Bottom Left
        glTexCoord2f(1.0f,0.0f);    glVertex3f(0.0f,-rayon,-rayon);   // Bottom Right
        glTexCoord2f(1.0f,1.0f);    glVertex3f(0.0f, rayon,-rayon);   // Top Right
        glTexCoord2f(0.0f,1.0f);    glVertex3f(0.0f, rayon, rayon);   // Top Left
    glEnd ();

    glBindTexture(GL_TEXTURE_2D, texIds[10]->getID());   //Zenith
    glTranslatef(30.0f,30.0f,0.0f);
    glBegin(GL_QUADS );
        glTexCoord2f(0.0f,0.0f);    glVertex3f( rayon, 0.0f,-rayon);  // Bottom Left
        glTexCoord2f(1.0f,0.0f);    glVertex3f( rayon, 0.0f, rayon);  // Bottom Right
        glTexCoord2f(1.0f,1.0f);    glVertex3f(-rayon, 0.0f, rayon);  // Top Right
        glTexCoord2f(0.0f,1.0f);    glVertex3f(-rayon, 0.0f,-rayon);  // Top Left
    glEnd ();

    glBindTexture(GL_TEXTURE_2D, texIds[11]->getID());   //Nadir
    glTranslatef(0.0f,-60.0f,0.0f);
    glBegin(GL_QUADS );
        glTexCoord2f(0.0f,0.0f);    glVertex3f( rayon, 0.0f,-rayon);  // Bottom Left
        glTexCoord2f(1.0f,0.0f);    glVertex3f( rayon, 0.0f, rayon);  // Bottom Right
        glTexCoord2f(1.0f,1.0f);    glVertex3f(-rayon, 0.0f, rayon);  // Top Right
        glTexCoord2f(0.0f,1.0f);    glVertex3f(-rayon, 0.0f,-rayon);  // Top Left
    glEnd ();
    glPopMatrix();
}

// Draw the milky way : used too to 'clear' the buffer
void DrawMilkyWay(void)
{   if (!global.FlagMilkyWay)
    {   glClear(GL_COLOR_BUFFER_BIT);
        return;
    }
    glPushMatrix();
    glColor3f(0.12f-2*global.SkyBrightness, 0.12f-2*global.SkyBrightness, 0.16f-2*global.SkyBrightness);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, texIds[2]->getID());
    glRotatef(206,1,0,0);
    glRotatef(-16+90,0,1,0);
    glRotatef(-84,0,0,1);
    GLUquadricObj * MilkyWay=gluNewQuadric();
    gluQuadricTexture(MilkyWay,GL_TRUE);
    gluSphere(MilkyWay,500,15,15);
    gluDeleteQuadric(MilkyWay);
    glPopMatrix();
}

// Draw the equatorial merididians
void DrawMeridiens(void)
{       glPushMatrix(); 
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        //glEnable (GL_LINE_STIPPLE);
        //glLineStipple (1, 0x0101);  /*  dotted  */
        //glLineStipple (1, 0x1C47); 
        float coef;
        coef=rand();
        coef/=RAND_MAX;
        glColor3f(0.12*coef+0.18,0.015*coef+0.2,0.015*coef+0.2);
        for(int i=0;i<12;i++)
        {       glRotatef(360./24.,0,0,1);
                for(int j=0;j<49;++j){
                        glBegin(GL_LINES);
                                glVertex3f(DmeriParal[j][0], 0.0f, DmeriParal[j][1]);
                                glVertex3f(DmeriParal[j+1][0], 0.0f, DmeriParal[j+1][1]);
                        glEnd();
                }
        }
        //glDisable (GL_LINE_STIPPLE);
        glPopMatrix();
}


// Draw the azimuthal merididians
void DrawMeridiensAzimut(void)
{       glPushMatrix();
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
///     glEnable (GL_LINE_STIPPLE);
///     glLineStipple (1, 0x00FF);  /*  dashed  */
        float coef = 1;//(float)(1.0f * rand() / RAND_MAX);
        glColor3f(0.015*coef+0.2,0.12*coef+0.18,0.015*coef+0.2);
        for(int i=0;i<12;i++)
        {       glRotatef(360/24,0,0,1);
                for(int j=0;j<49;++j){
                        glBegin(GL_LINES);
                                glVertex3f(DmeriParal[j][0], 0.0f, DmeriParal[j][1]);
                                glVertex3f(DmeriParal[j+1][0], 0.0f, DmeriParal[j+1][1]);
                        glEnd();
                }
        }
///     glDisable (GL_LINE_STIPPLE);
        glPopMatrix();
}


// Draw the equatorial parallels
void DrawParallels(void)
{       glPushMatrix();
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);

        float coef = 1;//(float)(1.0f * rand() / RAND_MAX);
        glColor3f(0.2*coef+0.2,0.025*coef+0.2,0.025*coef+0.2);
        for(int i=0;i<18;++i){
                for(int j=0;j<50;++j){
                        glBegin(GL_LINES);
                                glVertex3f(DmeriParalCos[i] [j] [0],DmeriParalCos[i] [j] [1],sinTable[i]);
                                glVertex3f(DmeriParalCos[i][j+1][0],DmeriParalCos[i][j+1][1],sinTable[i]);
                        glEnd();
                }
        }
        glPopMatrix();
}


// Draw the azimuthal parallels
void DrawParallelsAzimut(void)
{       glPushMatrix();
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);

        float coef = (float)(1.0f * rand() / RAND_MAX);
        glColor3f(0.015*coef+0.2,0.012*coef+0.18,0.015*coef+0.2);
        for(int i=0;i<18;++i){
                for(int j=0;j<50;++j){
                        glBegin(GL_LINES);
                                glVertex3f(DmeriParalCos[i] [j] [0],DmeriParalCos[i] [j] [1],sinTable[i]);
                                glVertex3f(DmeriParalCos[i][j+1][0],DmeriParalCos[i][j+1][1],sinTable[i]);
                        glEnd();
                }
        }
        glPopMatrix();
}


// Draw the celestrial equator
void DrawEquator(void)
{       glPushMatrix();
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        float coef = (float)(1.0f * rand() / RAND_MAX);
        glColor3f(0.2*coef+0.3,0.025*coef+0.1,0.025*coef+0.1);
        for(int j=0;j<49;++j){
                glBegin(GL_LINES);
                        glVertex3f(DmeriParal [j] [0]*20.0f, DmeriParal [j] [1]*20.0f, 0.0f);
                        glVertex3f(DmeriParal[j+1][0]*20.0f, DmeriParal[j+1][1]*20.0f, 0.0f);
                glEnd();
        }
        glPopMatrix();
}


// Draw the ecliptic line
void DrawEcliptic(void)
{       /*glPushMatrix();
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        float coef = (float)(1.0f * rand() / RAND_MAX);
        glColor3f(0.025*coef+0.2,0.025*coef+0.2,0.025*coef+0.0);
        const double centuries = Astro::toMillenia(global.JDay);
        const double obliquity = AstroOps::meanObliquity(centuries);
        glRotatef(obliquity*180/PI,0,0,1);
        for(int j=0;j<49;++j){
                glBegin(GL_LINES);
                        glVertex3f(DmeriParal [j] [0]*20.0f, 0, DmeriParal [j] [1]*20.0f);
                        glVertex3f(DmeriParal[j+1][0]*20.0f, 0, DmeriParal[j+1][1]*20.0f);
                glEnd();
        }
        glPopMatrix();*/
}


// Draw the horizon fog
void DrawFog(void)
{   glPushMatrix();
    glColor4f(0.15f+0.2f*global.SkyBrightness, 0.20f+0.2f*global.SkyBrightness, 0.20f+0.2f*global.SkyBrightness,0.0);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, texIds[3]->getID());
    glTranslatef(0,0,-0.7);
    GLUquadricObj * Horizon=gluNewQuadric();
    gluQuadricTexture(Horizon,GL_TRUE);
    gluCylinder(Horizon,10,10,10,8,1);
    gluDeleteQuadric(Horizon);
    glPopMatrix();
}

// Draw a point... (used for tests)
void DrawPoint(float X,float Y,float Z)
{       glColor3f(0.5, 1.0, 0.5);
        glDisable(GL_TEXTURE_2D);
        //glEnable(GL_BLEND);
        glPointSize(5);
        glBegin(GL_POINTS);
        glVertex3f(X,Y,Z);
        glEnd();
}


// Draw the mountains with a few pieces of a big texture
void DrawDecor(int nb)
{   
    glPushMatrix();
    glColor3f(global.SkyBrightness, global.SkyBrightness, global.SkyBrightness);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    float delta=1.98915/nb; // environ egal à sin((float)(PI/8))/cos((float)(PI/8))*10/2;
    for (int ntex=0;ntex<4*nb;ntex++)
    {   
        glBindTexture(GL_TEXTURE_2D, texIds[31+ntex%4]->getID());
        for (int nDecoupe=0;nDecoupe<4;nDecoupe++)
        {   
            glBegin(GL_QUADS );
                //Haut Gauche
                glTexCoord2f((float)nDecoupe/4,1.0f);       
                glVertex3f(10.0f,-delta,1.5f);
                //Bas Gauche
                glTexCoord2f((float)nDecoupe/4,0.0f);       
                glVertex3f(10.0f,-delta,-0.4f);
                //Bas Droit
                glTexCoord2f((float)nDecoupe/4+0.25,0.0f);
                glVertex3f(10.0f,delta,-0.4f);
                //Haut Droit
                glTexCoord2f((float)nDecoupe/4+0.25,1.0f);
                glVertex3f(10.0f,delta,1.5f);
            glEnd ();
            glRotatef(22.5/nb,0,0,-1);
        }
    }
    glPopMatrix();
}


// Draw the ground
void DrawGround(void)
{
	glPushMatrix();
    glColor3f(global.SkyBrightness, global.SkyBrightness, global.SkyBrightness);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, texIds[1]->getID());
    glBegin (GL_QUADS);
        glTexCoord2f (0.0f,0.0f);
        glVertex3f (-10.0f, -10.0f, -0.2f);
        glTexCoord2f (0.2f, 0.0f);
        glVertex3f(-10.0f, 10.0f, -0.2f);
        glTexCoord2f (0.2f, 0.2f);
        glVertex3f( 10.0f, 10.0f, -0.2f);
        glTexCoord2f (0.0f, 0.2f);
        glVertex3f( 10.0f, -10.0f, -0.2f);
    glEnd ();
    glPopMatrix();
}

