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

#include <list>

#include "s_texture.h"
#include "s_font.h"
#include "vecmath.h"
#include "callback.h"

// gui Return Values:
enum S_GUI_VALUE
{
	S_GUI_MOUSE_RIGHT,
    S_GUI_MOUSE_LEFT,
    S_GUI_MOUSE_MIDDLE,
    S_GUI_MOUSE_ENTER,
    S_GUI_MOUSE_LEAVE,
    S_GUI_RIGHT_CLIC,
    S_GUI_LEFT_CLIC,
    S_GUI_MIDDLE_CLIC,
    S_GUI_NOTHING,
    S_GUI_UP,
    S_GUI_DOWN,
	S_GUI_KEY_PRESS,
	S_GUI_KEY_RELEASE
};

namespace s_gui
{
	typedef CBFunctor4wRet<int,int,S_GUI_VALUE,S_GUI_VALUE,int> *t_clicCallback;
	typedef CBFunctor2wRet<int,int,int> *t_moveCallback;
	typedef CBFunctor2wRet<SDLKey,S_GUI_VALUE,int> *t_keyCallback;


	typedef Vec4f s_color;
	typedef Vec4i s_square;
	typedef Vec2i s_vec2i;

	class Scissor
	{
	public:
		Scissor(int _winW, int _winH);
		void push(int posx, int posy, int sizex, int sizey);
		void push(const s_vec2i& pos, const s_vec2i& size);
		void pop(void);
		void activate(void) {glEnable(GL_SCISSOR_TEST);}
		void desactivate(void) {glDisable(GL_SCISSOR_TEST);};
	private :
		int winW, winH;
		std::list<s_square> stack;
	};

    class Painter
    {
    public:
        Painter();
		Painter(s_texture* _tex1, s_font* _font, const s_color& _baseColor, const s_color& _textColor);
        s_texture* tex1;
        s_font* font;
		s_color baseColor;
		s_color textColor;
		void drawSquareEdge(const s_vec2i& pos, const s_vec2i& sz) const;
		void drawSquareEdge(const s_vec2i& pos, const s_vec2i& sz, const s_color& c) const;
		void drawSquareFill(const s_vec2i& pos, const s_vec2i& sz) const;
		void drawSquareFill(const s_vec2i& pos, const s_vec2i& sz, const s_color& c) const;
		void drawSquareFill(const s_vec2i& pos, const s_vec2i& sz, const s_color& c, const s_texture * t) const;
		void print(int x, int y, const char * str) const;
    private:
    };


    class Component
    {
    public:
        Component(void);
		virtual ~Component();
        virtual void draw(void) = 0;
        virtual void reshape(s_vec2i _pos, s_vec2i _size);
        virtual void reshape(int x, int y, int w, int h);
        virtual int getPosx() const {return pos[0];}
		virtual int getPosy() const {return pos[1];}
        virtual int getSizex() const {return size[0];}
		virtual int getSizey() const {return size[1];}
        virtual const s_vec2i getPos() const {return pos;}
        virtual const s_vec2i getSize() const {return size;}
        virtual void setVisible(int _visible) {visible=_visible;}
        virtual int getVisible(void) const {return visible;}
        virtual void setActive(int _active) {active = _active;}
        virtual int getActive(void) const {return active;}
        virtual void setFocus(int _focus) {focus = _focus;};
        virtual int getFocus(void) const {return focus;}
		virtual int onClic(int, int, S_GUI_VALUE, S_GUI_VALUE) {return 0;}
		virtual int onMove(int, int) {return 0;}
		virtual int onKey(SDLKey, S_GUI_VALUE) {return 0;}
		static void setDefaultPainter(const Painter& p) {defaultPainter=p;}
		static void initScissor(int winW, int winH) {scissor = Scissor(winW, winH);}
		static void enableScissor(void) {scissor.activate();}
		static void disableScissor(void) {scissor.desactivate();}
    protected:
        s_vec2i pos;
        s_vec2i size;
        int visible;
        int active;
        int focus;
		Painter painter;
		virtual int isIn(float x , float y);
		static Painter defaultPainter;
		static Scissor scissor;
    private:
    };


    class Container : public Component
    {
    public:
        Container();
        virtual ~Container();
        virtual void addComponent(Component*);
        virtual void draw(void);
		virtual int onClic(int, int, S_GUI_VALUE, S_GUI_VALUE);
		virtual int onMove(int, int);
		virtual int onKey(SDLKey, S_GUI_VALUE);
    protected:
        std::list<Component*> childs;
    };


    class FilledContainer : public Container
    {
    public:
        virtual void draw(void);
    };

	/*
		t_clicCallback clicCallback;
		t_moveCallback moveCallback;
		t_keyCallback  keyCallback;
        virtual void setClicCallback(s_clicCallback);
        virtual void setMoveCallback(s_moveCallback);
        virtual void setKeyCallback(s_keyCallback);
	*/

	/*
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
        Textured_Button(s_texture* _texBt);
        Textured_Button(s_texture* _texBt, vec2_i _position, vec2_i _size ,vec3_t _activeColor, vec3_t _passiveColor, Callback1Base* _c1, Callback1Base* _c2, int _ID, int _active);
        virtual ~Textured_Button();
        virtual void render(GraphicsContext& gc);
    private:
		Callback1Base * c1;
		Callback1Base * c2;
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
*/
};

#endif // _GUI_H_
