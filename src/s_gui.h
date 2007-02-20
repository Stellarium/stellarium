/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#include "SDL_opengl.h"

// SDL is used only for the key codes, i'm lasy to redefine them
#include "SDL.h"

#include <list>
#include <string>
#include <sstream>
#include <vector>
#include <map>

#include "STextureTypes.hpp"
#include "SFont.hpp"
#include "vecmath.h"
#include "callbacks.hpp"

#include "Translator.hpp"

void glCircleGui(const Vec3d& pos, float radius, float linewidth = 1.0);
void glEllipse(const Vec3d& pos, float radius, float y_ratio, float linewidth = 1.0);

using namespace std;
using namespace boost;

// cursor blink
Uint32 toggleBlink(Uint32 interval);
void* getLastCallbackObject(void);

////////////////////////////////// s_gui ///////////////////////////////////////
//
// Scissor
//
// Painter (has s_texture, s_font, s_color(base), s_color(text)
//
// Component
// |- Label
// |- CursorBar
// |- CallbackComponent
//    |- StringList
//    |- Picture
//    |  |- PictureMap
//    |- Button
//    |  |- TexturedButton
//    |  |- FilledButton
//    |  |- Checkbox
//    |  |  |- FlagButton
//    |  |- LabeledButton
//    |     |- TabHeader
//    |  |- EditBox
//    |- Container
//       |- FramedContainer
//       |- FilledContainer
//       |- TabContainer
//       |- StdWin
//       |  |-StdDlgWin
//       |  |-StdBtWin
//       |    |-StdTransBtWin
//       |- TabContainer
//       |- TextLabel
//       |- IntIncDec
//       |  |- IntIncDecVert
//       |- FloatIncDec
//       |- Time_item
//       |- LabeledCheckBox (CheckBox, Label)
//       |- Time_zone_item
//
////////////////////////////////////////////////////////////////////////////////

namespace s_gui
{
	typedef Vec4f s_color;
	typedef Vec3f s_vec3;
	typedef Vec4i s_square;
	typedef Vec2i s_vec2i;
	typedef Vec4i s_vec4i;

	static const s_color s_white = s_color(s_vec3(1,1,1));

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
		~Painter();
		Painter(const STextureSP _tex1, SFont* _font, const s_color& _baseColor, const s_color& _textColor);
		void drawSquareEdge(const s_vec2i& pos, const s_vec2i& sz) const;
		void drawSquareEdge(const s_vec2i& pos, const s_vec2i& sz, const s_color& c) const;
		void drawSquareFill(const s_vec2i& pos, const s_vec2i& sz) const;
		void drawSquareFill(const s_vec2i& pos, const s_vec2i& sz, const s_color& c) const;
		void drawSquareFill(const s_vec2i& pos, const s_vec2i& sz, const s_color& c, const STextureSP  t) const;
		void drawCross(const s_vec2i& pos, const s_vec2i& sz) const;
		void drawLine(const s_vec2i& pos1, const s_vec2i& pos2) const;
		void drawLine(const s_vec2i& pos1, const s_vec2i& pos2, const s_color& c) const;
		void print(int x, int y, const string& str);
		void print(int x, int y, const string& str, const s_color& c);
		void print(int x, int y, const wstring& str);
		void print(int x, int y, const wstring& str, const s_color& c);		
		void setTexture(const STextureSP tex) {tex1 = tex;}
		void setFont(SFont* f) {font = f;}
		void setTextColor(const s_color& c) {textColor = c;}
		void setBaseColor(const s_color& c) {baseColor = c;}
		const s_color& getBaseColor(void) const {return baseColor;}
		const s_color& getTextColor(void) const {return textColor;}
		SFont* getFont(void) const {return font;}
		void setOpaque(bool _b) { opaque = _b; }
    private:
		STextureSP tex1;
		SFont* font;
		s_color baseColor;
		s_color textColor;
		bool opaque;
    };

#define CT_COMPONENTBASE		0x0000
#define CT_LABEL 				0x1000
#define CT_CURSORBAR 			0x2000
#define CT_CALLBACK 			0x3000
	#define CT_STRINGLIST 		0x0010
	#define CT_PICTURE 			0x0020
	#define CT_BUTTON 			0x0040
		#define CT_TABHEADER 	0x0001
	#define CT_EDITBOX 			0x0080
	#define CT_CONTAINER 		0x0100

    class Component
    {
    public:
        Component();
		virtual ~Component();
        virtual void draw(void);
        virtual void reshape(const s_vec2i& _pos, const s_vec2i& _size);
        virtual void reshape(int x, int y, int w, int h);
        virtual int getPosx() const {return pos[0];}
		virtual int getPosy() const {return pos[1];}
        virtual int getSizex() const {return size[0];}
		virtual int getSizey() const {return size[1];}
        virtual void setSizex(int v) {size[0] = v;}
		virtual void setSizey(int v) {size[1] = v;}
        virtual void setPosx(int v) {pos[0] = v;}
		virtual void setPosy(int v) {pos[1] = v;}
        virtual const s_vec2i getPos() const {return pos;}
        virtual const s_vec2i getSize() const {return size;}
        virtual void setPos(const s_vec2i& _pos) {pos = _pos;}
        virtual void setSize(const s_vec2i& _size) {size = _size;}
        virtual void setPos(int x, int y) {pos[0] = x; pos[1]=y;}
        virtual void setSize(int w, int h) {size[0] = w; size[1]=h;}
        virtual void setVisible(bool _visible);
        virtual int getVisible(void) const {return visible;}
        virtual void setFocus(bool _focus) { focus = _focus; };
        virtual bool getFocus(void) const {return focus;}
		virtual bool onClic(int, int, Uint8, Uint8) {return 0;}
		virtual bool onMove(int, int) {return 0;}
		virtual bool onKey(Uint16, Uint8) {return 0;}
		virtual void setTexture(const STextureSP tex) {painter.setTexture(tex);}
		virtual void setFont(SFont* f) {painter.setFont(f);}
		SFont* getFont(void) const {return painter.getFont();}
		virtual void setTextColor(const s_color& c) {painter.setTextColor(c);}
		virtual void setBaseColor(const s_color& c) {painter.setBaseColor(c);}
		virtual void setPainter(const Painter& p) {painter = p;}
		virtual bool isIn(int x, int y);
		static void setDefaultPainter(const Painter& p) {defaultPainter=p; }
		static void initScissor(int winW, int winH);
		static void deleteScissor(void);
		static void enableScissor(void) {scissor->activate();}
		static void disableScissor(void) {scissor->desactivate();}
		bool inFront(void) { return moveToFront; }
		bool getNeedNewEdit(void) { return needNewTopEdit; }
		void setNeedNewEdit(bool _b) { needNewTopEdit = _b; }
		void setInFront(bool b) { moveToFront = b; } // signals this component to move to front
		void setOpaque(bool b) { painter.setOpaque(b); }
		unsigned int getType(void) { return type; }
		virtual void setColorScheme(const s_color& baseColor, const s_color& textColor);
   		virtual void setGUIColorSchemeMember(bool _b) {GUIColorSchemeMember = _b;}
    protected:
        s_vec2i pos;
        s_vec2i size;
        bool visible;
        bool focus;
        static bool focusing;
		Painter painter;
		
		static s_color guiBaseColor;
		static s_color guiTextColor;
		bool GUIColorSchemeMember;
		static Painter defaultPainter;
		static Scissor* scissor;
		bool moveToFront;
		bool needNewTopEdit;
		unsigned int type;
		bool desktop;
    private:
    };

    class CallbackComponent : public Component
    {
    public:
		CallbackComponent();
		virtual void setOnMouseInOutCallback(const callback<void>& c) {onMouseInOutCallback = c;}
        virtual void setOnPressCallback(const callback<void>& c) {onPressCallback = c;}
		virtual bool getIsMouseOver(void) {return is_mouse_over;}
		virtual bool onMove(int, int);
		virtual bool onClic(int, int, Uint8, Uint8);
    protected:
		callback<void> onPressCallback;
		callback<void> onMouseInOutCallback;
		bool is_mouse_over;
		bool pressed;
	};

    class Container : public CallbackComponent
    {
		friend class Component;
    public:
        Container(bool desktop = false);
        virtual ~Container();
        virtual void addComponent(Component*);
        virtual void removeComponent(Component*);
		virtual void removeAllComponents(void);
        virtual void draw(void);
		virtual bool onClic(int, int, Uint8, Uint8);
		virtual bool onMove(int, int);
		virtual bool onKey(Uint16, Uint8);
        virtual void setFocus(bool _focus);
   		void setGUIColorSchemeMember(bool _b);
		void setColorScheme(const s_color& _baseColor, const s_color& _textColor);
    protected:
        std::list<Component*> childs;
    };

////////////////////////////////////////////////////////////////////////////////
// FilledContainer
////////////////////////////////////////////////////////////////////////////////

    class FilledContainer : public Container
    {
    public:
        virtual void draw(void);
    };

////////////////////////////////////////////////////////////////////////////////
// Button
////////////////////////////////////////////////////////////////////////////////

    class Button : public CallbackComponent
    {
    public:
		Button();
        virtual void draw();
		virtual bool onClic(int, int, Uint8, Uint8);
		void setHideBorder(bool _b) { hideBorder = _b;}
		void setHideBorderMouseOver(bool _b) { hideBorderMouseOver = _b;}
		void setHideTexture(bool _b) { hideTexture = _b;}
    protected:
		bool hideBorder;
		bool hideBorderMouseOver;
		bool hideTexture;
    };

////////////////////////////////////////////////////////////////////////////////
// TexturedButton
////////////////////////////////////////////////////////////////////////////////

    class TexturedButton : public Button
    {
    public:
		TexturedButton(const STextureSP tex = STextureSP());
        virtual void draw();
    protected:
    };

    class FilledButton : public Button
    {
    public:
        virtual void draw();
    protected:
    };

////////////////////////////////////////////////////////////////////////////////
// CheckBox
////////////////////////////////////////////////////////////////////////////////

	class CheckBox : public Button
    {
    public:
		CheckBox(bool state = false);
        virtual void draw();
		virtual int getState(void) const {return isChecked;}
		virtual void setState(bool s) {isChecked = s;}
		virtual bool onClic(int, int, Uint8, Uint8);
    protected:
		bool isChecked;
    };

	class FlagButton : public CheckBox
    {
    public:
		FlagButton(int state = 0, const STextureSP tex=STextureSP(), STextureSP specificTex=STextureSP());
        virtual ~FlagButton();
		virtual void draw();
    protected:
		STextureSP specific_tex;
    };

////////////////////////////////////////////////////////////////////////////////
// Label
////////////////////////////////////////////////////////////////////////////////

    class Label : public Component
    {
    public:
        Label(const wstring& _label = L"", SFont* _font = NULL);
        virtual ~Label();
        virtual const wstring& getLabel() const {return label;}
        /** @brief Set new label */
        virtual void setLabel(const wstring&);
        virtual void draw();
        virtual void draw(float intensity);
		virtual void adjustSize(void);
    protected:
        wstring label;
    };

////////////////////////////////////////////////////////////////////////////////
// LabeledButton
////////////////////////////////////////////////////////////////////////////////

	enum Justification { JUSTIFY_LEFT, JUSTIFY_CENTER, JUSTIFY_RIGHT };

    class LabeledButton : public Button
    {
		friend class TabContainer;
    public:
        LabeledButton(const wstring& _label = L"", SFont* font = NULL, 
			Justification _j = JUSTIFY_CENTER, bool _bright = false);
		virtual ~LabeledButton();
        virtual void draw(void);
		virtual void setColorScheme(const s_color& baseColor, const s_color& textColor);
		virtual void setVisible(bool _visible);
		virtual void setFont(SFont* f) {Button::setFont(f); label.setFont(f);}
		virtual void setTextColor(const s_color& c) {Button::setTextColor(c); label.setTextColor(c);}
		virtual void setPainter(const Painter& p) {Button::setPainter(p); label.setPainter(p);}
		virtual void setJustification(Justification _j) { justification = _j; }
		virtual void adjustSize(void);
        void setLabel(const wstring& _label) { label.setLabel(_label); };
        const wstring& getLabel(void) {return label.getLabel();}
        void setBright(bool _b) { isBright = _b; };
    protected:
    	void justify(void);
		Label label;
		Justification justification;
		bool isBright;
    };
    
////////////////////////////////////////////////////////////////////////////////
// History
////////////////////////////////////////////////////////////////////////////////

    class History
    {
    public:
		History(unsigned int _items = 0);
		void clear(void);
		void add(const wstring& _text);
		wstring prev(void);
		wstring next(void);
	private:
		unsigned int maxItems;
		int pos;
		vector<wstring> history;
	};

////////////////////////////////////////////////////////////////////////////////
// AutoCompleteString
////////////////////////////////////////////////////////////////////////////////

    class AutoCompleteString
    {
    public:
		AutoCompleteString();
		void setOptions(vector<wstring> _options) { options = _options; }
		wstring getOptions(void);
		wstring getFirstOption(void);
		void reset(void);
		bool hasOption(void) { return options.size() > 0; }
	private:
		int lastMatchPos;
		vector<wstring> options;
	};    

////////////////////////////////////////////////////////////////////////////////
// Editbox
////////////////////////////////////////////////////////////////////////////////

    class EditBox : public Button
    {
    public:
        EditBox(const wstring& _label = L"", SFont* font = NULL);
		virtual ~EditBox();
		virtual void setOnReturnKeyCallback(const callback<void>& c) {onReturnKeyCallback = c;}
		virtual void setOnKeyCallback(const callback<void>& c) {onKeyCallback = c;}
		virtual void setOnAutoCompleteCallback(const callback<void>& c) {onAutoCompleteCallback = c;}
        virtual void draw(void);
		virtual void setColorScheme(const s_color& baseColor, const s_color& textColor);
		virtual void setFont(SFont* f) {Button::setFont(f); label.setFont(f);}
		virtual void setTextColor(const s_color& c) {Button::setTextColor(c); label.setTextColor(c);}
		virtual void setPainter(const Painter& p) {Button::setPainter(p); label.setPainter(p);}
		virtual bool onKey(Uint16, Uint8);
		virtual bool onClic(int, int, Uint8, Uint8);
		void setVisible(bool _visible);
		void setAutoCompleteOptions(vector<wstring> _autocomplete) { autoComplete.setOptions(_autocomplete); };
		wstring getAutoCompleteOptions(void) { return autoComplete.getOptions(); }
		wstring getText(void) { return text; }
		void setEditing(bool _b);
		void refreshLabel(void);
		void setPrompt(const wstring &p);
		void setPrompt(void) { setPrompt(wstring()); };
		void setText(const wstring &_text);
		void clearText(void) { setText(L""); }
		wstring getDefaultPrompt(void);
		Uint16 getLastKey(void) { return lastKey;}
		void setAutoFocus(bool _b) {autoFocus = _b; }
		bool getAutoFocus(void) { return autoFocus; }

		static EditBox *activeEditBox;
	protected:
		callback<void> onReturnKeyCallback;
		callback<void> onKeyCallback;
		callback<void> onAutoCompleteCallback;
		AutoCompleteString autoComplete;
		History history;

        void cursorToNextWord(void);
        void cursorToPrevWord(void);

        Label label;
		bool isEditing;
		wstring text;
		unsigned int cursorPos;
		wstring lastText;
		wstring prompt;
		Uint16 lastKey;
		bool autoFocus;
		bool cursorVisible;
		bool blinkTimerValid;
		SDL_TimerID blinkTimer;
    };
	
	class ScrollBar : public CallbackComponent
	{
	public:
		ScrollBar(bool _vertical = true, int _totalElements = 1, int _elementsForBar = 1);
		virtual void setOnChangeCallback(const callback<void>& c) {onChangeCallback = c;}
		virtual void draw(void);
		virtual bool onClic(int, int, Uint8, Uint8);
		virtual bool onMove(int, int);
		void setTotalElements(int _elements);
		void setElementsForBar(int _elementsForBar);
		void setValue(int _value);
		int getValue(void) { return value; }
		
	private:
		callback<void> onChangeCallback;
		void adjustSize(void);
		Button scrollBt;
		bool vertical;
		unsigned int scrollOffset, scrollSize;
		int elements, elementsForBar;
		bool dragging;
		int value;
		int firstElement;
		bool sized;
		s_vec2i oldPos;
		int oldValue;
	};
	

    class ListBox : public Component
    {
    public:
		ListBox(int _displayLines = 5);
		~ListBox();
		virtual void setOnChangeCallback(const callback<void>& c) {onChangeCallback = c;}
		virtual bool onClic(int, int, Uint8, Uint8);
		virtual bool onMove(int, int);
        virtual void draw(void);
   		virtual void setVisible(bool _visible);
		wstring getItem(int value);
		wstring getCurrent(void) {return getItem(getValue());}
		void addItems(const vector<wstring> _items);
		void addItemList(const wstring& _text);
		void addItem(const wstring& _text);
		void clear(void);
		int getValue(void) { return value;}
		void setCurrent(const wstring& ws);
	private:
		callback<void> onChangeCallback;
		ScrollBar scrollBar;
		void createLines(void);
		void adjustAfterItemsAdded(void);
		void scrollChanged(void);
		callback<void> onChangedCallback;
		int firstItemIndex;
		vector<LabeledButton*> itemBt;
		vector<wstring> items;
		int value;
		unsigned int displayLines;
	};

    class TextLabel : public Container
	{
	public:
	    TextLabel(const wstring& _label = L"", SFont* _font = NULL);
	    virtual ~TextLabel();
        virtual const wstring& getLabel() const {return label;}
        virtual void draw(void);
        virtual void setLabel(const wstring&);
		virtual void adjustSize(void);
		virtual void setTextColor(const s_color& c);
	protected:
        wstring label;
	};

	class IntIncDec : public Container
	{
	public:
//	    IntIncDec(SFont* _font = NULL, const STextureSP tex_up = NULL,
//			const STextureSP tex_down = NULL, int min = 0, int max = 9,
//			int init_value = 0, int inc = 1, bool _loop = false);
	    IntIncDec(SFont* _font,
			const STextureSP tex_up,
			const STextureSP tex_down,
			int min = 0, int max = 9,
			int init_value = 0, int inc = 1, bool _loop = false);
	    virtual ~IntIncDec();
		virtual void draw();
        virtual const float getValue() const {return value;}
		virtual void setValue(int v) {value=v; if(value>max) value=max; if(value<min) value=min;}
	protected:
		void inc_value();
		void dec_value();
        int value, min, max, inc;
		TexturedButton* btmore;
		TexturedButton* btless;
		Label * label;
		bool loop;
	};

	class IntIncDecVert : public IntIncDec
	{
	public:
	    IntIncDecVert(SFont* _font = NULL, const STextureSP tex_up=STextureSP(),
			const STextureSP tex_down=STextureSP(), int min = 0, int max = 9,
			int init_value = 0, int inc = 1, bool _loop = false);
	};

	enum Format { FORMAT_DEFAULT, FORMAT_LATITUDE, FORMAT_LONGITUDE };
	class FloatIncDec : public Container
	{
	public:
	    FloatIncDec(SFont* _font = NULL, const STextureSP tex_up=STextureSP(),
			const STextureSP tex_down=STextureSP(), float min = 0.f, float max = 1.0f,
			float init_value = 0.5f, float inc = 0.1f);
	    virtual ~FloatIncDec();
		virtual void draw();
        virtual const float getValue() const {return value;}
		virtual void setValue(float v) {value=v; if(value>max) value=max; if(value<min) value=min;}
		void setFormat(Format _f) { format = _f; }
	protected:
		void inc_value();
		void dec_value();
        float value, min, max, inc;
		TexturedButton* btmore;
		TexturedButton* btless;
		Label * label;
		Format format;
	};

	// Widget used to set time and date.
    class Time_item : public Container
    {
    public:
		Time_item(SFont* _font = NULL, const STextureSP tex_up=STextureSP(),
			const STextureSP tex_down=STextureSP(), double _JD = 2451545.0);
		double getJDay(void) const;
		string getDateString(void);
		void setJDay(double jd);
		virtual void draw();
		void onTimeChange(void);
		virtual void setOnChangeTimeCallback(callback<void> c) {onChangeTimeCallback = c;}
    protected:
		IntIncDec* d, *m, *y, *h, *mn, *s;
		callback<void> onChangeTimeCallback;
    };

	class LabeledCheckBox : public Container
	{
	public:
		LabeledCheckBox(bool state = false, const wstring& _label = L"");
		~LabeledCheckBox();
        virtual void draw(void);
		virtual int getState(void) const {return checkbox->getState();}
		virtual void setState(bool s) {checkbox->setState(s);}
        virtual void setOnPressCallback(const callback<void>& c) {checkbox->setOnPressCallback(c);}
    protected:
		CheckBox* checkbox;
		Label* label;
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
	    StdWin(const wstring& _title = L"", const STextureSP _header_tex = STextureSP(),
			SFont * _winfont = NULL, int headerSize = 18);
	   	virtual void draw();
	    virtual wstring getTitle() const {return titleLabel->getLabel();}
	    virtual void setTitle(const wstring& _title);
		virtual bool onClic(int, int, Uint8, Uint8);
		virtual bool onMove(int, int);
		virtual void setVisible(bool _visible);
	protected:
	    Label* titleLabel;
		STextureSP header_tex;
		int dragging;
		s_vec2i oldPos;
	};

	class Picture;
	
#define BT_NOTSET 			0
#define BT_YES 				1
#define BT_NO 				2
#define BT_CANCEL 			4
#define BT_OK 				8
#define BT_ICON_BLANK 		256
#define BT_ICON_QUESTION 	512
#define BT_ICON_ALERT 		1024

#define STDDLGWIN_MSG		0
#define STDDLGWIN_INPUT		1

	class StdDlgWin : public StdWin
	{
	public:
		StdDlgWin(const wstring& _title,  STextureSP blankIconTex, STextureSP questionIconTex, STextureSP alertIconTex,
                  const STextureSP _header_tex = STextureSP(),
                  SFont * _winfont = NULL, int headerSize = 18);
		~StdDlgWin();
		virtual void setDialogCallback(const callback<void>& c) {onCompleteCallback = c;}
		void MessageBox(const wstring &_title, const wstring &_prompt, int _buttons, const string &_ID = "");
		void InputBox(const wstring &_title, const wstring &_prompt, const string &_ID = "");
		virtual void setOnCompleteCallback(const callback<void>& c) {onCompleteCallback = c;}
		string getLastID(void) { return lastID;}
		int getLastType(void) { return lastType;}
		int getLastButton(void) { return lastButton;}
		wstring getLastInput(void) { return lastInput;}
	private:
		callback<void> onCompleteCallback;
		void resetResponse(void);
		void arrangeButtons(void);
		void onInputReturnKey(void);
		void onFirstBt(void);
		void onSecondBt(void);
		LabeledButton *firstBt, *secondBt;
		TextLabel *messageLabel;
		EditBox *inputEdit;
		STextureSP blankIcon;
		STextureSP questionIcon;
		STextureSP alertIcon;
		Picture *picture;
		wstring originalTitle;
		bool hasIcon;
		int numBtns;
		int firstBtType, secondBtType;
		string lastID;
		int lastType;	
		int lastButton;
		wstring lastInput;
	};
	
	class StdBtWin : public StdWin
	{
	public:
	    StdBtWin(const wstring& _title = NULL,
                 const STextureSP _header_tex = STextureSP(),
			SFont * _winfont = NULL, int headerSize = 18);
		virtual void draw();
		virtual void setOnHideBtCallback(const callback<void>& c) {onHideBtCallback = c;}
	protected:
		void onHideBt(void);
		Button * hideBt;
		callback<void> onHideBtCallback;
	};

	// stdbtwin with transient (timeout) functionality
	class StdTransBtWin : public StdBtWin
	{
	public:
	    StdTransBtWin(const wstring& _title = L"", int _time_out =0,
                      const STextureSP _header_tex = STextureSP(),
			SFont * _winfont = NULL, int headerSize = 18);
		virtual void update(int _delta_time);
		virtual void set_timeout(int _time_out=0);
	protected:
		int time_left;
		bool timer_on;
	};

	class TabHeader : public LabeledButton
	{
	public:
		TabHeader(Component*, const wstring& _label = L"", SFont* _font = NULL);
		void draw(void);
		void setActive(bool);
	protected:
		Component* assoc;
		bool active;
	};

	class TabContainer : public Container
	{
	public:
		TabContainer(SFont* _font = NULL);
		void addTab(Component* c, const wstring& name);
		virtual void draw(void);
		virtual bool onClic(int, int, Uint8, Uint8);
		virtual void setColorScheme(const s_color& baseColor, const s_color& textColor);
	protected:
		int getHeadersSize(void);
		void select(TabHeader*);
		list<TabHeader*> headers;
		int headerHeight;
	};


    class CursorBar : public CallbackComponent
	{
	public:
	    CursorBar(bool vertical = false, float _min = 0, float _max = 0, float _val = 0);
	    virtual void draw(void);
        virtual float getValue(void) {return value;}
		virtual void setValue(float _value);
		virtual void setOnChangeCallback(const callback<void>& c) {onChangeCallback = c;}
		virtual bool onClic(int, int, Uint8, Uint8);
		virtual bool onMove(int, int);
	private:
		void adjustSize(void);
		bool dragging;
		Button cursorBt;
	    float minValue, maxValue, range;
		float value;
		callback<void> onChangeCallback;
		bool vertical;
		s_vec2i oldPos;
		float oldValue;
		bool sized;
	};


    class Picture : public CallbackComponent
	{
	public:
	    Picture(const STextureSP  _imageTex,
                int xpos = 0, int ypos = 0, int xsize = 32, int ysize = 32);
		~Picture();
		virtual void draw(void);
		void setShowEdge(bool v) {showedges = v;}
		void setImgColor(const s_color &c) {imgcolor = c;}
	private:
	    const STextureSP  imageTex;
		bool showedges;
		s_color imgcolor;
	};

	class City 
	{
	public:
		City(const string& _name = "", const string& _state = "", const string& _country = "", 
			double _longitude = 0.f, double _latitude = 0.f, float zone = 0, int _showatzoom = 0, int _altitude = 0);
		void addCity(const string& _name = "", const string& _state = "", const string& _country = "", 
			double _longitude = 0.f, double _latitude = 0.f, float zone = 0, int _showatzoom = 0, int _altitude = 0);
		string getName(void) { return name; }
		wstring getNameI18(void) { return _(name); }
		string getState(void) { return state; }
		wstring getStateI18(void) { return _(state.c_str()); }
		string getCountry(void) { return country; }
		wstring getCountryI18(void) { return _(country.c_str()); }
		double getLatitude(void) { return latitude; }
		double getLongitude(void) { return longitude; }
		int getShowAtZoom(void) { return showatzoom; }
		int getAltitude(void) { return altitude; }
	private:
		string name;
		string state;
		string country;
		double latitude;
		double longitude;
		float zone;
		int showatzoom;
		int altitude;
	};
	
	#define CITIES_PROXIMITY 10
	class City_Mgr 
	{
	public:
		City_Mgr(double _proximity = CITIES_PROXIMITY);
		~City_Mgr();
		void addCity(const string& _name, const string& _state, const string& _country, 
			double _longitude, double _latitude, float _zone, int _showatzoom, int _altitude = 0);
		int getNearest(double _longitude, double _latitude);
		void setProximity(double _proximity);
		City *getCity(unsigned int _index);
		unsigned int size(void) { return cities.size(); }
	private:
		vector<City*> cities;
		double proximity;
	};

	#define UNKNOWN_OBSERVATORY "Unknown observatory"
	#define ZOOM_LIMIT 100.f
    class MapPicture : public Picture
	{
	public:
	    MapPicture(const STextureSP _imageTex,
                   const STextureSP _pointerTex,
                   const STextureSP _cityTex,
                   int xpos = 0, int ypos = 0, int xsize = 32, int ysize = 32);
		~MapPicture();
		virtual void setOnNearestCityCallback(const callback<void>& c) {onNearestCityCallback = c;}
		virtual void draw(void);
		virtual bool onClic(int, int, Uint8, Uint8);
		virtual bool onMove(int, int);
		virtual bool isIn(int x, int y);
		bool onKey(Uint16 k, Uint8 s);
		void set_font(SFont* newFont) {city_name_font = newFont;}
		double getPointerLongitude(void);
		double getPointerLatitude(void);
		int getPointerAltitude(void);
		void setPointerLongitude(double _longitude) { pointerPos[0] = getxFromLongitude(_longitude);};
		void setPointerLatitude(double _latitude) { pointerPos[1] = getyFromLatitude(_latitude);};

		void zoomInOut(float _amount);
		
		void addCity(const string& _name, const string& _state, const string& _country, 
			double _longitude, double _latitude, float _zone, int _showatzoom, int _altitude) { cities.addCity(_name, _state, _country, _longitude, _latitude, _zone, _showatzoom, _altitude);}
		string getPositionString(void);
		string getCursorString(void);
		void findPosition(double longitude, double latitude);
	private:
		callback<void> onNearestCityCallback;
		string getCity(int index) { if (index != -1) return cities.getCity(index)->getName(); else return ""; }
		string getState(int index) { if (index != -1) return cities.getCity(index)->getState(); else return ""; }
		string getCountry(int index) { if (index != -1) return cities.getCity(index)->getCountry(); else return ""; }
		string getLocationString(int index);
		int getxFromLongitude(double _longitude);
		int getyFromLatitude(double _latitude);
		double getLongitudeFromx(int x);
		double getLatitudeFromy(int y);
		void drawCities(void);
		void drawCityName(const s_vec2i& cityPos, const wstring& _name);
		void drawCity(const s_vec2i& cityPos, int ctype);
		void drawNearestCity(void);
		void calcPointerPos(int x, int y);
		void setPointerSize(void);
		Picture *pointer;
		Picture *cityPointer;
		SFont* city_name_font;
		s_vec2i pointerPos, oldPos;
		s_vec2i originalSize, originalPos;
		s_vec2i cursorPos;
		bool panning, dragging;
		float zoom;
		bool sized;
		City_Mgr cities;
		int nearestIndex;
		int pointerIndex;
		bool exact;
		double exactLatitude, exactLongitude;
		int exactAltitude;
		float fontsize;
	};

    class StringList : public CallbackComponent
	{
	public:
		StringList();
		virtual void draw(void);
		virtual bool onClic(int, int, Uint8, Uint8);
		void addItem(const string &);
		void addItemList(const string& s)
		{
			istringstream is(s);
			string elem;
			while(getline(is, elem))
			{
				addItem(elem);
			}
		}
		const string getValue() const;
		bool setValue(const string &);
		void adjustSize(void) {setSizey(elemsSize);}
	private:
		int elemsSize;
		int itemSize;
		vector<string> items;
		vector<string>::iterator current;
	};

	// Widget used to set time zone. Initialized from a file of type /usr/share/zoneinfo/zone.tab
    class Time_zone_item : public Container
    {
    public:
		Time_zone_item(const string& zonetab_file);
		virtual void draw(void);
		string gettz(void); // should be const but gives a boring error...
		void settz(const string& tz);
		void onContinentClic(void);
		void onCityClic(void);
    protected:
		StringList continents_names;
		map<string, StringList > continents;
		StringList* current_edit;
		StringList* lb;
    };

};

#endif // _GUI_H_
