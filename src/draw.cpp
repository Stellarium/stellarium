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
#include "s_texture.h"
#include "navigator.h"
#include "solarsystem.h"
#include "stellastro.h"

extern s_texture * texIds[200];            // Common Textures
float DmeriParal[51][2];                   // For grids drawing optimisation

float DmeriParalCos[18][51][2];
float sinTable[18];

// precalculation of the grid points
void InitMeriParal(void)
{
	float Pi_Over_24 = (float)(M_PI / 24.0f);
	for (register int j = 0; j < 51; ++j)
	{
		DmeriParal[j][0] = (float)(27.0f * cos(j * Pi_Over_24));
		DmeriParal[j][1] = (float)(27.0f * sin(j * Pi_Over_24));
	}


	float Pi_Over_18 = (float)(M_PI / 18.0f);
	for (register int k = 0; k < 18; ++k)
	{
		sinTable[k] = (float)(27.0f * sin((k - 9) * Pi_Over_18));
		register float _cos = (float)(cos((k - 9) * Pi_Over_18));
		for (int j = 0; j < 51; ++j)
		{
			DmeriParalCos[k][j][0] = DmeriParal[j][0] * _cos;
			DmeriParalCos[k][j][1] = DmeriParal[j][1] * _cos;
		}
	}
}


// Draw the cardinals points : N S E W (and Z (Zenith) N (Nadir))
void DrawCardinaux(draw_utility * du)
{   
	GLdouble M[16];
	GLdouble P[16];
	GLint V[4];
	glGetDoublev(GL_MODELVIEW_MATRIX,M);
	glGetDoublev(GL_PROJECTION_MATRIX,P);
	glGetIntegerv(GL_VIEWPORT,V);

	double x[4],y[4],z[4];
	gluProject(-1.0f, 0.0f, 0.0f,M,P,V,&x[0],&y[0],&z[0]); // North
	gluProject( 1.0f, 0.0f, 0.0f,M,P,V,&x[1],&y[1],&z[1]); // South
	gluProject( 0.0f, 1.0f, 0.0f,M,P,V,&x[2],&y[2],&z[2]); // East
	gluProject( 0.0f,-1.0f, 0.0f,M,P,V,&x[3],&y[3],&z[3]); // West

	du->set_orthographic_projection();

	glColor3f(0.8f, 0.1f, 0.1f);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	int r=24;
	
	if (z[0]<1)
	{
		glBindTexture(GL_TEXTURE_2D,texIds[6]->getID());     //North
		glBegin(GL_QUADS );
			glTexCoord2f(0.0f,0.0f);    glVertex2f(x[0]-r,y[0]-r);  // Bottom Left
			glTexCoord2f(1.0f,0.0f);    glVertex2f(x[0]+r,y[0]-r);  // Bottom Right
			glTexCoord2f(1.0f,1.0f);    glVertex2f(x[0]+r,y[0]+r);  // Top Right
			glTexCoord2f(0.0f,1.0f);    glVertex2f(x[0]-r,y[0]+r);  // Top Left
		glEnd ();
	}

	if (z[1]<1)
	{
		glBindTexture(GL_TEXTURE_2D, texIds[7]->getID());    //South
		glBegin(GL_QUADS );
			glTexCoord2f(0.0f,0.0f);    glVertex2f(x[1]-r,y[1]-r);  // Bottom Left
			glTexCoord2f(1.0f,0.0f);    glVertex2f(x[1]+r,y[1]-r);  // Bottom Right
			glTexCoord2f(1.0f,1.0f);    glVertex2f(x[1]+r,y[1]+r);  // Top Right
			glTexCoord2f(0.0f,1.0f);    glVertex2f(x[1]-r,y[1]+r);  // Top Left
		glEnd ();
	}

	if (z[2]<1)
	{
		glBindTexture(GL_TEXTURE_2D, texIds[8]->getID());    //East
		glBegin(GL_QUADS );
			glTexCoord2f(0.0f,0.0f);    glVertex2f(x[2]-r,y[2]-r);  // Bottom Left
			glTexCoord2f(1.0f,0.0f);    glVertex2f(x[2]+r,y[2]-r);  // Bottom Right
			glTexCoord2f(1.0f,1.0f);    glVertex2f(x[2]+r,y[2]+r);  // Top Right
			glTexCoord2f(0.0f,1.0f);    glVertex2f(x[2]-r,y[2]+r);  // Top Left
		glEnd ();
	}

	if (z[3]<1)
	{
		glBindTexture(GL_TEXTURE_2D, texIds[9]->getID());    //West
		glBegin(GL_QUADS );
			glTexCoord2f(0.0f,0.0f);    glVertex2f(x[3]-r,y[3]-r);  // Bottom Left
			glTexCoord2f(1.0f,0.0f);    glVertex2f(x[3]+r,y[3]-r);  // Bottom Right
			glTexCoord2f(1.0f,1.0f);    glVertex2f(x[3]+r,y[3]+r);  // Top Right
			glTexCoord2f(0.0f,1.0f);    glVertex2f(x[3]-r,y[3]+r);  // Top Left
		glEnd ();
	}
	
	du->reset_perspective_projection();
}

// Draw the milky way : used too to 'clear' the buffer
void DrawMilkyWay(float sky_brightness)
{   
	glPushMatrix();
	glColor3f(0.12f-2*sky_brightness, 0.12f-2*sky_brightness, 0.16f-2*sky_brightness);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, texIds[2]->getID());
	glRotatef(206,1,0,0);
	glRotatef(-16+90,0,1,0);
	glRotatef(-84,0,0,1);
	GLUquadricObj * MilkyWay=gluNewQuadric();
	gluQuadricTexture(MilkyWay,GL_TRUE);
	gluSphere(MilkyWay,29.,15,15);
	gluDeleteQuadric(MilkyWay);
	glPopMatrix();
}

// Draw the equatorial merididians
void DrawMeridiens(void)
{
	glPushMatrix(); 
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	float coef;
	coef=rand();
	coef/=RAND_MAX;
	glColor3f(0.12*coef+0.18,0.015*coef+0.2,0.015*coef+0.2);
	for(int i=0;i<12;i++)
	{
		glRotatef(360./24.,0,0,1);
		for(int j=0;j<49;++j)
		{
			glBegin(GL_LINES);
			glVertex3f(DmeriParal[j][0], 0.0f, DmeriParal[j][1]);
			glVertex3f(DmeriParal[j+1][0], 0.0f, DmeriParal[j+1][1]);
			glEnd();
		}
	}
	glPopMatrix();
}


// Draw the azimuthal merididians
void DrawMeridiensAzimut(void)
{
	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	float coef = 1;
	glColor3f(0.015*coef+0.2,0.12*coef+0.18,0.015*coef+0.2);
	for(int i=0;i<12;i++)
	{
		glRotatef(360/24,0,0,1);
		for(int j=0;j<49;++j)
		{
			glBegin(GL_LINES);
				glVertex3f(DmeriParal[j][0], 0.0f, DmeriParal[j][1]);
				glVertex3f(DmeriParal[j+1][0], 0.0f, DmeriParal[j+1][1]);
			glEnd();
		}
	}
	glPopMatrix();
}


// Draw the equatorial parallels
void DrawParallels(void)
{       
	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	float coef = 1;
	glColor3f(0.2*coef+0.2,0.025*coef+0.2,0.025*coef+0.2);
	for(int i=0;i<18;++i)
	{
		for(int j=0;j<50;++j)
		{
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
{
	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	
	float coef = (float)(1.0f * rand() / RAND_MAX);
	glColor3f(0.015*coef+0.2,0.012*coef+0.18,0.015*coef+0.2);
	for(int i=0;i<18;++i)
	{
		for(int j=0;j<50;++j)
		{
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
{       
	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	float coef = (float)(1.0f * rand() / RAND_MAX);
	glColor3f(0.2*coef+0.3,0.025*coef+0.1,0.025*coef+0.1);
	for(int j=0;j<49;++j)
	{
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
void DrawFog(float sky_brightness)
{   
	glPushMatrix();
	glColor4f(0.15f+0.4f*sky_brightness, 0.20f+0.4f*sky_brightness, 0.20f+0.4f*sky_brightness,0.5);
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
{       
	glColor3f(0.8, 1.0, 0.8);
	glDisable(GL_TEXTURE_2D);
	//glEnable(GL_BLEND);
	glPointSize(20.);
	glBegin(GL_POINTS);
		glVertex3f(X,Y,Z);
	glEnd();
}


// Draw the mountains with a few pieces of a big texture
void DrawDecor(int nb, float sky_brightness)
{   
	glPushMatrix();
	glColor3f(sky_brightness, sky_brightness, sky_brightness);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	float delta=1.98915/nb; // almost egal to sin((float)(PI/8))/cos((float)(PI/8))*10/2;
	for (int ntex=0;ntex<4*nb;ntex++)
	{   
		glBindTexture(GL_TEXTURE_2D, texIds[31+ntex%4]->getID());
		for (int nDecoupe=0;nDecoupe<4;nDecoupe++)
		{   
			glBegin(GL_QUADS );
				//Haut Gauche
				glTexCoord2f((float)nDecoupe/4,1.0f);       
				glVertex3f(10.0f,delta,1.5f);
				//Bas Gauche
				glTexCoord2f((float)nDecoupe/4,0.0f);
				glVertex3f(10.0f,delta,-0.4f);
				//Bas Droit
				glTexCoord2f((float)nDecoupe/4+0.25,0.0f);
				glVertex3f(10.0f,-delta,-0.4f);
				//Haut Droit
				glTexCoord2f((float)nDecoupe/4+0.25,1.0f);
				glVertex3f(10.0f,-delta,1.5f);
			glEnd ();
			glRotatef(22.5/nb,0,0,-1);
		}
	}
	glPopMatrix();
}


// Draw the ground
void DrawGround(float sky_brightness)
{
	glPushMatrix();
	glColor3f(sky_brightness, sky_brightness, sky_brightness);
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

