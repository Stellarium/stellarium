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

#include "s_gui_window.h"

void ExtStdWinClicCallback(int x,int y,enum guiValue state,enum guiValue button,Component * me)
{   
    ((StdWin*)me)->StdWinClicCallback(button, state);
    ((Container *)me)->handleMouseClic((int)(x-me->getPosition()[0]),(int)(y-me->getPosition()[1]),state,button);
}

void ExtStdWinMoveCallback(int x,int y,enum guiValue action,Component * me)
{   
    ((StdWin*)me)->StdWinMoveCallback(x,y,action);
    ((Container *)me)->handleMouseMove((int)(x-me->getPosition()[0]),(int)(y-me->getPosition()[1]));
}

StdWin::StdWin(int posx, int posy, int sizex, int sizey, char * _title, Component * parent, s_font * winfont_)
 : Container(), mouseOn(false), winfont(winfont_)
{
    if (!winfont)
    {
        printf("ERROR WHILE CREATING WINDOW : NO FONT\n");
        exit(1);
    }
    setTitle(_title);
    reshape(posx,posy,sizex,sizey);
    setClicCallback(ExtStdWinClicCallback);
    setMoveCallback(ExtStdWinMoveCallback);
    theContainer = new Container();
    headerSize = (int)winfont->getLineHeight()+3;
    theContainer->reshape(0,headerSize, sizex,sizey-headerSize);
    Container::addComponent(theContainer);
}

StdWin::~StdWin()
{
    //printf("Enter StdWin destructor %d\n",this);
	//printf("Quit StdWin destructor %d\n",this);
}

void StdWin::StdWinClicCallback(enum guiValue button,enum guiValue state)
{   
    if (state==GUI_DOWN)
    {
	mouseOn=true;
        if (overBar) draging=true;
    }
    else
    {   if (state==GUI_UP) mouseOn=false;
        draging=false;
    }
}

void StdWin::StdWinMoveCallback(int x, int y,enum guiValue action)
{   if (action==GUI_MOUSE_ENTER)
    {   mouseOn=false;
    }
    if (action==GUI_NOTHING && !mouseOn)
    {   lastXY[0]=x;
        lastXY[1]=y;
    }
    if (y<=position[1]+headerSize)
    {   overBar=true;
    }
    else overBar=false;
    if (draging)
    {   position[0]+=x-lastXY[0];
        position[1]+=y-lastXY[1];
        lastXY[0]=x;
        lastXY[1]=y;
    }
}

void StdWin::render(GraphicsContext& gc)
{
    vec2_i pos = getPosition();
    vec2_i sz = getSize();
    glColor3f(gc.baseColor[0],gc.baseColor[1],gc.baseColor[2]);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D,gc.backGroundTexture->getID());
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f,0.0f);    glVertex2i(pos[0], pos[1] + sz[1]);    // Bas Gauche
        glTexCoord2f(1.0f,0.0f);    glVertex2i(pos[0] + sz[0], pos[1] + sz[1]);   // Bas Droite
        glTexCoord2f(1.0f,1.0f);    glVertex2i(pos[0] + sz[0], pos[1]);  // Haut Droit
        glTexCoord2f(0.0f,1.0f);    glVertex2i(pos[0], pos[1]);   // Haut Gauche
    glEnd();
    glBindTexture(GL_TEXTURE_2D,gc.headerTexture->getID());
    glBegin(GL_QUADS );
        glTexCoord2f(0.0f,0.0f);    glVertex2i(pos[0],        pos[1]);    // Bas Gauche
        glTexCoord2f(1.0f,0.0f);    glVertex2i(pos[0] + sz[0], pos[1]);   // Bas Droite
        glTexCoord2f(1.0f,1.0f);    glVertex2i(pos[0] + sz[0], pos[1] + headerSize); // Haut Droit
        glTexCoord2f(0.0f,1.0f);    glVertex2i(pos[0],        pos[1] + headerSize);  // Haut Gauche
    glEnd ();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    glBegin(GL_LINE_LOOP);
        glVertex2f(pos[0]        +0.5, pos[1]        +0.5);
        glVertex2f(pos[0] + sz[0]-0.5, pos[1]        +0.5);
        glVertex2f(pos[0] + sz[0]-0.5, pos[1] + sz[1]-0.5);
        glVertex2f(pos[0]        +0.5, pos[1] + sz[1]-0.5);
    glEnd();

    glBegin(GL_LINES);
        glVertex2f(pos[0]+0.5,        pos[1]+headerSize);
        glVertex2f(pos[0] + sz[0]-0.5, pos[1]+headerSize);
    glEnd();

    glColor3f(gc.textColor[0],gc.textColor[1],gc.textColor[2]);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glPushMatrix();
    glTranslatef(pos[0] + (sz[0] - winfont->getStrLen(title)) / 2,
                 pos[1] + (headerSize - winfont->getLineHeight()) / 2,
                 0);
    winfont->print(0,0,title);
    glPopMatrix();
    Container::render(gc);
}

const char * StdWin::getTitle() const
{
    return title;
}

void StdWin::setTitle(char * _title)
{   strncpy(title,_title,127);
}


vec2_i StdWin::getInSize()
{   
    return theContainer->getSize();
}

void StdWin::setInSize(vec2_i newInSize)
{   
    reshape(getPosition(),(vec2_i(0,headerSize)+newInSize));
}

/**** StdBtWin ***/
void closeBtOnClicCallback(guiValue button,Component * me)
{
   ((StdBtWin*)me->getParent())->Hide();
}

StdBtWin::StdBtWin(int posx, int posy, 
                    int sizex, int sizey,
                    char * title, Component * parent, s_font * winfont_)
                    : StdWin(posx,posy,sizex,sizey,title,parent,winfont_), closeBt(NULL), onHideCallback(NULL)
{
    closeBt = new Button();
    closeBt->setOnClicCallback(closeBtOnClicCallback);
    closeBt->reshape(size[0]-headerSize,2,headerSize-4,headerSize-4);
    Container::addComponent(closeBt);
}

void StdBtWin::Hide(void)
{
    if (onHideCallback!=NULL) onHideCallback();
}

void StdBtWin::setInSize(vec2_i newInSize)
{   
    StdWin::setInSize(newInSize);
    closeBt->reshape(size[0]-headerSize,2,headerSize-4,headerSize-4);
}
