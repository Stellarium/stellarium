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

// Class to manage windows in gui

#ifndef _S_GUI_WINDOW_H_
#define _S_GUI_WINDOW_H_

#include "s_gui.h"

using namespace gui;

class StdWin : public Container
{
public:
    StdWin(float posx, float posy, float sizex, float sizey, char * title, Component * parent);
    void StdWinClicCallback(enum guiValue button,enum guiValue state);
    void StdWinMoveCallback(int x, int y,enum guiValue action);
    void render(GraphicsContext& gc);
    const char * getTitle() const;
    void setTitle(char * _title);
    void addComponent(Component * c) {theContainer->addComponent(c);}
    vec2_i getInSize(GraphicsContext& gc);
    void setInSize(vec2_i, GraphicsContext& gc);
protected:
    float headerSize;
    Container * theContainer;
private:
    bool mouseOn;
    vec2_i lastXY;
    char title[128];
    bool overBar;
};

class StdBtWin : public StdWin
{
public:
    StdBtWin(float posx, float posy, float sizex, float sizey, char * title, Component * parent);
    void Hide(void);
    void setHideCallback(void (*_onHideCallback)(void))
    {   onHideCallback=_onHideCallback;
    }
    void StdBtWin::render(GraphicsContext& gc);
protected:
    Button * closeBt;
private:
    void (*onHideCallback)(void);
};

#endif  //_S_GUI_WINDOW_H_
