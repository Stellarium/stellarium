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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "s_font.h"
#include "s_texture.h"

#define SPACING 1

s_font::s_font(float size_i, char * textureName, char * dataFileName) : size(size_i), s_fontTexture(NULL)
{
    if (!buildDisplayLists(dataFileName, textureName)) printf("ERROR WHILE CREATING FONT %s\n",textureName);
}

s_font::~s_font()
{
    glDeleteLists(g_base, 256);                          // Delete All 256 Display Lists
    if (s_fontTexture) delete s_fontTexture;
    s_fontTexture=NULL;
}

int s_font::buildDisplayLists(char * dataFileName, char * textureName)
{
    int posX;
    int posY;
    int sizeX;
    int sizeY;
    int leftSpacing;
    int charNum=0;
    int nbChar=0;

    s_fontTexture = new s_texture(textureName);
    if (!s_fontTexture)
    {
	printf("ERROR WHILE CREATING FONT TEXTURE\n");
	return 0;
    }

    FILE * pFile;
    pFile = fopen(dataFileName,"r");
    if (pFile==NULL)
    {
	printf("ERROR WHILE LOADING %s\n",dataFileName);
        return 0;
    }
    if (fscanf(pFile,"%s LineSize %d\n",name, &lineHeight)!=2)
    {
	fclose(pFile);
        return 0;
    }
   
    averageCharLen=0;
    ratio = size/lineHeight;
    g_base = glGenLists(25);                           // Creating 256 Display Lists
    glBindTexture(GL_TEXTURE_2D, s_fontTexture->getID());          // Select Our s_font Texture

    while(fscanf(pFile,"%d %d %d %d %d %d\n",&charNum,&posX,&posY,&sizeX,&sizeY,&leftSpacing)==6)
    {
	//printf("%d %d %d %d %d %d\n",charNum,posX,posY,sizeX,sizeY,leftSpacing);
        theSize[charNum].sizeX=sizeX;
        theSize[charNum].sizeY=sizeY;
        theSize[charNum].leftSpacing=leftSpacing;
        theSize[charNum].rightSpacing=0;
        averageCharLen+=sizeX;
        nbChar++;

        glNewList(g_base+charNum,GL_COMPILE);                                       // Start Building A List
	glTranslated(leftSpacing*ratio,0,0);                                    // Move To The Left Of The Character
	glBegin(GL_QUADS );
	    glTexCoord2f((float)posX/256,(float)(256-posY-sizeY)/256);
	    glVertex3f(0,sizeY*ratio,0);                                        //Bas Gauche
	    glTexCoord2f((float)(posX+sizeX)/256,(float)(256-posY-sizeY)/256);  
	    glVertex3f(sizeX*ratio,sizeY*ratio,0);                              //Bas Droite
	    glTexCoord2f((float)(posX+sizeX)/256,(float)(256-posY)/256);
	    glVertex3f(sizeX*ratio,0,0);                                        //Haut Droit
	    glTexCoord2f((float)posX/256,(float)(256-posY)/256);
	    glVertex3f(0,0,0);                                                  //Haut Gauche
	glEnd ();
	glTranslated((sizeX+SPACING)*ratio,0,0);
        glEndList();                                    // Done Building The Display List
    }
    fclose(pFile);
    averageCharLen/=nbChar;
    return 1;
}

void s_font::print(float x, float y, char * str)
{ 
    if	(!s_fontTexture)
    {
	printf("ERROR, NO FONT TEXTURE\n");
	exit(1);
    }
    glBindTexture(GL_TEXTURE_2D, s_fontTexture->getID());  // Select Our s_font Texture
    glPushMatrix();
    glTranslated(x,y,0);                                // Position The Text (0,0 - Top Left)
    glListBase(g_base);                                 // Init the Display list base
    glCallLists(strlen(str),GL_BYTE,str);               // Write The Text To The Screen
    glPopMatrix();
}

float s_font::getStrLen(const char * str)
{
    float s=0;
    for (int i=0;i<(int)strlen(str);i++)
    { 
	s+=theSize[str[i]].sizeX+theSize[str[i]].leftSpacing+theSize[str[i]].rightSpacing+SPACING;
    }
    return s*ratio;
}

float s_font::getStrHeight(const char * str)
{  
    float s=0;
    for (int i=0;i<(int)strlen(str);i++)
    {
	if (s<theSize[str[i]].sizeY) s=theSize[str[i]].sizeY;
    }
    return s*ratio;
}
