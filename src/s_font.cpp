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


#include <string>
#include <iostream>

#include <stdio.h>
#include "s_font.h"


s_font::s_font(float size_i, const string& textureName, const string& dataFileName) : size(size_i)
{
    if (!buildDisplayLists(dataFileName, textureName)) cout << "ERROR WHILE CREATING FONT " << textureName << endl;
}

s_font::~s_font()
{
    glDeleteLists(g_base, 256);                          // Delete All 256 Display Lists
	delete s_fontTexture; s_fontTexture = NULL;
}

int s_font::buildDisplayLists(const string& dataFileName, const string& textureName)
{
    int posX;
    int posY;
    int sizeX;
    int sizeY;
    int leftSpacing;
    int charNum=0;
    int nbChar=0;

    s_fontTexture = new s_texture(textureName, TEX_LOAD_TYPE_PNG_BLEND3);
    if (!s_fontTexture)
    {
		cout << "ERROR WHILE CREATING FONT TEXTURE " << textureName << endl;
		return 0;
    }

	FILE * pFile = fopen(dataFileName.c_str(),"r");
    if (!pFile)
    {
		cout << "ERROR WHILE LOADING "<< dataFileName << endl;
        return 0;
    }
    if (fscanf(pFile,"%s LineSize %d\n",name, &lineHeight)!=2)
    {
		fclose(pFile);
        return 0;
    }
   
    averageCharLen=0;
    ratio = size/lineHeight;
    g_base = glGenLists(256);                           // Creating 256 Display Lists
    glBindTexture(GL_TEXTURE_2D, s_fontTexture->getID());          // Select Our s_font Texture

    while(fscanf(pFile,"%d %d %d %d %d %d\n",&charNum,&posX,&posY,&sizeX,&sizeY,&leftSpacing)==6)
    {
        theSize[charNum].sizeX=sizeX;
        theSize[charNum].sizeY=sizeY;
        theSize[charNum].leftSpacing=leftSpacing;
        theSize[charNum].rightSpacing=0;
        averageCharLen+=sizeX;
        nbChar++;

	//	cout << charNum << " " << posX << ":" << posY << endl;

	// Special ascii code used to set text color
	// was R,G,B, but changed to normal or hilighted since R G or B was hard to see - rms
	if (charNum==17 || charNum==18)
	  {
	    glNewList(g_base+charNum,GL_COMPILE);
	    if( charNum==17 ) {
	      // regular text color
	      glColor3f(0.5,1,0.5);
	    } else {
	      // hilighted text
	      glColor3f(1,1,1);
	    }
	    
	    glEndList();
	    continue;
	  }
	
        glNewList(g_base+charNum,GL_COMPILE); {  // Start Building A List
	  glTranslated(leftSpacing*ratio,0,0);	 // Move To The Left Of The Character
	  glBegin(GL_QUADS );
	  glTexCoord2f((float)posX/256,(float)(256-posY-sizeY)/256);
	  glVertex3f(0,sizeY*ratio,0); //  Bottom Left
	  glTexCoord2f((float)(posX+sizeX)/256,(float)(256-posY-sizeY)/256);
	  glVertex3f(sizeX*ratio,sizeY*ratio,0);  // Bottom Right
	  glTexCoord2f((float)(posX+sizeX)/256,(float)(256-posY)/256);
	  glVertex3f(sizeX*ratio,0,0); // Top Right
	  glTexCoord2f((float)posX/256,(float)(256-posY)/256);
	  glVertex3f(0,0,0); // Top Left
	  glEnd ();
	  glTranslated((sizeX+SPACING)*ratio,0,0); }
        glEndList();   // Done Building The Display List
    }

    fclose(pFile);
    
    averageCharLen/=nbChar;
    
    return 1;
}


void s_font::print(float x, float y, const string& str, int upsidedown) const
{
    glBindTexture(GL_TEXTURE_2D, s_fontTexture->getID());  // Select Our s_font Texture
    glPushMatrix();
    glTranslatef(x,y,0);  // Position The Text (0,0 - Top Left)
	if (upsidedown) glScalef(1, -1, 1);  // invert the y axis, down is positive
    glListBase(g_base);	 // Init the Display list base
    glCallLists(str.length(), GL_UNSIGNED_BYTE, str.c_str());  // Write The Text To The Screen
    glPopMatrix();
}

void s_font::print_char(const unsigned char c) const
{
  glBindTexture(GL_TEXTURE_2D, s_fontTexture->getID());  // Select Our s_font Texture
  glCallList(g_base+c);
}

// print with dark outline
// this is somewhere between a hack and a kludge
void s_font::print_char_outlined(const unsigned char c) const
{

  GLfloat current_color[4];
  glGetFloatv(GL_CURRENT_COLOR, current_color);	 
 	 	 
  glBindTexture(GL_TEXTURE_2D, s_fontTexture->getID());  // Select Our s_font Texture

  glColor3f(0.2,0.2,0.2);

  glPushMatrix();
  glTranslatef(1,1,0);		
  glCallList(g_base+c);	
  glPopMatrix();

  glPushMatrix();
  glTranslatef(-1,-1,0);		
  glCallList(g_base+c);	
  glPopMatrix();

  glPushMatrix();
  glTranslatef(1,-1,0);		
  glCallList(g_base+c);	
  glPopMatrix();

  glPushMatrix();
  glTranslatef(-1,1,0);		
  glCallList(g_base+c);	
  glPopMatrix();

  glColor3f(current_color[0],current_color[1],current_color[2]);
  glCallList(g_base+c);

}

float s_font::getStrLen(const string& str) const
{
	if (str.empty()) return 0;
    float s=0;
    for (unsigned int i=0;i<str.length();++i)
    { 
		s += theSize[(unsigned int)str[i]].sizeX + theSize[(unsigned int)str[i]].leftSpacing +
			theSize[(unsigned int)str[i]].rightSpacing + SPACING;
    }
    return s*ratio;
}

float s_font::getStrHeight(const string& str) const
{  
    float s=0;
    for (unsigned int i=0;i<str.length();++i)
    {
		if (s<theSize[(unsigned int)str[i]].sizeY) s=theSize[(unsigned int)str[i]].sizeY;
    }
    return s*ratio;
}

