/*
* Stellarium
* Copyright (C) 2002 Fabien Chéreau
* Inspired by the gui.h by Chris Laurel <claurel@shatters.net>
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

//#include <stdio.h>

#include "s_gui.h"

using namespace std;
using namespace s_gui;

Scissor Component::scissor(640, 480);	// Default initialization
Painter Component::defaultPainter;

///////////////////////////////// Scissor //////////////////////////////////////
// Manages the use of the OpenGL Scissor test to prevent drawings outside the
// components borders. Use a stack like for openGL matrices.
////////////////////////////////////////////////////////////////////////////////

Scissor::Scissor(int _winW, int _winH) :
	winW(_winW),
	winH(_winH)
{
	s_square v(0, 0, winW, winH);
	stack.push_back(v);
}

// Define a new GlScissor zone relative to the previous one and apply it so
// that nothing can be drawn outside the limits
void Scissor::push(int posx, int posy, int sizex, int sizey)
{
	s_square v = *stack.end();
	s_square el(v[0] + posx, v[1] + posy, v[2], v[3]);

	// Check and adjust in case of overlapping
	if (posx + sizex > v[2])
	{
		el[2] = v[2] - posx;
		if (el[2] < 0) el[2] = 0;
	}
	if (posy + sizey > v[3])
	{
		el[3] = v[3] - posy;
		if (el[3] < 0) el[3] = 0;
	}

	// Apply the new values
	glScissor(el[0], winH - el[1] - el[3], el[2], el[3]);

	// Add the new value at the end of the stack
	stack.push_back(el);
}

// See above
void Scissor::push(const s_vec2i& pos, const s_vec2i& size)
{
	s_square v = *stack.end();
	s_square el(v[0] + pos[0], v[1] + pos[1], v[2], v[3]);

	// Check and adjust in case of overlapping
	if (pos[0] + size[0] > v[2])
	{
		el[2] = v[2] - pos[0];
		if (el[2] < 0) el[2] = 0;
	}
	if (pos[1] + size[1] > v[3])
	{
		el[3] = v[3] - pos[1];
		if (el[3] < 0) el[3] = 0;
	}

	// Apply the new values
	glScissor(el[0], winH - el[1] - el[3], el[2], el[3]);

	// Add the new value at the end of the stack
	stack.push_back(el);
}

// Remove the last element in the stack : ie comes back to the previous
// GlScissor borders.
void Scissor::pop(void)
{
	// Remove the last value from the stack
	stack.pop_back();

	// Apply the previous value
	s_square v = *stack.end();
	glScissor(v[0], winH - v[1] - v[3], v[2], v[3]);
}


//////////////////////////////// Painter ///////////////////////////////////////
// Class used to manage all the drawings for a component. Stores informations
// like colors or used textures. Performs the primitives drawing.
////////////////////////////////////////////////////////////////////////////////

Painter::Painter() :
	tex1(NULL),
	font(NULL)
{
}

Painter::Painter(s_texture* _tex1, s_font* _font, const s_color& _baseColor, const s_color& _textColor) :
	tex1(_tex1),
	font(_font),
	baseColor(_baseColor),
	textColor(_textColor)
{

}

// Draw the edges of the defined square with the default base color
void Painter::drawSquareEdge(const s_vec2i& pos, const s_vec2i& sz) const
{
	glColor4fv(baseColor);
    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINE_LOOP);
        glVertex2f(pos[0] + 0.5		   , pos[1] + 0.5        );
        glVertex2f(pos[0] + sz[0] - 0.5, pos[1] + 0.5        );
        glVertex2f(pos[0] + sz[0] - 0.5, pos[1] + sz[1] - 0.5);
        glVertex2f(pos[0] + 0.5		   , pos[1] + sz[1] - 0.5);
    glEnd();
}

// Draw the edges of the defined square with the given color
void Painter::drawSquareEdge(const s_vec2i& pos, const s_vec2i& sz, const s_color& c) const
{
    glColor4fv(c);
    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINE_LOOP);
        glVertex2f(pos[0] + 0.5		   , pos[1] + 0.5        );
        glVertex2f(pos[0] + sz[0] - 0.5, pos[1] + 0.5        );
        glVertex2f(pos[0] + sz[0] - 0.5, pos[1] + sz[1] - 0.5);
        glVertex2f(pos[0] + 0.5		   , pos[1] + sz[1] - 0.5);
    glEnd();
}

// Fill the defined square with the default texture and default base color
void Painter::drawSquareFill(const s_vec2i& pos, const s_vec2i& sz) const
{
    glColor4fv(baseColor);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, tex1->getID());
    glBegin(GL_QUADS );
        glTexCoord2s(0, 0); glVertex2i(pos[0]        , pos[1] + sz[1]);	// Bottom Left
        glTexCoord2s(1, 0); glVertex2i(pos[0] + sz[0], pos[1] + sz[1]);	// Bottom Right
        glTexCoord2s(1, 1); glVertex2i(pos[0] + sz[0], pos[1]        );	// Top Right
        glTexCoord2s(0, 1); glVertex2i(pos[0]        , pos[1]        );	// Top Left
    glEnd ();
}

// Fill the defined square with the default texture and given color
void Painter::drawSquareFill(const s_vec2i& pos, const s_vec2i& sz, const s_color& c) const
{
    glColor4fv(c);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, tex1->getID());
    glBegin(GL_QUADS );
        glTexCoord2s(0, 0); glVertex2i(pos[0]        , pos[1] + sz[1]);	// Bottom Left
        glTexCoord2s(1, 0); glVertex2i(pos[0] + sz[0], pos[1] + sz[1]);	// Bottom Right
        glTexCoord2s(1, 1); glVertex2i(pos[0] + sz[0], pos[1]        );	// Top Right
        glTexCoord2s(0, 1); glVertex2i(pos[0]        , pos[1]        );	// Top Left
    glEnd ();
}

// Fill the defined square with the given texture and given color
void Painter::drawSquareFill(const s_vec2i& pos, const s_vec2i& sz, const s_color& c, const s_texture * t) const
{
    glColor4fv(c);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, t->getID());
    glBegin(GL_QUADS );
        glTexCoord2s(0, 0); glVertex2i(pos[0]        , pos[1] + sz[1]);	// Bottom Left
        glTexCoord2s(1, 0); glVertex2i(pos[0] + sz[0], pos[1] + sz[1]);	// Bottom Right
        glTexCoord2s(1, 1); glVertex2i(pos[0] + sz[0], pos[1]        );	// Top Right
        glTexCoord2s(0, 1); glVertex2i(pos[0]        , pos[1]        );	// Top Left
    glEnd ();
}

// Print the text with the default font and default text color
void Painter::print(int x, int y, const char * str) const
{
    glColor4fv(textColor);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
	font->print(x, y, str);
}


//////////////////////////////// Component /////////////////////////////////////
// Mother class for every s_gui object.
////////////////////////////////////////////////////////////////////////////////

Component::Component(void) :
	pos(0, 0),
	size(0, 0),
	visible(1),
	active(1),
	focus(0),
	painter(defaultPainter)
{
}

Component::~Component()
{
}

void Component::reshape(s_vec2i _pos, s_vec2i _size)
{
	pos = _pos;
	size = _size;
}

void Component::reshape(int x, int y, int w, int h)
{
	pos.set(x, y);
	size.set(w, h);
}

int Component::isIn(float x , float y)
{
	return (pos[0]<=x && (size[0]+pos[0])>=x && pos[1]<=y && (pos[1]+size[1])>=y);
}


//////////////////////////////// Container /////////////////////////////////////
// Manages hierarchical components : send signals ad actions  to childrens
////////////////////////////////////////////////////////////////////////////////

Container::Container() : Component()
{
}

Container::~Container()
{
    list<Component*>::iterator iter = childs.begin();
    while (iter != childs.end())
    {
        if (*iter)
        {
            delete (*iter);
            (*iter)=NULL;
        }
        iter++;
    }
}

void Container::addComponent(Component* c)
{
	childs.push_front(c);
}

void Container::draw(void)
{
    if (!visible) return;
    list<Component*>::iterator iter = childs.begin();
    glPushMatrix();
    glTranslatef(pos[0], pos[1], 0.f);

    Component::scissor.push(pos, size);

	while (iter != childs.end())
	{
		(*iter)->draw();
		iter++;
	}

    Component::scissor.pop();
    glPopMatrix();
}

int Container::onClic(int x, int y, S_GUI_VALUE button, S_GUI_VALUE state)
{
	list<Component*>::iterator iter = childs.begin();
	while (iter != childs.end())
	{
		if ((*iter)->onClic(x - pos[0], y - pos[1], button, state)) return 1;	// The signal has been intercepted
        iter++;
    }
    return 0;
}

int Container::onMove(int x, int y)
{
	list<Component*>::iterator iter = childs.begin();
	while (iter != childs.end())
	{
		if ((*iter)->onMove(x - pos[0], y - pos[1])) return 1;	// The signal has been intercepted
        iter++;
    }
    return 0;
}

int Container::onKey(SDLKey k, S_GUI_VALUE s)
{
	list<Component*>::iterator iter = childs.begin();
	while (iter != childs.end())
	{
		if ((*iter)->onKey(k, s)) return 1;	// The signal has been intercepted
        iter++;
    }
    return 0;
}

//////////////////////////// FilledContainer ///////////////////////////////////
// Container filled with a texture and with an edge
////////////////////////////////////////////////////////////////////////////////


void FilledContainer::draw(void)
{
	painter.drawSquareFill(pos, size);
	painter.drawSquareEdge(pos, size);
	Container::draw();
}

/*
// Button

void ExtButtonClicCallback(int x, int y, enum guiValue state, enum guiValue button, Component * me)
{
    ((Button*)me)->ButtonClicCallback(button, state);
}

void ExtButtonMoveCallback(int x, int y, enum guiValue action, Component * me)
{
    ((Button*)me)->ButtonMoveCallback(action);
}

Button::Button() :
    Component(), 
    mouseOn(false), 
    onClicCallback(NULL), 
    onMouseOverCallback(NULL)
{
    setClicCallback(ExtButtonClicCallback);
    setMoveCallback(ExtButtonMoveCallback);
}

Button::~Button()
{
    //printf("Destructor Button.\n");
}

void Button::render(GraphicsContext& gc)
{
    if (mouseOn) glColor3fv(gc.baseColor / 2);
    else glColor3fv(gc.baseColor*1.5);
    if (mouseIn) glColor3fv(gc.baseColor*2);
    glDisable(GL_TEXTURE_2D);
    vec2_i pos = getPosition();
    vec2_i sz = getSize();
    glBegin(GL_LINE_LOOP);
        glVertex2f(pos[0]+0.5, pos[1]+0.5);
        glVertex2f(pos[0] + sz[0]-0.5, pos[1]+0.5);
        glVertex2f(pos[0] + sz[0]-0.5, pos[1] + sz[1]-0.5);
        glVertex2f(pos[0]+0.5, pos[1] + sz[1]-0.5);
    glEnd();
}

void Button::setOnClicCallback(void (*_onClicCallback)(enum guiValue, Component *))
{
    onClicCallback = _onClicCallback;
}

void Button::setOnMouseOverCallback(void (*_onMouseOverCallback)(enum guiValue, Component *))
{
    onMouseOverCallback = _onMouseOverCallback;
}

void Button::ButtonClicCallback(enum guiValue button, enum guiValue state)
{
    if (state == GUI_DOWN) mouseOn = true;
    else
    {
        if (mouseOn && onClicCallback != NULL) onClicCallback(button, this);
        mouseOn = false;
    }
}

void Button::ButtonMoveCallback(enum guiValue action)
{
    if (action == GUI_MOUSE_LEAVE || action == GUI_MOUSE_ENTER)
    {
        mouseOn = false;
    }
    if (onMouseOverCallback != NULL) onMouseOverCallback(action, this);
}



// Labeled_Button

Labeled_Button::Labeled_Button(char * _label) : Button(), label(NULL)
{
    if (_label)  setLabel(_label);
}

Labeled_Button::~Labeled_Button()
{
    if (label) free(label);
	label = NULL;
}

const char * Labeled_Button::getLabel() const
{
    return label;
}

void Labeled_Button::setLabel(char * _label)
{
    if (label) free(label);
    label = NULL;
    label = strdup(_label);
}

void Labeled_Button::render(GraphicsContext& gc)
{
    Button::render(gc);
    vec2_i pos = getPosition();
    vec2_i sz = getSize();
    if (mouseOn) glColor3fv(gc.textColor / 2);
    else glColor3fv(gc.textColor);
    glEnable(GL_TEXTURE_2D);
    glPushMatrix();
    glTranslatef((int)(pos[0] + (sz[0] - gc.getFont()->getStrLen(label)) / 2),
                 (int)(pos[1] + (sz[1] - gc.getFont()->getLineHeight()) / 2),
                 0);
    gc.getFont()->print(0, 0, label);
    glPopMatrix();
}

// Textured_Button

Textured_Button::Textured_Button(s_texture * _texBt) : 
    Button(), 
    texBt(_texBt), 
    activeColor(1.0, 1.0, 1.0), 
    passiveColor(0.5, 0.5, 0.5)
{
}



Textured_Button::Textured_Button(
    s_texture * _texBt,
    vec2_i _position,
    vec2_i _size,
    vec3_t _activeColor,
    vec3_t _passiveColor,
    Callback1Base* _c1,
    Callback1Base* _c2,
    int _ID, int _active) :
        Button(), 
        texBt(_texBt),
        activeColor(_activeColor), 
        passiveColor(_passiveColor),
		c1(_c1), c2(_c2)
{
	if (!texBt)
	{
		printf("ERROR NO TEXTURE FOR TEXTURED BUTTON\n");
		exit(-1);
	}
    position = _position;
    size = _size;
    ID = _ID;
    active = _active;
}

Textured_Button::~Textured_Button()
{
    if (texBt) delete texBt;
    texBt = NULL;
}

void Textured_Button::render(GraphicsContext& gc)
{
    if (active)
    {
        glColor3fv(activeColor);
    }
    else
    {
        glColor3fv(gc.baseColor);
        //glColor3fv(passiveColor);
    }
    vec2_i pos = getPosition();
    vec2_i sz = getSize();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, texBt->getID());
    glBegin(GL_QUADS );
        glTexCoord2f(0.0f, 0.0f); glVertex2i(pos[0], pos[1] + sz[1]); // Bas Gauche
        glTexCoord2f(1.0f, 0.0f); glVertex2i(pos[0] + sz[0], pos[1] + sz[1]); // Bas Droite
        glTexCoord2f(1.0f, 1.0f); glVertex2i(pos[0] + sz[0], pos[1]); // Haut Droit
        glTexCoord2f(0.0f, 1.0f); glVertex2i(pos[0], pos[1]); // Haut Gauche
    glEnd ();
    Button::render(gc);
}

// Label

Label::Label(char * _label) : 
        Component(),
        label(NULL),
        theFont(NULL),
        colour(vec3_t( -1, -1, -1))
{
    setLabel(_label);
    passThru = true;
}

Label::Label(char * _label, s_font * _theFont) :
        Component(),
        label(NULL),
        theFont(_theFont),
        colour(vec3_t( -1, -1, -1))
{
    setLabel(_label);
    passThru = true;
}

Label::~Label()
{
    if (label) delete label;
	label=NULL;
}

const char * Label::getLabel() const
{
    return label;
}

void Label::setLabel(char * _label)
{
    if (label) delete label;
    label = strdup(_label);
}

void Label::render(GraphicsContext& gc)
{
    glEnable(GL_TEXTURE_2D);
    if (colour[0] < 0) glColor3fv(gc.textColor);
    else
        glColor3fv(colour);
    vec2_i pos = getPosition();
    vec2_i sz = getSize();
    glPushMatrix();
    if (theFont == NULL)
    {
        glTranslatef(pos[0], pos[1] + (sz[1] - gc.getFont()->getLineHeight()) / 2, 0);
        gc.getFont()->print(0, 0, label);
    }
    else
    {
        glTranslatef(pos[0], pos[1] + (sz[1] - theFont->getLineHeight()) / 2, 0);
        theFont->print(0, 0, label);
    }
    glPopMatrix();
}

// TextLabel
TextLabel::TextLabel(char * _label, s_font * _theFont) :
    Container(),
    label(NULL),
    theFont(_theFont),
    colour(vec3_t( -1, -1, -1))
{
    passThru = true;
    if (theFont == NULL) return ;
    setLabel(_label);
}

TextLabel::~TextLabel()
{
    if (label) delete label;
	label=NULL;
}

const char * TextLabel::getLabel() const
{
    return label;
}

void TextLabel::setLabel(char * _label)
{
    if (label) delete label;
    if (_label == NULL) return ;
    label = strdup(_label);
    components.clear();
    Label * tempLabel;
    char * pch;
    int i = 0;
    pch = strtok (label, "\n");
    while (pch != NULL)
    {
        tempLabel = new Label(pch, theFont);
        tempLabel->reshape(vec2_i(3,(int)( i*(theFont->getLineHeight() + 1) + 3)), vec2_i((int)theFont->getStrLen(pch), (int)theFont->getLineHeight()));
        tempLabel->setColour(colour);
        addComponent(tempLabel);
        pch = strtok (NULL, "\n");
        i++;
    }
}

void TextLabel::setColour(vec3_t _colour)
{
    colour = _colour;
    vector<Component*>::iterator iter = components.begin();
    while (iter != components.end())
    {
        {
            ((Label*)(*iter))->setColour(_colour);
        }
        iter++;
    }
}

// FilledTextLabel
FilledTextLabel::FilledTextLabel(char * _label, s_font * _theFont) : TextLabel(_label, _theFont)
{
}


void FilledTextLabel::render(GraphicsContext& gc)
{
    glColor3fv(gc.baseColor);
    vec2_i pos = getPosition();
    vec2_i sz = getSize();

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, gc.backGroundTexture->getID());
    glBegin(GL_QUADS );
        glTexCoord2f(0.0f, 0.0f); glVertex2i(pos[0], pos[1] + sz[1]); // Bas Gauche
        glTexCoord2f(1.0f, 0.0f); glVertex2i(pos[0] + sz[0], pos[1] + sz[1]); // Bas Droite
        glTexCoord2f(1.0f, 1.0f); glVertex2i(pos[0] + sz[0], pos[1]); // Haut Droit
        glTexCoord2f(0.0f, 1.0f); glVertex2i(pos[0], pos[1]); // Haut Gauche
    glEnd ();

    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINE_LOOP);
        glVertex2f(pos[0]+0.5, pos[1]+0.5);
        glVertex2f(pos[0] + sz[0]-0.5, pos[1]+0.5);
        glVertex2f(pos[0] + sz[0]-0.5, pos[1] + sz[1]-0.5);
        glVertex2f(pos[0]+0.5, pos[1] + sz[1]-0.5);
    glEnd();

    TextLabel::render(gc);
}


// Cursor Bar
void ExtCursorBarClicCallback(int x, int y, enum guiValue state, enum guiValue button, Component * me)
{
    ((CursorBar*)me)->CursorBarClicCallback(x, button, state);
}

void ExtCursorBarMoveCallback(int x, int y, enum guiValue action, Component * me)
{
    ((CursorBar*)me)->CursorBarMoveCallback(x, action);
}

CursorBar::CursorBar(
    vec2_i _position,
    vec2_i _size,
    float _minBarValue,
    float _maxBarValue,
    float _barValue,
    void(*_onValueChangeCallBack)(float _barValue, Component *)) :
        Component(), 
        mouseOn(false), 
        minBarValue(_minBarValue),
        maxBarValue(_maxBarValue),
        barValue(_barValue),
        onValueChangeCallBack(_onValueChangeCallBack)
{
    reshape(_position, _size);
    setClicCallback(ExtCursorBarClicCallback);
    setMoveCallback(ExtCursorBarMoveCallback);
}

void CursorBar::render(GraphicsContext& gc)
{
    glColor3fv(gc.baseColor);
    glDisable(GL_TEXTURE_2D);
    vec2_i pos = getPosition();
    vec2_i sz = getSize();
    glBegin(GL_LINE_LOOP);
    	glVertex2i(pos[0] + 3 , pos[1] + sz[1] / 2 - 1);
    	glVertex2i(pos[0] + sz[0] - 3 , pos[1] + sz[1] / 2 - 1);
    	glVertex2i(pos[0] + sz[0] - 3 , pos[1] + sz[1] / 2 + 1);
    	glVertex2i(pos[0] + 3 , pos[1] + sz[1] / 2 + 1);
    	glVertex2i(pos[0] + 3 , pos[1] + sz[1] / 2 - 1);
    glEnd();
    float xpos = pos[0] + 3 + (sz[0] - 6) * ((float)(barValue - minBarValue) / (float)(maxBarValue - minBarValue));
    glColor3fv(gc.baseColor*2);
    glBegin(GL_LINE_LOOP);
    	glVertex2f(xpos - 2 , sz[1] / 2 + pos[1] - 5);
    	glVertex2f(xpos + 2 , sz[1] / 2 + pos[1] - 5);
    	glVertex2f(xpos + 2 , sz[1] / 2 + pos[1] + 5);
    	glVertex2f(xpos - 2 , sz[1] / 2 + pos[1] + 5);
    	glVertex2f(xpos - 2 , sz[1] / 2 + pos[1] - 5);
    glEnd();
}

void CursorBar::CursorBarClicCallback(int x, enum guiValue button, enum guiValue state)
{
    if (state == GUI_DOWN)
    {
        mouseOn = true;
        draging = true;
        vec2_i pos = getPosition();
        vec2_i sz = getSize();
        float xpos = x - pos[0] - 3;
        barValue = minBarValue + xpos / (sz[0] - 6) * (maxBarValue - minBarValue);
        if (barValue < minBarValue) barValue = minBarValue;
        if (barValue > maxBarValue) barValue = maxBarValue;
        if (onValueChangeCallBack != NULL) onValueChangeCallBack(barValue, this);
    }
    else
    {
        if (state == GUI_UP) mouseOn = false;
        draging = false;
    }
}

void CursorBar::CursorBarMoveCallback(int x, enum guiValue action)
{
    if (action == GUI_MOUSE_ENTER)
    {
        mouseOn = false;
    }
    if (draging)
    {
        float oldBarValue = barValue;
        vec2_i pos = getPosition();
        vec2_i sz = getSize();
        int xpos = x - pos[0] - 3;
        barValue = minBarValue + (float)xpos / ((float)sz[0] - 6) * (maxBarValue - minBarValue);
        if (barValue < minBarValue) barValue = minBarValue;
        if (barValue > maxBarValue) barValue = maxBarValue;
        if (oldBarValue != barValue && onValueChangeCallBack != NULL) onValueChangeCallBack(barValue, this);
    }
}

void CursorBar::setValue(float _barValue)
{
	if (_barValue==barValue) return;
	barValue=_barValue;
	onValueChangeCallBack(barValue, this);
}

Picture::Picture(vec2_i _position, vec2_i _size, s_texture * _imageTex) : imageTex(_imageTex)
{
	if (!imageTex)
	{
		printf("ERROR NO TEXTURE FOR PICTURE WIDGET\n");
		exit(-1);
	}
    reshape(_position, _size);
}

Picture::~Picture()
{
    if (imageTex) delete imageTex;
    imageTex = NULL;
}

void Picture::render(GraphicsContext&)
{
    glColor3f(1.0,1.0,1.0);
    vec2_i pos = getPosition();
    vec2_i sz = getSize();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, imageTex->getID());
    glBegin(GL_QUADS );
        glTexCoord2f(0.0f, 0.0f); glVertex2i(pos[0], pos[1] + sz[1]); // Bas Gauche
        glTexCoord2f(1.0f, 0.0f); glVertex2i(pos[0] + sz[0], pos[1] + sz[1]); // Bas Droite
        glTexCoord2f(1.0f, 1.0f); glVertex2i(pos[0] + sz[0], pos[1]); // Haut Droit
        glTexCoord2f(0.0f, 1.0f); glVertex2i(pos[0], pos[1]); // Haut Gauche
    glEnd ();
}

BorderPicture::BorderPicture(vec2_i _position, vec2_i _size, s_texture * _imageTex) : Picture(_position,_size,_imageTex)
{}


void BorderPicture::render(GraphicsContext& gc)
{
	Picture::render(gc);

    glColor3fv(gc.baseColor);
    vec2_i pos = getPosition();
    vec2_i sz = getSize();
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINE_LOOP);
        glVertex2f(pos[0]+0.5, pos[1]+0.5);
        glVertex2f(pos[0] + sz[0]-0.5, pos[1]+0.5);
        glVertex2f(pos[0] + sz[0]-0.5, pos[1] + sz[1]-0.5);
        glVertex2f(pos[0]+0.5, pos[1] + sz[1]-0.5);
    glEnd();
}

// ClickablePicture

void ExtClickablePictureClicCallback(int x, int y, enum guiValue state, enum guiValue button, Component * me)
{
    ((ClickablePicture*)me)->ClickablePictureClicCallback(x,y,button, state);
}


ClickablePicture::ClickablePicture(vec2_i _position, vec2_i _size, s_texture * _imageTex, void (*_onValueChangeCallBack)(vec2_t _pointerPosition, Component *)) : BorderPicture(_position, _size, _imageTex)
{
	setClicCallback(ExtClickablePictureClicCallback);
	onValueChangeCallBack=_onValueChangeCallBack;
	pointerPosition=vec2_t(0.0,0.0);
}

void ClickablePicture::render(GraphicsContext& gc)
{
	BorderPicture::render(gc);

	vec2_i pos = getPosition();
	glTranslatef(pos[0]+0.5, pos[1]+0.5, 0);
	// Draw the pointer
	glColor3f(1.0,1.0,1.0);
    glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
		glVertex2f(pointerPosition[0]+1,pointerPosition[1]);
		glVertex2f(pointerPosition[0]+4,pointerPosition[1]);
		glVertex2f(pointerPosition[0]-1,pointerPosition[1]);
		glVertex2f(pointerPosition[0]-4,pointerPosition[1]);
		glVertex2f(pointerPosition[0],pointerPosition[1]+1);
		glVertex2f(pointerPosition[0],pointerPosition[1]+4);
		glVertex2f(pointerPosition[0],pointerPosition[1]-1);
		glVertex2f(pointerPosition[0],pointerPosition[1]-4);
    glEnd();
	glTranslatef(-pos[0]-0.5, -pos[1]-0.5, 0);
}

void ClickablePicture::setPointerPosition(vec2_t _pointerPosition)
{
	pointerPosition=_pointerPosition;
}

void ClickablePicture::ClickablePictureClicCallback(int x, int y, enum guiValue button, enum guiValue state)
{
    if (state == GUI_DOWN)
    {
		vec2_i pos = getPosition();
		pointerPosition[0]=x-pos[0];
		pointerPosition[1]=y-pos[1];
        if (onValueChangeCallBack != NULL) onValueChangeCallBack(pointerPosition, this);
    }
}

*/
