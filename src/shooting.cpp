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

#include "shooting.h"

ShootingStar::ShootingStar(int frame) : 
	BirthDate(frame),
	Dead(0)	 
{	
	ShootTexture = new s_texture("etoile32x32");
	XYZ0.Set(500.,500,0);
	XYZ=XYZ0 * -1;
	
	XYZ.Normalize();
	
	XYZ[0] += 0.4*rand()/RAND_MAX-0.2;
	XYZ[1] += 0.4*rand()/RAND_MAX-0.2;
	XYZ[2] += 0.4*rand()/RAND_MAX-0.2;
}

ShootingStar::~ShootingStar()
{   
}

void ShootingStar::Draw(int frame)
{
	if (frame-BirthDate>1000) 
	{
		Dead=1;
		return;
	}

    double z;
    glColor3f(0.7, 0.7, 1.0);
    GLdouble M[16];
    GLdouble P[16];
    GLint V[4];
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    glGetDoublev(GL_PROJECTION_MATRIX,P);
    glGetIntegerv(GL_VIEWPORT,V);

    setOrthographicProjection(global.X_Resolution, global.Y_Resolution);
    glPushMatrix();
    glLoadIdentity();

	

	gluProject(XYZ0[0]+XYZ[0]*(frame-BirthDate),
				XYZ0[1]+XYZ[1]*(frame-BirthDate),
				XYZ0[2]+XYZ[2]*(frame-BirthDate),M,P,V,&XY[0],&XY[1],&z);
    // Check if in the field of view, if not return
    if (z>=1 || XY[0]<0 || XY[0]>global.X_Resolution ||
		XY[1]<0 || XY[1]>global.Y_Resolution) 
	{
		glPopMatrix();
    	resetPerspectiveProjection();
		return;
	}
	float rmag=5.;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture (GL_TEXTURE_2D, ShootTexture->getID());


    glTranslatef(XY[0],global.Y_Resolution-XY[1],0);
    glBegin(GL_QUADS );
        glTexCoord2i(0,0);    glVertex3f(-rmag,-rmag,0);      //Bas Gauche
        glTexCoord2i(1,0);    glVertex3f(rmag,-rmag,0);       //Bas Droite
        glTexCoord2i(1,1);    glVertex3f(rmag,rmag,0);        //Haut Droit
        glTexCoord2i(0,1);    glVertex3f(-rmag,rmag,0);       //Haut Gauche
    glEnd ();

    glPopMatrix();
    resetPerspectiveProjection();
}