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
{   ((StdWin*)me)->StdWinClicCallback(button, state);
    ((Container *)me)->handleMouseClic((int)(x-me->getPosition()[0]),(int)(y-me->getPosition()[1]),state,button);
}

void ExtStdWinMoveCallback(int x,int y,enum guiValue action,Component * me)
{   ((StdWin*)me)->StdWinMoveCallback(x,y,action);
    ((Container *)me)->handleMouseMove((int)(x-me->getPosition()[0]),(int)(y-me->getPosition()[1]));
}

StdWin::StdWin(float posx, float posy, float sizex, float sizey, char * _title, Component * parent)
 : Container(), mouseOn(false)
{   setTitle(_title);
    reshape(vec2_i(posx,posy),vec2_i(sizex,sizey));
    setClicCallback(ExtStdWinClicCallback);
    setMoveCallback(ExtStdWinMoveCallback);
    theContainer = new Container();
    Container::addComponent(theContainer);
}


void StdWin::StdWinClicCallback(enum guiValue button,enum guiValue state)
{   if (state==GUI_DOWN)
    {   mouseOn=true;
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
    headerSize = gc.getFont()->getLineHeight()+2;
    theContainer->reshape(vec2_i(0,headerSize), vec2_i(sz[0],sz[1]-headerSize));
    glColor3f(gc.baseColor[0],gc.baseColor[1],gc.baseColor[2]);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D,gc.backGroundTexture->getID());
    glBegin(GL_QUADS );
        glTexCoord2f(0.0f,0.0f);    glVertex2i(pos[0],        pos[1] + sz[1]);    // Bas Gauche
        glTexCoord2f(1.0f,0.0f);    glVertex2i(pos[0] + sz[0], pos[1] + sz[1]);   // Bas Droite
        glTexCoord2f(1.0f,1.0f);    glVertex2i(pos[0] + sz[0], pos[1]);  // Haut Droit
        glTexCoord2f(0.0f,1.0f);    glVertex2i(pos[0],        pos[1]);   // Haut Gauche
    glEnd ();
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
        glVertex2i(pos[0],        pos[1]);
        glVertex2i(pos[0] + sz[0], pos[1]);
        glVertex2i(pos[0] + sz[0], pos[1] + sz[1]);
        glVertex2i(pos[0],        pos[1] + sz[1]);
    glEnd();

    glBegin(GL_LINES);
        glVertex2i(pos[0],        pos[1]+headerSize);
        glVertex2i(pos[0] + sz[0], pos[1]+headerSize);
    glEnd();

    glColor3f(gc.textColor[0],gc.textColor[1],gc.textColor[2]);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glPushMatrix();
    glTranslatef(pos[0] + (sz[0] - gc.getFont()->getStrLen(title)) / 2,
                 pos[1] + (headerSize - gc.getFont()->getLineHeight()) / 2,
                 0);
    gc.getFont()->print(0,0,title);
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


vec2_i StdWin::getInSize(GraphicsContext& gc)
{   
    vec2_i sz = getSize();
    headerSize = gc.getFont()->getLineHeight()+2;
    theContainer->reshape(vec2_i(0,headerSize), vec2_i(sz[0],sz[1]-headerSize));
    return theContainer->getSize();
}

void StdWin::setInSize(vec2_i newInSize, GraphicsContext& gc)
{   
    vec2_i sz = getSize();
    headerSize = gc.getFont()->getLineHeight()+2;
    reshape(getPosition(),vec2_i(0,headerSize)+newInSize);
}

/**** StdBtWin ***/
void closeBtOnClicCallback(guiValue button,Component * me)
{
   ((StdBtWin*)me->getParent())->Hide();
}

StdBtWin::StdBtWin(float posx, float posy, 
                    float sizex, float sizey,
                    char * title, Component * parent)
                    : StdWin(posx,posy,sizex,sizey,title,parent), onHideCallback(NULL), closeBt(NULL)
{
    closeBt = new Button();
    closeBt->setOnClicCallback(closeBtOnClicCallback);
    Container::addComponent(closeBt);
}

void StdBtWin::Hide(void)
{
    if (onHideCallback!=NULL) onHideCallback();
}

void StdBtWin::render(GraphicsContext& gc)
{
    vec2_i pos = getPosition();
    vec2_i sz = getSize();
    vec2_i P(size[0]-gc.getFont()->getLineHeight(),2);
    vec2_i V(gc.getFont()->getLineHeight()-4,gc.getFont()->getLineHeight()-4);
    if(closeBt) closeBt->reshape(P,V);
    StdWin::render(gc);
}