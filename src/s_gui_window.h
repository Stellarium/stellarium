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
    StdWin(int posx, int posy, int sizex, int sizey, char * title, Component * parent, s_font * winfont_);
	virtual ~StdWin();
    virtual void StdWinClicCallback(enum guiValue button,enum guiValue state);
    virtual void StdWinMoveCallback(int x, int y,enum guiValue action);
    virtual void render(GraphicsContext& gc);
    virtual const char * getTitle() const;
    virtual void setTitle(char * _title);
    virtual void addComponent(Component * c) {theContainer->addComponent(c);}
    virtual vec2_i getInSize();
    virtual void setInSize(vec2_i);
protected:
    int headerSize;
    Container * theContainer;
private:
    bool mouseOn;
    vec2_i lastXY;
    char title[128];
    bool overBar;
    s_font * winfont;
};

class StdBtWin : public StdWin
{
public:
    StdBtWin(int posx, int posy, int sizex, int sizey, char * title, Component * parent, s_font * winfont_);
    void Hide(void);
    void setHideCallback(void (*_onHideCallback)(void))
    {   
        onHideCallback=_onHideCallback;
    }
    void StdBtWin::setInSize(vec2_i newInSize);
protected:
    Button * closeBt;
private:
    void (*onHideCallback)(void);
};

#endif  //_S_GUI_WINDOW_H_
