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

#include "s_gui.h"

#include <stdio.h>

using namespace std;
using namespace gui;


/**** GraphicsContext ****/

GraphicsContext::GraphicsContext(int _winW, int _winH) :
        backGroundTexture(NULL),
        headerTexture(NULL),
        baseColor(vec3_t(0.6, 0.4, 0.1)),
        textColor(vec3_t(0.1, 0.9, 0.8)),
        scissorPos(vec2_i(0, 0)),
        winW(_winW),
        winH(_winH),
        font(NULL)
{
}

GraphicsContext::~GraphicsContext()
{
    if (backGroundTexture) delete backGroundTexture;
    backGroundTexture = NULL;
    if (headerTexture) delete headerTexture;
    headerTexture = NULL;
}

s_font* GraphicsContext::getFont() const
{
    return font;
}

void GraphicsContext::setFont(s_font* _font)
{
    font = _font;
}


/**** Component ****/

Component::Component() :
        draging(false),
        passThru(false),
        ID( -1),
        visible(true),
        position(0, 0),
        size(0, 0),
        parent(NULL),
        clicCallback(NULL),
        moveCallback(NULL),
        mouseIn(false),
        active(true),
        focus(false)
{
}

Component::~Component()
{
}

vec2_i Component::getPosition() const
{
    return position;
}

vec2_i Component::getSize() const
{
    return size;
}

void Component::reshape(vec2_i _position, vec2_i _size)
{
    position = _position;
    size = _size;
}

void Component::reshape(int x, int y, int w, int h)
{
	position[0]=x;
	position[1]=y;
	size[0]=w;
	size[1]=h;
}

Component* Component::getParent() const
{
    return parent;
}

void Component::setParent(Component* c)
{
    parent = c;
}

void Component::setClicCallback(void (*_clicCallback)(int, int, enum guiValue, enum guiValue, Component *))
{
    clicCallback = _clicCallback;
}

void Component::setMoveCallback(void (*_moveCallback)(int, int, enum guiValue, Component *))
{
    moveCallback = _moveCallback;
}

void Component::setKeyCallback(int (*_keyCallback)(SDLKey key, int state, Component *))
{
    keyCallback = _keyCallback;
}

/**** Container ****/

void containerClicCallBack(int x, int y, enum guiValue state, enum guiValue button, Component * caller)
{
    ((Container *)caller)->handleMouseClic((int)(x - caller->getPosition()[0]), (int)(y - caller->getPosition()[1]), state, button);
}

void containerMoveCallBack(int x, int y, enum guiValue, Component * caller)
{
    ((Container *)caller)->handleMouseMove((int)(x - caller->getPosition()[0]), (int)(y - caller->getPosition()[1]));
}

int containerKeyCallBack(SDLKey key, int state, Component * caller)
{
    return ((Container *)caller)->handleKey(key, state);
}

Container::Container() : Component()
{
    setClicCallback(containerClicCallBack);
    setMoveCallback(containerMoveCallBack);
    setKeyCallback(containerKeyCallBack);
}

Container::~Container()
{
    //printf("Enter Container destructor %d\n",this);
    vector<Component*>::iterator iter = components.begin();
    while (iter != components.end())
    {
        if (*iter)
        {
            delete (*iter);
            (*iter)=NULL;
        }
        iter++;
    }
    //printf("Quit Container destructor %d\n",this);
}

int Container::getComponentCount() const
{
    return components.size();
}

Component* Container::getComponent(int n) const
{
    if (n >= 0 && (unsigned int)n < components.size())
        return components[n];
    else
        return NULL;
}

void Container::addComponent(Component* c)
{   components.insert(components.end(), c);
    c->setParent(this);
}

void Container::render(GraphicsContext& gc)
{
    if (!visible) return ;
    
    vec2_i pos = getPosition();
    vec2_i sz = getSize();

    vector<Component*>::iterator iter = components.begin();
    glPushMatrix();

    glTranslatef(pos[0], pos[1], 0);
    gc.scissorPos += pos;
    while (iter != components.end())
    {
        if ((*iter)->visible)
        {
            glScissor(gc.scissorPos[0], gc.winH - gc.scissorPos[1] - sz[1], sz[0], sz[1]);
            (*iter)->render(gc);
        }
        iter++;
    }
    gc.scissorPos -= pos;
    glPopMatrix();
}

int Container::handleMouseClic(int x, int y, enum guiValue state, enum guiValue button)
{
    vector<Component*>::iterator iter = components.begin();
    while (iter != components.end())
    {
        if ((*iter)->visible && (*iter)->clicCallback != NULL && ((*iter)->draging || (*iter)->isIn(x, y)))
        {
            (*iter)->clicCallback(x, y, state, button, (*iter));
            if (!(*iter)->passThru) return 1;
        }
        iter++;
    }
    return 0;
}

void Container::handleMouseMove(int x, int y)
{
    vector<Component*>::iterator iter = components.begin();
    while (iter != components.end())
    {
        if ((*iter)->visible && (*iter)->moveCallback != NULL)
        {
            if ((*iter)->draging || (*iter)->isIn(x, y))
            {
                if (!(*iter)->mouseIn)
                {
                    (*iter)->moveCallback(x, y, GUI_MOUSE_ENTER, (*iter));
                    (*iter)->mouseIn = true;
                }
                else
                {
                    (*iter)->moveCallback(x, y, GUI_NOTHING, (*iter));
                }
            }
            else
            {
                if ((*iter)->mouseIn) (*iter)->moveCallback(x, y, GUI_MOUSE_LEAVE, (*iter));
                (*iter)->mouseIn = false;
            }
        }
        iter++;
    }
}

int Container::handleKey(SDLKey key,int state)
{
    vector<Component*>::iterator iter = components.begin();
    while (iter != components.end())
    {
        if ((*iter)->focus && (*iter)->visible && (*iter)->keyCallback != NULL)
        {
            return (*iter)->keyCallback(key, state, (*iter));
        }
        iter++;
    }
    return 0;
}

/*** FilledContainer ***/

void FilledContainer::render(GraphicsContext& gc)
{
    glColor3fv(gc.baseColor);
    vec2_i pos = getPosition();
    vec2_i sz = getSize();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, gc.backGroundTexture->getID());
    glBegin(GL_QUADS );
        glTexCoord2f(0.0f, 0.0f); glVertex2f(pos[0]+0.5, pos[1] + sz[1]-0.5); // Bas Gauche
        glTexCoord2f(1.0f, 0.0f); glVertex2f(pos[0] + sz[0]-0.5, pos[1] + sz[1]-0.5); // Bas Droite
        glTexCoord2f(1.0f, 1.0f); glVertex2f(pos[0] + sz[0]-0.5, pos[1]+0.5); // Haut Droit
        glTexCoord2f(0.0f, 1.0f); glVertex2f(pos[0]+0.5, pos[1]+0.5); // Haut Gauche
    glEnd ();
 
    glColor3fv(gc.baseColor*0.7);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINE_LOOP);
        glVertex2f(pos[0] +0.5, pos[1]+0.5);
        glVertex2f(pos[0] + sz[0]-0.5, pos[1]+0.5);
        glVertex2f(pos[0] + sz[0]-0.5, pos[1] + sz[1]-0.5);
        glVertex2f(pos[0]+0.5, pos[1] + sz[1]-0.5);
    glEnd();
 
    Container::render(gc);
}


/**** Button ****/

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



/**** Labeled_Button ****/

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

/**** Textured_Button ****/

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
    void (*_onClicCallback)(enum guiValue, Component *),
    void (*_onMouseOverCallback)(enum guiValue, Component *),
    int _ID, int _active) : 
        Button(), 
        texBt(_texBt),
        activeColor(_activeColor), 
        passiveColor(_passiveColor)
{
	if (!texBt)
	{
		printf("ERROR NO TEXTURE FOR TEXTURED BUTTON\n");
		exit(-1);
	}
    position = _position;
    size = _size;
    setOnClicCallback(_onClicCallback);
    setOnMouseOverCallback(_onMouseOverCallback);
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

/**** Label ****/

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

/**** TextLabel ****/
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

/**** FilledTextLabel ****/
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


/**** Cursor Bar ****/
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

/**** ClickablePicture ****/

void ExtClickablePictureClicCallback(int x, int y, enum guiValue state, enum guiValue button, Component * me)
{
    ((ClickablePicture*)me)->ClickablePictureClicCallback(x,y,button, state);
}

/*void ExtClickablePictureMoveCallback(int x, int y, enum guiValue action, Component * me)
{
    ((ClickablePicture*)me)->ClickablePictureMoveCallback(action);
}*/

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
