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

// TODO : remove the maximum of char* and replace by string to make the soft safer

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

// SDL is used only for the key codes, i'm lasy to redefine them
// This is TODO to make the s_ library independent
#include "SDL.h"

#include <list>
#include <string>

#include "s_texture.h"
#include "s_font.h"
#include "vecmath.h"
#include "callbacks.hpp"

using namespace std;
using namespace boost;

namespace s_gui
{
	// gui Return Values:
	enum S_GUI_VALUE
	{
		S_GUI_MOUSE_LEFT,
		S_GUI_MOUSE_RIGHT,
    	S_GUI_MOUSE_MIDDLE,
		S_GUI_PRESSED,
		S_GUI_RELEASED
	};

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
		void drawLine(const s_vec2i& pos1, const s_vec2i& pos2) const;
		void drawLine(const s_vec2i& pos1, const s_vec2i& pos2, const s_color& c) const;
		void print(int x, int y, const string& str) const;
		void setTexture(const s_texture* tex) {tex1 = tex;}
		void setFont(const s_font* f) {font = f;}
		void setTextColor(const s_color& c) {textColor = c;}
		void setBaseColor(const s_color& c) {baseColor = c;}
		const s_color& getBaseColor(void) const {return baseColor;}
		const s_color& getTextColor(void) const {return textColor;}
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
        Component();
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
		virtual void setPainter(const Painter& p) {painter = p;}
		virtual int isIn(int x, int y);
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
		static Painter defaultPainter;
		static Scissor* scissor;
    private:
    };

    class CallbackComponent : public Component
    {
    public:
		CallbackComponent();
		virtual void setOnMouseInOutCallback(const callback<void>& c) {onMouseInOutCallback = c;}
        virtual void setOnPressCallback(const callback<void>& c) {onPressCallback = c;}
		virtual int getIsMouseOver(void) {return is_mouse_over;}
		virtual int onMove(int, int);
		virtual int onClic(int, int, S_GUI_VALUE, S_GUI_VALUE);
    protected:
		callback<void> onPressCallback;
		callback<void> onMouseInOutCallback;
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


    class Button : public CallbackComponent
    {
    public:
		Button();
        virtual void draw();
		int onClic(int x, int y, S_GUI_VALUE bt, S_GUI_VALUE state);
    protected:
    };

    class TexturedButton : public Button
    {
    public:
		TexturedButton(const s_texture* tex = NULL);
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
		virtual void setState(int s) {isChecked = s;}
		virtual int onClic(int, int, S_GUI_VALUE, S_GUI_VALUE);
    protected:
		int isChecked;
    };

	class FlagButton : public CheckBox
    {
    public:
		FlagButton(int state = 0, const s_texture* tex = NULL, const string& specificTexName = "");
        virtual ~FlagButton();
		virtual void draw();
    protected:
		s_texture* specific_tex;
    };

    class Label : public Component
    {
    public:
        Label(const string& _label = "", const s_font* _font = NULL);
        virtual ~Label();
        virtual const string& getLabel() const {return label;}
        virtual void setLabel(const string&);
        virtual void draw();
		virtual void adjustSize(void);
    protected:
        string label;
    };

    class LabeledButton : public Button
    {
    public:
        LabeledButton(const string& _label = "", const s_font* font = NULL);
		virtual ~LabeledButton();
        virtual void draw(void);
		virtual void setActive(int _active) {Button::setActive(_active); label.setActive(_active);}
		virtual void setFont(const s_font* f) {Button::setFont(f); label.setFont(f);}
		virtual void setTextColor(const s_color& c) {Button::setTextColor(c); label.setTextColor(c);}
		virtual void setPainter(const Painter& p) {Button::setPainter(p); label.setPainter(p);}
    protected:
		Label label;
    };


    class TextLabel : public Container
	{
	public:
	    TextLabel(const string& _label = "", const s_font* _font = NULL);
	    virtual ~TextLabel();
        virtual const string& getLabel() const {return label;}
        virtual void setLabel(const string&);
		virtual void adjustSize(void);
		virtual void setTextColor(const s_color& c);
	protected:
        string label;
	};

	class IntIncDec : public Container
	{
	public:
	    IntIncDec(const s_font* _font = NULL, const s_texture* tex_up = NULL,
			const s_texture* tex_down = NULL, int min = 0, int max = 9,
			int init_value = 0, int inc = 1);
	    virtual ~IntIncDec();
		virtual void draw();
        virtual const float getValue() const {return value;}
		virtual void setValue(int v) {value=v; if(value>max) value=max; if(value<min) value=min;}
	protected:
		void inc_value() {value+=inc; if(value>max) value=max; if (!onPressCallback.empty()) onPressCallback();}
		void dec_value() {value-=inc; if(value<min) value=min; if (!onPressCallback.empty()) onPressCallback();}
        int value, min, max, inc;
		TexturedButton* btmore;
		TexturedButton* btless;
		Label label;
	};

	class FloatIncDec : public Container
	{
	public:
	    FloatIncDec(const s_font* _font = NULL, const s_texture* tex_up = NULL,
			const s_texture* tex_down = NULL, float min = 0.f, float max = 1.0f,
			float init_value = 0.5f, float inc = 0.1f);
	    virtual ~FloatIncDec();
		virtual void draw();
        virtual const float getValue() const {return value;}
		virtual void setValue(int v) {value=v; if(value>max) value=max; if(value<min) value=min;}
	protected:
		void inc_value() {value+=inc; if(value>max) value=max; if (!onPressCallback.empty()) onPressCallback();}
		void dec_value() {value-=inc; if(value<min) value=min; if (!onPressCallback.empty()) onPressCallback();}
        float value, min, max, inc;
		TexturedButton* btmore;
		TexturedButton* btless;
		Label label;
	};

	class LabeledCheckBox : public Container
	{
	public:
		LabeledCheckBox(int state = 0, const string& label = "");
		virtual int getState(void) const {return checkbx->getState();}
		virtual void setState(int s) {checkbx->setState(s);}
        virtual void setOnPressCallback(const callback<void>& c) {checkbx->setOnPressCallback(c);}
    protected:
		CheckBox* checkbx;
		Label* lbl;
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

	class StdWin : public FramedContainer
	{
	public:
	    StdWin(const char * _title = NULL, s_texture* _header_tex = NULL,
			s_font * _winfont = NULL, int headerSize = 18);
	   	virtual void draw();
	    virtual const char * getTitle() const {return titleLabel->getLabel().c_str();}
	    virtual void setTitle(const char * _title);
		virtual int onClic(int, int, S_GUI_VALUE, S_GUI_VALUE);
		virtual int onMove(int, int);
	protected:
	    Label* titleLabel;
		s_texture* header_tex;
		int dragging;
		s_vec2i oldpos;
	};

	class StdBtWin : public StdWin
	{
	public:
	    StdBtWin(const char * _title = NULL, s_texture* _header_tex = NULL,
			s_font * _winfont = NULL, int headerSize = 18);
		virtual void draw();
		virtual void setOnHideBtCallback(const callback<void>& c) {onHideBtCallback = c;}
	protected:
		void onHideBt(void);
		Button * hideBt;
		callback<void> onHideBtCallback;
	};

	class TabHeader : public LabeledButton
	{
	public:
		TabHeader(Component*, const string& _label = "", const s_font* _font = NULL);
		void draw(void);
		void setActive(int);
	protected:
		Component* assoc;
	};

	class TabContainer : public Container
	{
	public:
		TabContainer(const s_font* _font = NULL);
		void addTab(Component* c, const string& name);
		virtual void draw(void);
		virtual int onClic(int, int, S_GUI_VALUE, S_GUI_VALUE);
	protected:
		int getHeadersSize(void);
		void select(TabHeader*);
		list<TabHeader*> headers;
		int headerHeight;
	};


    class CursorBar : public Component
	{
	public:
	    CursorBar(float _min, float _max, float _val = 0);
	    virtual void draw(void);
        virtual float getValue(void) {return barVal;}
		virtual void setValue(float _barVal);
		virtual void setOnChangeCallback(const callback<void>& c) {onChangeCallback = c;}
		virtual int onClic(int, int, S_GUI_VALUE, S_GUI_VALUE);
		virtual int onMove(int, int);
	private:
		int dragging;
		Button cursor;
	    float minBar, maxBar, barVal;
		callback<void> onChangeCallback;
		s_vec2i oldPos;
	};

/*
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
