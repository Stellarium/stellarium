/* 
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
 * Inspired by the s_gui.h by Chris Laurel <claurel@shatters.net>
 * in his Open Source Software Celestia
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

#ifndef _GUI_H_
#define _GUI_H_

#include <vector>
#include "s_font.h"
#include "vector.h"
#include "s_texture.h"

// gui Return Values:
enum guiValue
{   GUI_MOUSE_RIGHT,
    GUI_MOUSE_LEFT,
    GUI_MOUSE_MIDDLE,
    GUI_MOUSE_ENTER,
    GUI_MOUSE_LEAVE,
    GUI_RIGHT_CLIC,
    GUI_LEFT_CLIC,
    GUI_MIDDLE_CLIC,
    GUI_NOTHING,
    GUI_UP,
    GUI_DOWN
};

namespace gui
{
    class GraphicsContext
    {
    public:
        GraphicsContext(int _winW,int _winH);
        ~GraphicsContext();
        s_font* getFont() const;
        void setFont(s_font*);
        void setWinWH(int _winW,int _winH) {winW=_winW;winH=_winH;}
        s_texture * backGroundTexture;
        s_texture * headerTexture;
        vec3_t baseColor;
        vec3_t textColor;
        vec2_i scissorPos;
        int winW, winH;
    private:
        s_font* font;
    };


    class Component
    {
    friend class Container;
    public:
        Component();
        vec2_i getPosition() const;
        vec2_i getSize() const;
        void reshape(vec2_i, vec2_i);
        Component* getParent() const;
        virtual void render(GraphicsContext&) = 0;
        void setClicCallback(void (*_clicCallback)(int,int,enum guiValue,enum guiValue,Component *));
        void setMoveCallback(void (*_moveCallback)(int,int,enum guiValue,Component *));
        void setParent(Component*);
        void setVisible(int _visible) {visible=_visible;}
        int getVisible(void) {return visible;}
        void setActive(int _active) {active = _active;}
        int getActive(void) {return active;}
        int draging;
        void setID(int _ID) {ID=_ID;}
        int getID(void) {return ID;}
    private:
    protected:
        int passThru;
        int ID;
        int visible;
        int isIn(float x , float y)
        {   return (position[0]<=x && (size[0]+position[0])>=x && 
                    position[1]<=y && (position[1]+size[1])>=y);
        }
        vec2_i position;
        vec2_i size;
        Component* parent;
        void (*clicCallback)(int,int,enum guiValue,enum guiValue,Component *);
        void (*moveCallback)(int,int,enum guiValue,Component *);
        void (*keyCallback)(int,enum guiValue,Component *);
        int mouseIn;
        int active;
    };


    class Container : public Component
    {
    public:
        Container();
        virtual ~Container();
        int getComponentCount() const;
        Component* getComponent(int) const;
        void addComponent(Component*);
        void render(GraphicsContext&);
        int handleMouseClic(int x, int y, enum guiValue, enum guiValue);
        void handleMouseMove(int x, int y);
    protected:
        std::vector<Component*> components;
    };

    class FilledContainer : public Container
    {
    public:
        void render(GraphicsContext&);
    };

    class Button : public Component
    {
    public:
        Button();
        void render(GraphicsContext&);
        void setOnClicCallback(void (*_onClicCallback)(enum guiValue,Component *));
        void setOnMouseOverCallback(void (*_onMouseOverCallback)(enum guiValue,Component *));
        void ButtonClicCallback(enum guiValue button,enum guiValue state);
        void ButtonMoveCallback(enum guiValue action);
    protected:
        void (*onClicCallback)(enum guiValue,Component *);
        void (*onMouseOverCallback)(enum guiValue,Component *);
        int mouseOn;
    };

    class Labeled_Button : public Button
    {
    public:
        Labeled_Button(char * _label);
        ~Labeled_Button();
        const char * getLabel() const;
        void setLabel(char *);
        void render(GraphicsContext& gc);
    private:
        char * label;
    };

    class Textured_Button : public Button
    {
    public:
        Textured_Button(s_texture * _texBt);
        Textured_Button(s_texture * _texBt, vec2_i _position, vec2_i _size ,vec3_t _activeColor, vec3_t _passiveColor, void (*_onClicCallback)(enum guiValue,Component *),void (*_onMouseOverCallback)(enum guiValue,Component *), int _ID, int _active);
        ~Textured_Button();
        void render(GraphicsContext& gc);
    private:
        s_texture * texBt;
        vec3_t activeColor;
        vec3_t passiveColor;
    };

    class Label : public Component
    {
    public:
        Label(char * _label);
        Label(char * _label, s_font * _theFont);
        ~Label();
        const char * getLabel() const;
        void setLabel(char *);
        void render(GraphicsContext&);
        void setColour(vec3_t _colour) 
        {   colour=_colour;
        }
    private:
        char * label;
        s_font * theFont;
        vec3_t colour;
    };

    class TextLabel : public Container
	{
	public:
	    TextLabel(char * _label, s_font * _theFont);
	    ~TextLabel();
	    const char * getLabel() const;
	    void setLabel(char *);
	    void setColour(vec3_t _colour);
	protected:
	    char * label;
	    s_font * theFont;
	    vec3_t colour;
	};
    
    class FilledTextLabel : public TextLabel
	{
	public:
	    FilledTextLabel(char * _label, s_font * _theFont);
	    ~FilledTextLabel();
	    void render(GraphicsContext&);
	private:
	};
    
    class CursorBar : public Component
	{
	public:
	    CursorBar(vec2_i _position, vec2_i _size, float _minBarValue, float _maxBarValue, float _barValue, void(*_onValueChangeCallBack)(float _barValue, Component *));
	    void render(GraphicsContext&);
	    void CursorBarClicCallback(int x, enum guiValue button,enum guiValue state);
	    void CursorBarMoveCallback(int x, enum guiValue action);
        float getValue(void) { return barValue;}
	private:
	    int mouseOn;	    
	    float minBarValue, maxBarValue, barValue;
	    void (*onValueChangeCallBack)(float,Component *);
	};
};

#endif // _GUI_H_
