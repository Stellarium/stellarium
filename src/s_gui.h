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
	S_GUI_MOUSE_LEFT,
	S_GUI_MOUSE_RIGHT,
    S_GUI_MOUSE_MIDDLE,
	S_GUI_PRESSED,
	S_GUI_RELEASED
};

namespace s_gui
{
	typedef CBFunctor0 * s_pcallback0;
	typedef CBFunctor0 s_callback0;

	//typedef CBFunctor2wRet<int,int,int> *t_moveCallback;
	//typedef CBFunctor2wRet<SDLKey,S_GUI_VALUE,int> *t_keyCallback;


	typedef Vec4f s_color;
	typedef Vec4i s_square;
	typedef Vec2i s_vec2i;
	typedef Vec4i s_vec4i;

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
		Painter(const s_texture* _tex1, const s_font* _font, const s_color& _baseColor, const s_color& _textColor);
		void drawSquareEdge(const s_vec2i& pos, const s_vec2i& sz) const;
		void drawSquareEdge(const s_vec2i& pos, const s_vec2i& sz, const s_color& c) const;
		void drawSquareFill(const s_vec2i& pos, const s_vec2i& sz) const;
		void drawSquareFill(const s_vec2i& pos, const s_vec2i& sz, const s_color& c) const;
		void drawSquareFill(const s_vec2i& pos, const s_vec2i& sz, const s_color& c, const s_texture * t) const;
		void drawCross(const s_vec2i& pos, const s_vec2i& sz) const;
		void print(int x, int y, const char * str) const;
		void setTexture(const s_texture* tex) {tex1 = tex;}
		void setFont(const s_font* f) {font = f;}
		void setTextColor(const s_color& c) {textColor = c;}
		void setBaseColor(const s_color& c) {baseColor = c;}
		const s_color& getBaseColor(void) const {return baseColor;}
		const s_font* getFont(void) const {return font;}
    private:
		const s_texture* tex1;
		const s_font* font;
		s_color baseColor;
		s_color textColor;
    };


    class Component
    {
    public:
        Component(void);
		virtual ~Component();
        virtual void draw(void) = 0;
        virtual void reshape(const s_vec2i& _pos, const s_vec2i& _size);
        virtual void reshape(int x, int y, int w, int h);
        virtual int getPosx() const {return pos[0];}
		virtual int getPosy() const {return pos[1];}
        virtual int getSizex() const {return size[0];}
		virtual int getSizey() const {return size[1];}
        virtual const s_vec2i getPos() const {return pos;}
        virtual const s_vec2i getSize() const {return size;}
        virtual void setPos(const s_vec2i& _pos) {pos = _pos;}
        virtual void setSize(const s_vec2i& _size) {size = _size;}
        virtual void setPos(int x, int y) {pos[0] = x; pos[1]=y;}
        virtual void setSize(int w, int h) {size[0] = w; size[1]=h;}
        virtual void setVisible(int _visible) {visible=_visible;}
        virtual int getVisible(void) const {return visible;}
        virtual void setActive(int _active) {active = _active;}
        virtual int getActive(void) const {return active;}
        virtual void setFocus(int _focus) {focus = _focus;};
        virtual int getFocus(void) const {return focus;}
		virtual int onClic(int, int, S_GUI_VALUE, S_GUI_VALUE) {return 0;}
		virtual int onMove(int, int) {return 0;}
		virtual int onKey(SDLKey, S_GUI_VALUE) {return 0;}
		virtual void setTexture(const s_texture* tex) {painter.setTexture(tex);}
		virtual void setFont(const s_font* f) {painter.setFont(f);}
		virtual void setTextColor(const s_color& c) {painter.setTextColor(c);}
		virtual void setBaseColor(const s_color& c) {painter.setBaseColor(c);}
		static void setDefaultPainter(const Painter& p) {defaultPainter=p;}
		static void initScissor(int winW, int winH);
		static void enableScissor(void) {scissor->activate();}
		static void disableScissor(void) {scissor->desactivate();}
    protected:
        s_vec2i pos;
        s_vec2i size;
        int visible;
        int active;
        int focus;
		Painter painter;
		virtual int isIn(int x, int y);
		static Painter defaultPainter;
		static Scissor* scissor;
    private:
    };

    class CallbackComponent : public Component
    {
    public:
		CallbackComponent();
		virtual void setOnMouseInOutCallback(const s_callback0& c) {onMouseInOutCallback = c;}
        virtual void setOnPressCallback(const s_callback0& c) {onPressCallback = c;}
		virtual int getIsMouseOver(void) {return is_mouse_over;}
		virtual int onMove(int, int);
		virtual int onClic(int, int, S_GUI_VALUE, S_GUI_VALUE);
    protected:
		s_callback0 onPressCallback;
		s_callback0 onMouseInOutCallback;
		int is_mouse_over;
	};

    class Container : public CallbackComponent
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


    class FramedContainer : public Container
    {
    public:
		FramedContainer();
		virtual void addComponent(Component* c) {inside->addComponent(c);}
        virtual void reshape(const s_vec2i& _pos, const s_vec2i& _size);
        virtual void reshape(int x, int y, int w, int h);
        virtual int getSizex() const {return inside->getSizex();}
		virtual int getSizey() const {return inside->getSizey();}
        virtual const s_vec2i getSize() const {return inside->getSize();}
        virtual void setSize(const s_vec2i& _size);
        virtual void setSize(int w, int h);
        virtual void draw(void);
		virtual void setFrameSize(int left, int right, int top, int bottom);
	protected:
		Container* inside;
		s_vec4i frameSize;
    };


    class Button : public CallbackComponent
    {
    public:
		Button();
        virtual void draw();
    protected:
    };

    class FilledButton : public Button
    {
    public:
        virtual void draw();
    protected:
    };

	class CheckBox : public Button
    {
    public:
		CheckBox(int state = 0);
        virtual void draw();
		virtual int getState(void) const {return isChecked;}
		virtual int onClic(int, int, S_GUI_VALUE, S_GUI_VALUE);
    protected:
		int isChecked;
    };

	class FlagButton : public CheckBox
    {
    public:
		FlagButton(int state = 0, const s_texture* tex = NULL, const char* specificTexName = NULL);
        virtual ~FlagButton();
		virtual void draw();
    protected:
		s_texture* specific_tex;
    };

    class Label : public Component
    {
    public:
        Label(const char* _label  = NULL, const s_font* _font = NULL);
        virtual ~Label();
        virtual const char * getLabel() const {return label;}
        virtual void setLabel(const char *);
        virtual void draw();
		virtual void adjustSize(void);
    protected:
        char * label;
    };

    class Labeled_Button : public FilledButton, Label
    {
    public:
        Labeled_Button(const char * _label = NULL);
        virtual void draw();
    private:
    };

/*
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
