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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef MACOSX
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif

#include "SDL.h" // Just for the key codes, i'm lasy to redefine them
		 // This is to do to make the s_ library independent

#include <vector>
#include "s_font.h"
#include "vecmath.h"
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
		virtual ~Component();
        virtual vec2_i getPosition() const;
        virtual vec2_i getSize() const;
        virtual void reshape(vec2_i, vec2_i);
        virtual void reshape(int x, int y, int w, int h);
        virtual Component* getParent() const;
        virtual void render(GraphicsContext&) = 0;
        virtual void setClicCallback(void (*_clicCallback)(int,int,enum guiValue,enum guiValue,Component *));
        virtual void setMoveCallback(void (*_moveCallback)(int,int,enum guiValue,Component *));
        virtual void setKeyCallback(int (*_keyCallback)(SDLKey key,int state,Component *));
        virtual void setParent(Component*);
        virtual void setVisible(int _visible) {visible=_visible;}
        virtual int getVisible(void) {return visible;}
        virtual void setActive(int _active) {active = _active;}
        virtual int getActive(void) {return active;}
        //TODO virtual void setFocus(int _focus);
        virtual int getFocus(void) {return focus;}
        virtual void setID(int _ID) {ID=_ID;}
        virtual int getID(void) {return ID;}
		int draging;
    private:
    protected:
        int passThru;
        int ID;
        int visible;
        virtual int isIn(float x , float y)
        {   return (position[0]<=x && (size[0]+position[0])>=x && 
                    position[1]<=y && (position[1]+size[1])>=y);
        }
        vec2_i position;
        vec2_i size;
        Component* parent;
        void (*clicCallback)(int,int,enum guiValue,enum guiValue,Component *);
        void (*moveCallback)(int,int,enum guiValue,Component *);
        int (*keyCallback)(SDLKey key,int state,Component *);
        int mouseIn;
        int active;
        int focus;
    };


    class Container : public Component
    {
    public:
        Container();
        virtual ~Container();
        virtual int getComponentCount() const;
        virtual Component* getComponent(int) const;
        virtual void addComponent(Component*);
        virtual void render(GraphicsContext&);
        virtual int handleMouseClic(int x, int y, enum guiValue, enum guiValue);
        virtual void handleMouseMove(int x, int y);
        virtual int handleKey(SDLKey key,int state);
    protected:
        std::vector<Component*> components;
    };

    class FilledContainer : public Container
    {
    public:
        virtual void render(GraphicsContext&);
    };

    class Button : public Component
    {
    public:
        Button();
        virtual ~Button();
        virtual void render(GraphicsContext&);
        virtual void setOnClicCallback(void (*_onClicCallback)(enum guiValue,Component *));
        virtual void setOnMouseOverCallback(void (*_onMouseOverCallback)(enum guiValue,Component *));
        virtual void ButtonClicCallback(enum guiValue button,enum guiValue state);
        virtual void ButtonMoveCallback(enum guiValue action);
    protected:
        int mouseOn;
        void (*onClicCallback)(enum guiValue,Component *);
        void (*onMouseOverCallback)(enum guiValue,Component *);
    };

    class Labeled_Button : public Button
    {
    public:
        Labeled_Button(char * _label);
        virtual ~Labeled_Button();
        virtual const char * getLabel() const;
        virtual void setLabel(char *);
        virtual void render(GraphicsContext& gc);
    private:
        char * label;
    };

    class Textured_Button : public Button
    {
    public:
        Textured_Button(s_texture * _texBt);
        Textured_Button(s_texture * _texBt, vec2_i _position, vec2_i _size ,vec3_t _activeColor, vec3_t _passiveColor, void (*_onClicCallback)(enum guiValue,Component *),void (*_onMouseOverCallback)(enum guiValue,Component *), int _ID, int _active);
        virtual ~Textured_Button();
        virtual void render(GraphicsContext& gc);
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
        virtual ~Label();
        virtual const char * getLabel() const;
        virtual void setLabel(char *);
        virtual void render(GraphicsContext&);
        virtual void setColour(vec3_t _colour) {colour=_colour;}
    private:
        char * label;
        s_font * theFont;
        vec3_t colour;
    };

    class TextLabel : public Container
	{
	public:
	    TextLabel(char * _label, s_font * _theFont);
	    virtual ~TextLabel();
	    virtual const char * getLabel() const;
	    virtual void setLabel(char *);
	    virtual void setColour(vec3_t _colour);
	protected:
	    char * label;
	    s_font * theFont;
	    vec3_t colour;
	};
    
    class FilledTextLabel : public TextLabel
	{
	public:
	    FilledTextLabel(char * _label, s_font * _theFont);
	    //~FilledTextLabel();
	    virtual void render(GraphicsContext&);
	private:
	};

    class CursorBar : public Component
	{
	public:
	    CursorBar(vec2_i _position, vec2_i _size, float _minBarValue, float _maxBarValue, float _barValue, void(*_onValueChangeCallBack)(float _barValue, Component *));
	    virtual void render(GraphicsContext&);
	    virtual void CursorBarClicCallback(int x, enum guiValue button,enum guiValue state);
	    virtual void CursorBarMoveCallback(int x, enum guiValue action);
        virtual float getValue(void) { return barValue;}
		virtual void setValue(float _barValue);
	private:
	    int mouseOn;
	    float minBarValue, maxBarValue, barValue;
	    void (*onValueChangeCallBack)(float,Component *);
	};


    class Picture : public Component
	{
	public:
	    Picture(vec2_i _position, vec2_i _size, s_texture * _imageTex);
		virtual ~Picture();
	    virtual void render(GraphicsContext&);
	private:
	    s_texture * imageTex;
	};

    class BorderPicture : public Picture
	{
	public:
		BorderPicture(vec2_i _position, vec2_i _size, s_texture * _imageTex);
	    virtual void render(GraphicsContext&);
	};

	// Class used for the earth map position picker
    class ClickablePicture : public BorderPicture
	{
	public:
		ClickablePicture(vec2_i _position, vec2_i _size, s_texture * _imageTex, void(*_onValueChangeCallBack)(vec2_t _pointerPosition, Component *));
		virtual void setPointerPosition(vec2_t _pointerPosition);
		virtual vec2_t getPointerPosition(void) {return pointerPosition;}
	    virtual void render(GraphicsContext&);
		virtual void ClickablePictureClicCallback(int x, int y, enum guiValue button, enum guiValue state);
	private:
		void (*onValueChangeCallBack)(vec2_t, Component *);
		vec2_t pointerPosition;
	};

};

#endif // _GUI_H_
