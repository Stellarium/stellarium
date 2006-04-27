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

#include <iostream>
#include <sstream>
#include <fstream>
#include "SDL_timer.h"
#include "s_gui.h"
#include "stel_utility.h"

using namespace std;
using namespace s_gui;

bool EditBox::cursorVisible = false;
EditBox * EditBox::activeEditBox = NULL;
		
void glCircle(const Vec3d& pos, float radius, float line_width)
{
	float angle, facets;
	bool lastState = glIsEnabled(GL_TEXTURE_2D);
	
	glDisable(GL_TEXTURE_2D);
	glLineWidth(line_width);
	glBegin(GL_LINE_LOOP);

	if (radius < 2) facets = 6;
	else facets = (int)(radius*3);
	
	for (int i = 0; i < facets; i++)
	{
		angle = 2.0f*M_PI*i/facets;
		glVertex3f(pos[0] + radius * sin(angle), pos[1] + radius * cos(angle), 0.0f);
	}
	glEnd();
	
	if (lastState) glEnable(GL_TEXTURE_2D);
	glLineWidth(1.0f);
}

void glEllipse(const Vec3d& pos, float radius, float y_ratio, float line_width)
{
	float angle, facets;
	bool lastState = glIsEnabled(GL_TEXTURE_2D);
	
	glDisable(GL_TEXTURE_2D);
	glLineWidth(line_width);
	glBegin(GL_LINE_LOOP);

	if (radius < 2) facets = 6;
	else facets = (int)(radius*3);
	
	for (int i = 0; i < facets; i++)
	{
		angle = 2.0f*M_PI*i/facets;
		glVertex3f(pos[0] + radius * sin(angle), pos[1] + y_ratio* radius * cos(angle), 0.0f);
	}
	glEnd();
	
	if (lastState) glEnable(GL_TEXTURE_2D);
	glLineWidth(1.0f);
}


void *lastCallback;
#define RUNCALLBACK(c) doCallback((c), (void*)this)

void* getLastCallbackObject(void)
{
	return lastCallback;
}

inline void doCallback(const callback<void>& c, void *_component)
{
	lastCallback = _component;
	c();
}


// A float to nearest int conversion routine for the systems which
// are not C99 conformant
inline int S_ROUND(float value)
{
	static double i_part;

	if (value >= 0)
	{
		if (modf((double) value, &i_part) >= 0.5)
			i_part += 1;
	}
	else
	{
		if (modf((double) value, &i_part) <= -0.5)
			i_part -= 1;
	}

	return ((int) i_part);
}

Scissor* Component::scissor = NULL;			// Default initialization
Painter Component::defaultPainter;

///////////////////////////////// Scissor //////////////////////////////////////
// Manages the use of the OpenGL Scissor test to prevent drawings outside the
// components borders. Use a stack like for openGL matrices.
////////////////////////////////////////////////////////////////////////////////

Scissor::Scissor(int _winW, int _winH) :
	winW(_winW),
	winH(_winH)
{
	push(0, 0, winW, winH);
}

// Define a new GlScissor zone relative to the previous one and apply it so
// that nothing can be drawn outside the limits
void Scissor::push(int posx, int posy, int sizex, int sizey)
{
	if (stack.empty())
	{
		s_square el(posx, posy, sizex, sizey);

		// Apply the new values
		glScissor(el[0], winH - el[1] - el[3], el[2], el[3]);
		// Add the new value at the end of the stack
		stack.push_front(el);
	}
	else
	{
		s_square v = *stack.begin();
		s_square el(v[0] + posx, v[1] + posy, sizex, sizey);

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
		stack.push_front(el);
	}
}

// See above
void Scissor::push(const s_vec2i& pos, const s_vec2i& size)
{
	if (stack.empty())
	{
		s_square el(pos[0], pos[1], size[0], size[1]);
		// Apply the new values
		glScissor(el[0], winH - el[1] - el[3], el[2], el[3]);
		// Add the new value at the end of the stack
		stack.push_front(el);
	}
	else
	{
		s_square v = *stack.begin();
		s_square el(v[0] + pos[0], v[1] + pos[1], size[0], size[1]);

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
		stack.push_front(el);
	}

}

// Remove the last element in the stack : ie comes back to the previous
// GlScissor borders.
void Scissor::pop(void)
{
	// Remove the last value from the stack
	stack.pop_front();

	if (!stack.empty())
	{
		// Apply the previous value
		s_square v = *stack.begin();
		glScissor(v[0], winH - v[1] - v[3], v[2], v[3]);
	}
}


//////////////////////////////// Painter ///////////////////////////////////////
// Class used to manage all the drawings for a component. Stores informations
// like colors or used textures. Performs the primitives drawing.
////////////////////////////////////////////////////////////////////////////////

Painter::Painter() :
	tex1(NULL),
	font(NULL),
	opaque(false)
{
}

Painter::Painter(const s_texture* _tex1, s_font* _font,
	const s_color& _baseColor, const s_color& _textColor) :
		tex1(_tex1),
		font(_font),
		baseColor(_baseColor),
		textColor(_textColor),
		opaque(false)
{
}

Painter::~Painter()
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

	if (opaque) glDisable(GL_BLEND);
	else glEnable(GL_BLEND);

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

//	if (opaque) glDisable(GL_BLEND);
	//else 
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
void Painter::drawSquareFill(const s_vec2i& pos, const s_vec2i& sz,
                             const s_color& c, const s_texture * t) const
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

// Draw a cross with the default base color (part of checkbox requirements)
void Painter::drawCross(const s_vec2i& pos, const s_vec2i& sz) const
{
	glColor4fv(baseColor);
    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
        glVertex2f(pos[0] + 0.5		   , pos[1] + 0.5        );
        glVertex2f(pos[0] + sz[0] - 0.5, pos[1] + sz[1] - 0.5);
        glVertex2f(pos[0] + sz[0] - 0.5, pos[1] + 0.5        );
        glVertex2f(pos[0] + 0.5		   , pos[1] + sz[1] - 0.5);
    glEnd();
}

// Print the text with the default font and default text color
void Painter::print(int x, int y, const string& str)
{
    glColor4fv(textColor);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
	font->print(x, y, str, 0);	// 0 for upside down mode
}

// Print the text with the default font and default text color
void Painter::print(int x, int y, const wstring& str)
{
    glColor4fv(textColor);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
	font->print(x, y, str, 0);	// 0 for upside down mode
}

// Print the text with the default font and given text color
void Painter::print(int x, int y, const string& str, const s_color& c)
{
    glColor4fv(c);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
	font->print(x, y, str, 0);	// 0 for upside down mode
}

// Print the text with the default font and given text color
void Painter::print(int x, int y, const wstring& str, const s_color& c)
{
    glColor4fv(c);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
	font->print(x, y, str, 0);	// 0 for upside down mode
}

void Painter::drawLine(const s_vec2i& pos1, const s_vec2i& pos2) const
{
	glColor4fv(baseColor);
    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
        glVertex2f(pos1[0] + 0.5, pos1[1] + 0.5);
        glVertex2f(pos2[0] + 0.5, pos2[1] + 0.5);
    glEnd();
}

void Painter::drawLine(const s_vec2i& pos1, const s_vec2i& pos2, const s_color& c) const
{
	glColor4fv(c);
    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
        glVertex2f(pos1[0] + 0.5, pos1[1] + 0.5);
        glVertex2f(pos2[0] + 0.5, pos2[1] + 0.5);
    glEnd();
}

//////////////////////////////// Component /////////////////////////////////////
// Mother class for every s_gui object.
////////////////////////////////////////////////////////////////////////////////

bool Component::focusing = false;

Component::Component() :
	pos(0, 0),
	size(0, 0),
	visible(true),
	focus(false),
	painter(defaultPainter),
	GUIColorSchemeMember(true),
	moveToFront(false),
	needNewTopEdit(false),
	type(CT_COMPONENTBASE),
	desktop(false)
{
}

Component::~Component()
{
}

void Component::draw(void)
{
}

void Component::setVisible(bool _visible)
{
	visible = _visible;
}


void Component::reshape(const s_vec2i& _pos, const s_vec2i& _size)
{
	pos = _pos;
	size = _size;
}

void Component::reshape(int x, int y, int w, int h)
{
	pos.set(x, y);
	size.set(w, h);
}

bool Component::isIn(int x, int y)
{
	return (pos[0]<=x && (size[0]+pos[0])>=x && pos[1]<=y && (pos[1]+size[1])>=y);
}

void Component::initScissor(int winW, int winH)
{
	if (scissor) delete scissor;
	scissor = new Scissor(winW, winH);
}

void Component::deleteScissor(void)
{
	if (scissor) delete scissor;
	scissor = NULL;
}

///////////////////////////// CallbackComponent ////////////////////////////////
// Manages hierarchical components : send signals and actions to childrens
////////////////////////////////////////////////////////////////////////////////

CallbackComponent::CallbackComponent() : Component(), is_mouse_over(false), pressed(false)
{
}

bool CallbackComponent::onClic(int x, int y, S_GUI_VALUE bt, S_GUI_VALUE state)
{
	if (!visible) return false;
	pressed = false;
	if (state==S_GUI_PRESSED && bt==S_GUI_MOUSE_LEFT && isIn(x, y))
	{
		pressed = true;
	}
	if (state==S_GUI_RELEASED && bt==S_GUI_MOUSE_LEFT && isIn(x, y))
	{
		if (!onPressCallback.empty()) RUNCALLBACK(onPressCallback);
	}
	return false;
}

bool CallbackComponent::onMove(int x, int y)
{
	if (!visible) 
	{
		is_mouse_over = false;
		pressed = false;
		return false;
	}
	if (isIn(x, y))
	{
		if (!onMouseInOutCallback.empty() && !is_mouse_over)
		{
			is_mouse_over = true;
			RUNCALLBACK(onMouseInOutCallback);
		}
		is_mouse_over = true;
	}
	else
	{
		pressed = false;
		if (is_mouse_over)
		{
			if (!onMouseInOutCallback.empty() && is_mouse_over)
			{
				is_mouse_over = false;
				RUNCALLBACK(onMouseInOutCallback);
			}
		}
		is_mouse_over = false;
	}
	return false;
}

//////////////////////////////// Container /////////////////////////////////////
// Manages hierarchical components : send signals and actions to childrens
////////////////////////////////////////////////////////////////////////////////

Container::Container(bool _desktop) : 
	CallbackComponent()
{
	type += CT_CONTAINER;
	desktop = _desktop;
}

Container::~Container()
{
    list<Component*>::iterator iter = childs.begin();
    while (iter != childs.end())
    {
        delete (*iter);
        (*iter)=NULL;
        iter++;
    }
}

void Container::setGUIColorSchemeMember(bool _b)
{
    GUIColorSchemeMember = _b;
}

void Container::addComponent(Component* c)
{
	childs.push_front(c);
}

void Container::removeComponent(Component* c)
{
    list<Component*>::iterator iter = childs.begin();
	while (iter != childs.end() || *iter != c)
	{
		++iter;
	}
	if (iter==childs.end()) return;
	childs.erase(iter);
}

void Container::removeAllComponents(void)
{
	childs.clear();
}

void Container::setFocus(bool _focus)
{
	// set the focus to all children

	list<Component*>::iterator iter = childs.begin();
	while (iter != childs.end())
	{
		(*iter)->setFocus(_focus);
        iter++;
	}
	focus=_focus;
	focusing = _focus;
}

void Container::setColorScheme(const s_color& baseColor, const s_color& textColor)
{
	if (!GUIColorSchemeMember) return;

    list<Component*>::iterator iter = childs.begin();
	while (iter != childs.end())
	{
		(*iter)->setColorScheme(baseColor, textColor);
		iter++;
	}
	painter.setTextColor(textColor);
	painter.setBaseColor(baseColor);
}

void Component::setColorScheme(const s_color& baseColor, const s_color& textColor)
{
	if (!GUIColorSchemeMember) return;
	painter.setTextColor(textColor);
	painter.setBaseColor(baseColor);
}

void Container::draw(void)
{
    if (!visible) return;
	static Component *firstFocus = NULL;
	static bool justMovedToFront;
	static bool neededNewTopEdit;
    
    if (desktop) // first draw of cycle
	{
	    firstFocus = NULL;
    	justMovedToFront = false;
    	neededNewTopEdit = false;
	}
    
    list<Component*>::iterator iter = childs.begin();
	
	// look through the sub children for an in front component
	
	while (iter != childs.end())
	{
		if ((*iter)->inFront() == true)
		{
			EditBox::activeEditBox = NULL;
			(*iter)->setInFront(false);
			childs.push_front(*iter);
			childs.erase(iter);
			justMovedToFront = true;
			break;
		}
		iter++;
	}
	
    // reverse the child order so the current control is drawn on last (top)
    childs.reverse();

	// Draw the components
    iter = childs.begin();
    glPushMatrix();
    glTranslatef(pos[0], pos[1], 0.f);

    Component::scissor->push(pos, size);

	while (iter != childs.end())
	{
		if (((*iter)->getType() & CT_EDITBOX) && (*iter)->getVisible())
			firstFocus = (*iter);
		if ((*iter)->getNeedNewEdit())
		{
			(*iter)->setNeedNewEdit(false);
			neededNewTopEdit = true;
		}
		(*iter)->draw();
		iter++;
	}

    Component::scissor->pop();
    
	// swap back now the actual order of the components 
    childs.reverse();

    glPopMatrix();
    
	// after all the desktop and sub compopnents have been drawn
	// see a editbox that is displaying needs a cursor?
    if (desktop)
	{
		 //need to check that the firstfocus is a child of the one just moved to
		 //the front, otherwise it may be close to the top and blinking,
		 //but not the currently top window!
		 
		if (firstFocus && (justMovedToFront || neededNewTopEdit))
	    {
		    if (((EditBox*)firstFocus)->getAutoFocus())
				((EditBox*)firstFocus)->setEditing(true);
			firstFocus = NULL;
		}
	}
}

bool Container::onClic(int x, int y, S_GUI_VALUE button, S_GUI_VALUE state)
{
	if (!visible) return false;

	bool hasFocus = false;
	
	list<Component*>::iterator iter = childs.begin();

	// check if a component has elected sole focus for it and its children
	while (iter != childs.end())
	{
		if ((*iter)->getFocus())	
		{
			hasFocus = true;
			if ((*iter)->onClic(x - pos[0], y - pos[1], button, state)) 
			{
				// The signal has been intercepted
				// Set the component in first position in the objects list
				childs.push_front(*iter);
				childs.erase(iter);
				return true;
			}
		}
        iter++;
	}
	// focus but not handled - return unhandled
	if (hasFocus) return false;
	
	iter = childs.begin();
	while (iter != childs.end())
	{
		if ((*iter)->onClic(x - pos[0], y - pos[1], button, state))
		{
			// The signal has been intercepted
			// Set the component in first position in the objects list
			childs.push_front(*iter);
			childs.erase(iter);
			return true;
		}
        iter++;
    }
	return CallbackComponent::onClic(x, y, button, state);
}

bool Container::onMove(int x, int y)
{
	bool hasFocus = false;

	if (!visible) return false;
	list<Component*>::iterator iter = childs.begin();

	// check if a component has elected sole focus for it and its children
	while (iter != childs.end())
	{
		if ((*iter)->getFocus())	
		{
			hasFocus = true;
			if ((*iter)->onMove(x - pos[0], y - pos[1])) return true;
		}
        iter++;
	}
	// focus but not handled - return
	if (hasFocus) return CallbackComponent::onMove(x, y);
	
	iter = childs.begin();

	while (iter != childs.end())
	{
		if ((*iter)->onMove(x - pos[0], y - pos[1])) return true;	// The signal has been intercepted
        iter++;
    }
	return CallbackComponent::onMove(x, y);
}

bool Container::onKey(Uint16 k, S_GUI_VALUE s)
{
	bool hasFocus = false;
	if (!visible) return false;
	list<Component*>::iterator iter = childs.begin();

	// check if a component has elected sole focus for it and
	// its children
	while (iter != childs.end())
	{
		if ((*iter)->getFocus())	
		{
			hasFocus = true;
			if ((*iter)->onKey(k,s)) return true;
		}
        iter++;
	}
	// not handled, so we return at this point saying keys have been handled
	if (hasFocus) return true;
	
	iter = childs.begin();
	while (iter != childs.end())
	{
		if ((*iter)->onKey(k, s)) return 1;	// The signal has been intercepted
        iter++;
    }
    return false;
}

//////////////////////////// FilledContainer ///////////////////////////////////
// Container filled with a texture and with an edge
////////////////////////////////////////////////////////////////////////////////


void FilledContainer::draw(void)
{
    if (!visible) return;
	painter.drawSquareFill(pos, size);

	Container::draw();
}


////////////////////////////////// Button //////////////////////////////////////
// Simplest button with one press callback
////////////////////////////////////////////////////////////////////////////////

#define BUTTON_HEIGHT 25

Button::Button() : 
	CallbackComponent(),
	hideBorder(false),
	hideBorderMouseOver(false),
	hideTexture(false)
{
	size.set(10,10);
}

void Button::draw()
{
	if (!visible) return;

	if (!is_mouse_over)
	{
		if (!hideBorder)
			painter.drawSquareEdge(pos, size, painter.getBaseColor() * 1.f);
	}
	else
	{
		if (!hideBorderMouseOver)
			painter.drawSquareEdge(pos, size, painter.getBaseColor() * 1.4f);
	}
}

bool Button::onClic(int x, int y, S_GUI_VALUE bt, S_GUI_VALUE state)
{
	if (!visible) return false;

	CallbackComponent::onClic(x,y,bt,state);
	if (bt==S_GUI_MOUSE_LEFT && isIn(x, y)) return 1;
	return 0;
}

void FilledButton::draw()
{
	if (!visible) return;
	painter.drawSquareFill(pos, size);
	Button::draw();
}

//////////////////////////////////// TexturedButton //////////////////////////////////
// Button with a texture
////////////////////////////////////////////////////////////////////////////////


TexturedButton::TexturedButton(const s_texture* tex)
{
	if (tex) setTexture(tex);
}

void TexturedButton::draw()
{
	if (!visible) return;
	painter.drawSquareFill(pos, size, painter.getBaseColor() * (1.f + 0.4 * is_mouse_over));
}

//////////////////////////////////// CheckBox //////////////////////////////////
// Button with a cross on it
////////////////////////////////////////////////////////////////////////////////

CheckBox::CheckBox(bool state) : Button(), isChecked(state)
{
}

void CheckBox::draw()
{
	if (!visible) return;
	if (isChecked) painter.drawCross(pos, size);
	Button::draw();
}

bool CheckBox::onClic(int x, int y, S_GUI_VALUE bt, S_GUI_VALUE state)
{
	if (!visible) return false;
	if (state==S_GUI_RELEASED && bt==S_GUI_MOUSE_LEFT && isIn(x, y))
		isChecked = !isChecked;
	return Button::onClic(x,y,bt,state);
}

LabeledCheckBox::LabeledCheckBox(bool state, const wstring& _label) : 
		Container(), 
		checkbox(NULL), 
		label(NULL)
{
	checkbox = new CheckBox(state);
	addComponent(checkbox);
	label = new Label(_label);
	label->setPos(checkbox->getSizex()+4, -(int)label->getFont()->getDescent());
	addComponent(label);
	setSize(checkbox->getSizex() + label->getSizex() + 4,
		checkbox->getSizey()>label->getSizey() ? checkbox->getSizey() : label->getSizey());
}

LabeledCheckBox::~LabeledCheckBox()
{
}

void LabeledCheckBox::draw(void)
{
	if (!visible) return;
	
	setSize(checkbox->getSizex() + label->getSizex() + 4,
		checkbox->getSizey()>label->getSizey() ? checkbox->getSizey() : label->getSizey());

	Container::draw();
}

FlagButton::FlagButton(int state, const s_texture* tex,
                       const string& specificTexName) : CheckBox(state)
{
	if (tex) setTexture(tex);
	if (!specificTexName.empty()) specific_tex = new s_texture(specificTexName);
	setSize(24,24);
}

FlagButton::~FlagButton()
{
	if (specific_tex) delete specific_tex;
}

void FlagButton::draw()
{
	if (!visible) return;
	if (isChecked || pressed)
	{
		if (specific_tex) painter.drawSquareFill(pos, size, painter.getBaseColor()*2, specific_tex);
		else painter.drawSquareFill(pos, size, painter.getBaseColor()*1.5);
	}
	else
	{
		if (specific_tex) painter.drawSquareFill(pos, size, painter.getBaseColor()*0.7, specific_tex);
		else painter.drawSquareFill(pos, size);
	}
	Button::draw();
}


//////////////////////////////////// Label /////////////////////////////////////
// Text label
////////////////////////////////////////////////////////////////////////////////

Label::Label(const wstring& _label, s_font * _font)
{
	if (_font) painter.setFont(_font);
	setLabel(_label);
	adjustSize();
}

Label::~Label()
{
}

void Label::setLabel(const wstring& _label)
{
	label = _label;
    adjustSize();
}

void Label::draw(void)
{
	if (!visible) return;

    if (painter.getFont()) 
		painter.print(pos[0], pos[1]+size[1], label);
}

void Label::draw(float _intensity)
{
	if (!visible) return;

    if (painter.getFont()) 
		painter.print(pos[0], pos[1]+size[1], label, painter.getTextColor()*_intensity);
}

void Label::adjustSize(void)
{
	size[0] = (int)ceilf(painter.getFont()->getStrLen(label));
	size[1] = (int)ceilf(painter.getFont()->getLineHeight());
}

////////////////////////////////////////////////////////////////////////////////
// History
////////////////////////////////////////////////////////////////////////////////

History::History(unsigned int _items)
{
	if (_items == 0) 
		maxItems = 1;
	else 
		maxItems = _items;
}

void History::clear(void)
{
    pos = 0;
	history.clear();
}

void History::add(const wstring& _text)
{
	if (_text == L"" || _text.empty()) return;
	
	if (history.size() == maxItems)
	{
		vector<wstring>::iterator iter = history.begin();
		history.erase(iter);
	}
	history.push_back(_text);
	pos = history.size();
}       

wstring History::prev(void)
{
    if (pos > 0) 
	{
		pos--;
	    return history[pos];
	}
	pos = -1;
	return L"";
}

wstring History::next(void)
{
	if (pos < (int)history.size()-1)  
	{
		pos++;
    	return history[pos];
	}
	pos = history.size();
	return L"";
}

////////////////////////////////////////////////////////////////////////////////
// AutoCompleteString
////////////////////////////////////////////////////////////////////////////////

AutoCompleteString::AutoCompleteString()
{
	maxMatches = 5;
}

wstring AutoCompleteString::test(const wstring& _text)
{
	matches.clear();

    if (options.size() == 0) return L"";
	
	unsigned int i = 0;
	while (i < options.size())
	{
		if (fcompare(options[i], _text) == 0) // match with item i
       		matches.push_back(options[i]);
		i++;
    }
         
	if (matches.empty()) 
		lastMatchPos = 0;
	else
    	lastMatchPos = _text.length();
    
	return getFirstMatch();;
}

void AutoCompleteString::reset(void)
{
	lastMatchPos = 0;
	matches.clear();
}

wstring AutoCompleteString::getOptions(int _number)
{
	if (_number == -1)
		_number = maxMatches;
		
    if (options.size() == 0) return L"";

	int i = 0;
	wstring text = L"";
	
	while (i < _number && i < (int)matches.size())
	{
       	if (text == L"") // first match
        	text += matches[i];
		else 
			text = text + L", " + matches[i];
		i++;
	}
	return text;
}


wstring AutoCompleteString::getFirstMatch(void)
{
	if (matches.size() > 0)
		return matches[0];
	else
		return L"";
}

///////////////////////////////// EditBox //////////////////////////////////////
// Standard Editbox component
////////////////////////////////////////////////////////////////////////////////

// we need to set this when actrivated, and only blink the active one

EditBox::EditBox(const wstring& _label, s_font* font) 
	: Button(), label(_label, font), isEditing(false), cursorPos(0)
{
	Component::setSize(label.getSize()+s_vec2i(4,2));
	text = _label;
	lastText = text;
    cursorVisible = false;
    setPrompt();
    type += CT_EDITBOX;
    autoFocus = true;
}

EditBox::~EditBox()
{
    SDL_SetTimer(0, NULL);
}

bool EditBox::onClic(int x, int y, S_GUI_VALUE bt, S_GUI_VALUE state)
{
	if (!visible) return false;
	if (state==S_GUI_PRESSED && bt==S_GUI_MOUSE_LEFT)
	{
		if (isIn(x, y)) 
        {
            setEditing(true);
            return true;
        }
		else if (isEditing) 
		{
            setEditing(false);
            return false;
         }
	}
	return Button::onClic(x,y,bt,state);
}

void EditBox::setColorScheme(const s_color& baseColor, const s_color& textColor)
{
	if (!GUIColorSchemeMember) return;
	label.setColorScheme(baseColor, textColor);
	Component::setColorScheme(baseColor, textColor);
}

void EditBox::draw(void)
{
    if (!visible) return;

	// may have been snatched away!
	if (isEditing && focusing && !focus)
		setEditing(false);
		
	Button::draw();
    glPushMatrix();
    glTranslatef(pos[0], pos[1], 0.f);
    Component::scissor->push(pos, size);
	label.setPos(6, (size[1]-label.getSizey())/2-1);

    refreshLabel();
	if (isEditing) label.draw(1.2);
    else label.draw();

    Component::scissor->pop();
	glPopMatrix();
}

#define EDITBOX_CURSOR L"|"
#define EDITBOX_DEFAULT_PROMPT L"\u00bb "
void EditBox::refreshLabel(void)
{
     if (!isEditing) label.setLabel(prompt + text);
     else
     {
        if (!cursorVisible) label.setLabel(prompt + text); 
        else
        {
            if (cursorPos == text.length()) // at end
               label.setLabel(prompt + text + EDITBOX_CURSOR); 
            else
               label.setLabel(prompt + text.substr(0,cursorPos) + EDITBOX_CURSOR + text.substr(cursorPos)); 
        }
     }
}

void EditBox::setText(const wstring &_text)
{
	text = _text;
	lastText = text;
    cursorPos = _text.length();
}

void EditBox::setVisible(bool _visible)
{
	label.setVisible(_visible);
	Component::setVisible(_visible);
	draw();
}


Uint32 toggleBlink(Uint32 interval)
{
     SDL_SetTimer(0, NULL);

     if (EditBox::cursorVisible == true)
        SDL_SetTimer(400, (SDL_TimerCallback)toggleBlink);
     else
        SDL_SetTimer(600, (SDL_TimerCallback)toggleBlink);
     
     EditBox::cursorVisible = !EditBox::cursorVisible;
     return interval;
}

void EditBox::setEditing(bool _editing)
{
	if (_editing)
	{
    	isEditing = true;
    	cursorVisible = true;
		autoComplete.reset();

		if (activeEditBox != this && activeEditBox != NULL)
			activeEditBox->setEditing(false);
		activeEditBox = this;

//    	SDL_SetTimer(0, NULL);
    	SDL_SetTimer(600, (SDL_TimerCallback)toggleBlink);
	}
	else
	{
		isEditing = false;
		cursorVisible = false;
		autoComplete.reset();
		activeEditBox = NULL;
    	SDL_SetTimer(0, NULL);
	}
}

void EditBox::setPrompt(const wstring &p)
{
	if (p.empty()) 
		prompt = EDITBOX_DEFAULT_PROMPT; 
	else 
		prompt = p; 
}

wstring EditBox::getDefaultPrompt(void)
{
	return EDITBOX_DEFAULT_PROMPT;
}

#define SDLK_A 65
#define SDLK_Z 90

bool EditBox::onKey(Uint16 k, S_GUI_VALUE s)
{
    if (!isEditing) return false;

	if (s==S_GUI_PRESSED)
    { 
		lastKey = k;
        if  (k==SDLK_RETURN)
        {
			if (autoComplete.hasMatch()) text = autoComplete.getFirstMatch();
            history.add(text);
       		if (!onReturnKeyCallback.empty()) RUNCALLBACK(onReturnKeyCallback);
            return 1;
        }

  		if (k == SDLK_TAB)
  		{
			if (autoComplete.hasMatch()) text = autoComplete.getFirstMatch();
		}
  		else if (k == SDLK_UP)
  		{
              text = history.prev();
              cursorPos = text.length();
        }
  		else if (k == SDLK_DOWN)
  		{
              text = history.next();
              cursorPos = text.length();
        }
  		else if (k == SDLK_LEFT)  		
  		{
            if (SDL_GetModState() & KMOD_CTRL)
                cursorToPrevWord();
            else
                if (cursorPos > 0) cursorPos--;
        }
        else if (k == SDLK_RIGHT)
        {
            if (SDL_GetModState() & KMOD_CTRL)
                cursorToNextWord();
            else
              if (cursorPos < text.length()) cursorPos++;
        }
        else if (k == SDLK_HOME) cursorPos = 0;
        else if (k == SDLK_END)  cursorPos = text.length();
  		else if (k == SDLK_DELETE)
  		{
           text = lastText;
           if (cursorPos < text.length()) text = text.erase(cursorPos, 1);
        }
        else if (k == SDLK_BACKSPACE)
        {
           text = lastText;
           if (cursorPos > 0)
           {
               cursorPos--;
               text = text.erase(cursorPos, 1);
           }
        }
        else if (k == SDLK_ESCAPE) setText(L"");
        else if ((k >= SDLK_0 && k <= SDLK_9) || (k >= SDLK_a && k <= SDLK_z) 
        || (k >= SDLK_A && k <= SDLK_Z) || (k >= 224 && k <= 255) 
		|| k == SDLK_SPACE || k == SDLK_UNDERSCORE)
        {
			text = lastText;
            wstring newtext = text.substr(0, cursorPos);
            newtext += k;
            newtext += text.substr(cursorPos);
            text = newtext;
            cursorPos++;
        }
        else
            return false;

		lastText = text;
		if (!onKeyCallback.empty()) RUNCALLBACK(onKeyCallback);
		if (!text.empty()) 
		{
			autoComplete.test(text);
			if (autoComplete.hasMatch())
			{
				text = autoComplete.getFirstMatch();
				if (!onAutoCompleteCallback.empty()) RUNCALLBACK(onAutoCompleteCallback);
			}
		}
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// EditBox::Cursor Previous and next words

void EditBox::cursorToNextWord(void)
{
    while (cursorPos < text.length())
    {
        cursorPos++;  
        if (text[cursorPos] == ' ')
        {
            if (cursorPos + 1 < text.length() && text[cursorPos+1] != ' ')
            {
               cursorPos++;
               break;
               }
        }
    }
}

void EditBox::cursorToPrevWord(void)
{
    while (cursorPos > 0)
    {
        cursorPos--;
        // cater for the cursor at the start of the word already 
        while (cursorPos > 0 && text[cursorPos] == ' ' && text[cursorPos-1] == ' ')
              cursorPos--;

        if (text[cursorPos] == ' ' && text[cursorPos+1] != ' ')
        {
            cursorPos++;
            break;
        }
    }
}

//////////////////////////////// ScrollBar /////////////////////////////////////
// ScrollBar
////////////////////////////////////////////////////////////////////////////////
#define SCROLL_SIZE 15

ScrollBar::ScrollBar(bool _vertical, int _totalElements, int _elementsForBar) 
	: dragging(false), value(0), firstElement(0), sized(false)

{
	vertical = _vertical;
	elements = _totalElements;
	elementsForBar = _elementsForBar;
}
	
void ScrollBar::draw(void)
{
    if (!visible) return;

	painter.drawSquareEdge(pos, size);
    glPushMatrix();
    glTranslatef(pos[0], pos[1], 0.f);
    Component::scissor->push(pos, size);

	if (!sized)
		adjustSize();
	scrollBt.draw();
    Component::scissor->pop();
	glPopMatrix();
}

bool ScrollBar::onClic(int x, int y, S_GUI_VALUE bt, S_GUI_VALUE state)
{
	if (bt==S_GUI_MOUSE_LEFT && state==S_GUI_RELEASED)
	{
		dragging = false;
		return false;
	}
	
	if (visible && isIn(x,y) && state==S_GUI_PRESSED)
	{
		if (scrollBt.isIn(x-pos[0],y-pos[1])) 
		{
			oldPos.set(x-pos[0],y-pos[1]);
			oldValue = value;
			dragging = true;
		}
		else
		{
			unsigned int n;

			if (vertical)
				n = y-pos[1];
			else
				n = x-pos[0];
				
			if (n < scrollOffset) setValue(value - 1);
			else if (n > scrollOffset + scrollSize) setValue(value +1);
			
		}
		return true;
	}
	return false;
}

bool ScrollBar::onMove(int x, int y)
{
	float delta, v;
	
	if (!visible) return false;

	scrollBt.onMove(x-pos[0],y-pos[1]);

	if (!isIn(x, y)) 
	{
		dragging = false;
		return false;
	}

	if (!dragging) return false;

	if (vertical)
		delta = ((float)(y - pos[1]) - oldPos[1])/(size[1]-(int)scrollSize);
	else
		delta = ((float)(x - pos[0]) - oldPos[0])/(size[0]-(int)scrollSize);

	v = (float)oldValue + delta*(elements-elementsForBar);
	
	if (v < 0) v = 0;
	else if (v > elements-elementsForBar) v = elements-elementsForBar;
	
	setValue(int(v));
	if (!onChangeCallback.empty()) RUNCALLBACK(onChangeCallback);

	return false;
}

void ScrollBar::setTotalElements(int _elements) 
{
	elements = _elements; 
	sized = false;
}

void ScrollBar::setElementsForBar(int _elementsForBar)
{
	if(_elementsForBar>(int)elements) 
		elementsForBar = elements; 
	else 
		elementsForBar =_elementsForBar; 
	sized = false; 
}

void ScrollBar::adjustSize(void)
{
	int s;
	
	if (vertical)
	{
		s = getSizey();
		setSize(SCROLL_SIZE,s);
	}
	else
	{
		s = getSizex();
		setSize(s,SCROLL_SIZE);
	}
	
	scrollSize = (int)(((float)elementsForBar/elements)*s)-2;
	scrollOffset = (int)(((float)firstElement/elements)*s)+1;
	
	if (value >= elements - elementsForBar && s-scrollSize-scrollOffset > 1)
		scrollOffset = s-scrollSize-1;
		
	if (vertical)
	{
		scrollBt.setSize(getSizex()-2, scrollSize);
		scrollBt.setPos(1, scrollOffset);
	}
	else
	{
		scrollBt.setSize(scrollSize, getSizey()-2);
		scrollBt.setPos(scrollOffset, 1);
	}
	sized = true;
}

void ScrollBar::setValue(int _value)
{
	value = _value;
	firstElement = value;
	if (!onChangeCallback.empty()) RUNCALLBACK(onChangeCallback);
	sized = false;
}


////////////////////////////////// ListBox /////////////////////////////////////
// ListBox
////////////////////////////////////////////////////////////////////////////////

#define LISTBOX_ITEM_HEIGHT (BUTTON_HEIGHT - 6)

ListBox::ListBox(int _displayLines) : 
	scrollBar(true), 
	firstItemIndex(0), 
	value(-1)
{
	displayLines = _displayLines;
	scrollBar.setVisible(false);
	scrollBar.setElementsForBar(displayLines);
}

void ListBox::createLines(void)
{
	unsigned int i;
	LabeledButton *bt;
	
    vector<LabeledButton*>::iterator iter = itemBt.begin();
    while (iter != itemBt.end())
    {
        delete (*iter);
        (*iter)=NULL;
        iter++;
    }
	itemBt.clear();
	
	for (i = 0; i < displayLines; i++)
	{
		bt = new LabeledButton(wstring()); 
		bt->setHideBorder(true);
		bt->setHideBorderMouseOver(true);
		bt->setHideTexture(true);
		bt->setJustification(JUSTIFY_LEFT);
		bt->adjustSize();
		bt->setVisible(visible);
		itemBt.push_back(bt);
	}
}

void ListBox::draw(void)
{
	unsigned int i, j;
	
	if (!visible) return;

	painter.drawSquareEdge(pos, size);
    glPushMatrix();
    glTranslatef(pos[0], pos[1], 0.f);
    Component::scissor->push(pos, size);

	if (scrollBar.getVisible()) scrollBar.draw();

	i = firstItemIndex;
	j = 0;
	while (i < firstItemIndex + displayLines && i < items.size())
	{
		if (value != -1 && value == (int)i)
		{
			itemBt[j]->setHideBorder(false);
			itemBt[j]->setHideBorderMouseOver(false);
		}
		else
		{
			itemBt[j]->setHideBorder(true);
			itemBt[j]->setHideBorderMouseOver(true);
		}
		itemBt[j++]->draw();
		i++;
	}
    Component::scissor->pop();
	glPopMatrix();
}

void ListBox::setCurrent(const wstring& ws)
{
	int i = firstItemIndex;
	vector<LabeledButton*>::iterator iter = itemBt.begin();
	while (iter != itemBt.end())
	{
		if ((*iter)->getLabel()==ws) 
		{
			value = i;
		}
		iter++;
		i++;
	}
}

bool ListBox::onClic(int x, int y, S_GUI_VALUE button, S_GUI_VALUE state)
{
	if (!visible) return false;
	if (!isIn(x,y)) return false;
	
	x = x - pos[0];
	y = y - pos[1];
	if (scrollBar.getVisible())
	{
		if (scrollBar.onClic(x, y, button, state)) return true;
	}

	if (state==S_GUI_PRESSED)
	{
		int i = firstItemIndex;
    	vector<LabeledButton*>::iterator iter = itemBt.begin();
		while (iter != itemBt.end())
		{
			if ((*iter)->onClic(x, y, button, state)) 
			{
				value = i;
				if (!onChangeCallback.empty()) RUNCALLBACK(onChangeCallback);
				return true;
			}
	   	    iter++;
	   	    i++;
		}
	}	

	return false;
}

bool ListBox::onMove(int x, int y)
{
	if (!visible) return false;
	x = x - pos[0];
	y = y - pos[1];
   	vector<LabeledButton*>::iterator iter = itemBt.begin();
	// highlight the item with the mouse over
	while (iter != itemBt.end())
	{
		(*iter)->onMove(x, y); 
   	    iter++;
	}
	if (scrollBar.onMove(x, y)) return true;
	
	return false;
}

void ListBox::setVisible(bool _visible)
{
    vector<LabeledButton*>::iterator iter = itemBt.begin();
    while (iter != itemBt.end())
    {
		(*iter)->setVisible(_visible);
        iter++;
    }
	Component::setVisible(_visible);
}

void ListBox::addItems(const vector<wstring> _items)
{
	if (_items.empty()) return;
	wstring item;
	
	unsigned int i = 0;
	while (i < _items.size())
	{
		item = _items[i];
		items.push_back(item);
		i++;
	}
	adjustAfterItemsAdded();
}

void ListBox::addItemList(const wstring& s)
{
	wistringstream is(s);
	wstring elem;
	while(getline(is, elem))
	{
		addItem(elem);
	}
}

void ListBox::addItem(const wstring& _text)
{
	if (!_text.empty())	items.push_back(_text);
	adjustAfterItemsAdded();
}

void ListBox::adjustAfterItemsAdded(void)
{
	createLines();
	setSize(getSizex(),LISTBOX_ITEM_HEIGHT * displayLines);
	scrollBar.setVisible(items.size() > displayLines);
	scrollBar.setOnChangeCallback(callback<void>(this, &ListBox::scrollChanged));
	scrollBar.setSize(SCROLL_SIZE,getSizey());
	scrollBar.setPos(getSizex()-SCROLL_SIZE,0);
	scrollBar.setTotalElements(items.size());
	scrollBar.setElementsForBar(displayLines);
	scrollChanged();
}

void ListBox::scrollChanged(void)
{
	firstItemIndex = scrollBar.getValue();
	unsigned int i = firstItemIndex;
	unsigned int j = 0;
	int w = getSizex();
	w = w - ((int)scrollBar.getVisible() * scrollBar.getSizex());
	while (i < firstItemIndex + displayLines && i < items.size())
	{
		itemBt[j]->setPos(0,j*LISTBOX_ITEM_HEIGHT);
		itemBt[j]->setSize(w,LISTBOX_ITEM_HEIGHT-1);
		itemBt[j++]->setLabel(items[i++]);
	}
}

void ListBox::clear(void)
{
	items.clear();
	adjustAfterItemsAdded();
}

wstring ListBox::getItem(int value)
{
	if (items.empty() || value < 0 || value >= (int)items.size()) 
		return wstring();	
	else
		return items[value];
}


/////////////////////////////// LabeledButton //////////////////////////////////
// Button with text on it
////////////////////////////////////////////////////////////////////////////////

#define LABEL_PAD 10

LabeledButton::LabeledButton(const wstring& _label, s_font* font, Justification _j, bool _bright) 
	: Button(), label(_label, font), justification(_j), isBright(_bright)
{
	Component::setSize(label.getSize()+s_vec2i(14,12+(int)label.getFont()->getDescent()));
	justify();
}

void LabeledButton::justify(void)
{
    if (justification == JUSTIFY_CENTER)
		label.setPos((size[0]-label.getSizex())/2,(size[1]-label.getSizey())/2-(int)label.getFont()->getDescent()+1);
	else if (justification == JUSTIFY_LEFT)
		label.setPos(0 + LABEL_PAD,(size[1]-label.getSizey())/2-(int)label.getFont()->getDescent()+1);
	else if (justification == JUSTIFY_RIGHT)
		label.setPos(size[0]-label.getSizex() - LABEL_PAD,(size[1]-label.getSizey())/2-(int)label.getFont()->getDescent()+1);	
}

void LabeledButton::adjustSize(void)
{
	Component::setSize(label.getSize()+s_vec2i(0,(int)label.getFont()->getDescent()));
	justify();
}

LabeledButton::~LabeledButton()
{
}

void LabeledButton::setColorScheme(const s_color& baseColor, const s_color& textColor)
{
	if (!GUIColorSchemeMember) return;
	label.setColorScheme(baseColor, textColor);
	Component::setColorScheme(baseColor, textColor);
}

void LabeledButton::draw(void)
{
    if (!visible) return;
	
	if (!hideTexture)
		painter.drawSquareFill(pos, size, painter.getBaseColor());
	Button::draw();

    glPushMatrix();
    glTranslatef(pos[0], pos[1], 0.f);
    Component::scissor->push(pos, size);
	
	if (pressed || isBright) label.draw(2.);
    else label.draw();   // faded

    Component::scissor->pop();
	glPopMatrix();
}

void LabeledButton::setVisible(bool _visible)
{
	label.setVisible(_visible);
	Component::setVisible(_visible);
}


////////////////////////////////// TextLabel ///////////////////////////////////
// A text bloc
////////////////////////////////////////////////////////////////////////////////

TextLabel::TextLabel(const wstring& _label, s_font* _font) :
	Container()
{
	if (_font) painter.setFont(_font);
	setLabel(_label);
	adjustSize();
}

TextLabel::~TextLabel()
{
}

void TextLabel::setLabel(const wstring& _label)
{
	label = _label;
    childs.clear();

    Label * tempLabel;
    wstring pch;

    unsigned int i = 0;
	unsigned int lineHeight = (int)painter.getFont()->getLineHeight()+1;

	wistringstream is(label);
    while (getline(is, pch))
    {
        tempLabel = new Label();
		tempLabel->setPainter(painter);
        tempLabel->setLabel(pch);
        tempLabel->setPos(0,i*lineHeight);
        addComponent(tempLabel);
        i++;
    }
    adjustSize();
}

void TextLabel::draw(void)
{
	Container::draw();
}

void TextLabel::adjustSize(void)
{
	unsigned int maxX = 0;
	unsigned int linelen;
	
	list<Component*>::iterator iter = childs.begin();
	while (iter != childs.end())
	{
		linelen = (*iter)->getSizex();	
		if (linelen > maxX) 
			maxX = linelen;
        iter++;
    }
	setSize(maxX,childs.size()*((int)painter.getFont()->getLineHeight()+1)+2);
}

void TextLabel::setTextColor(const s_color& c)
{
	list<Component*>::iterator iter = childs.begin();
	Component::setTextColor(c);
	while (iter != childs.end())
	{
		(*iter)->setTextColor(c);
        iter++;
    }
}


//////////////////////////// FramedContainer ///////////////////////////////////
// Container with a frame around it
////////////////////////////////////////////////////////////////////////////////

FramedContainer::FramedContainer() : Container(), frameSize(3,3,3,3)
{
	inside = new Container();
	inside->reshape(frameSize[0], frameSize[3], 10, 10);
	childs.push_front(inside);
}

void FramedContainer::draw(void)
{
    if (!visible) return;

	painter.drawSquareEdge(pos, size);
	painter.drawSquareEdge(inside->getPos()-s_vec2i(1,1) + pos, inside->getSize() + s_vec2i(2,2));

	Container::draw();
}

void FramedContainer::reshape(const s_vec2i& _pos, const s_vec2i& _size)
{
	pos = _pos;
	inside->setSize(_size);
	size = _size + s_vec2i( frameSize[0] + frameSize[1], frameSize[2] + frameSize[3]);
}

void FramedContainer::reshape(int x, int y, int w, int h)
{
	pos = s_vec2i(x, y);
	inside->setSize(s_vec2i(w,h));
	size = s_vec2i(w, h) + s_vec2i( frameSize[0] + frameSize[1], frameSize[2] + frameSize[3]);
}

void FramedContainer::setSize(const s_vec2i& _size)
{
	inside->setSize(_size);
	size = _size + s_vec2i( frameSize[0] + frameSize[1], frameSize[2] + frameSize[3]);
}

void FramedContainer::setSize(int w, int h)
{
	inside->setSize(s_vec2i(w,h));
	size = s_vec2i(w, h) + s_vec2i( frameSize[0] + frameSize[1], frameSize[2] + frameSize[3]);
}

void FramedContainer::setFrameSize(int left, int right, int bottom, int top)
{
	frameSize[0] = left;
	frameSize[1] = right;
	frameSize[2] = bottom;
	frameSize[3] = top;
	inside->setPos(left, top);
	size = inside->getSize() + s_vec2i( frameSize[0] + frameSize[1], frameSize[2] + frameSize[3]);
}


////////////////////////////////// StdWin //////////////////////////////////////
// Standard window widget
////////////////////////////////////////////////////////////////////////////////

StdWin::StdWin(const wstring& _title, const s_texture* _header_tex,
               s_font * _winfont, int headerSize) :
	FramedContainer(), titleLabel(NULL), header_tex(NULL), dragging(false)
{
	if (_header_tex) header_tex = _header_tex;
	if (_winfont) painter.setFont(_winfont);
	setFrameSize(1,1,1,headerSize);

	titleLabel = new Label(_title);
	titleLabel->adjustSize();

	Container::addComponent(titleLabel);
}

void StdWin::setTitle(const wstring& _title)
{
	titleLabel->setLabel(_title);
}

void StdWin::draw()
{
	if (!visible) return;

	titleLabel->setPos((size[0] - titleLabel->getSizex())/2, (frameSize[3]-titleLabel->getSizey())/2 - 1);
	painter.drawSquareFill(pos, size);
	painter.drawSquareFill(pos, s_vec2i(size[0], frameSize[3]));
	FramedContainer::draw();
}

bool StdWin::onClic(int x, int y, S_GUI_VALUE bt, S_GUI_VALUE state)
{
	if (!visible) return false;
	if (FramedContainer::onClic(x, y, bt, state)) return 1;
	if (state==S_GUI_RELEASED && bt==S_GUI_MOUSE_LEFT)
	{
		dragging = false;
	}
	if (isIn(x, y))
	{
		if (state==S_GUI_PRESSED && bt==S_GUI_MOUSE_LEFT)
		{
			dragging = true;
			oldPos.set(x,y);
		}
		return true;
	}
	return false;
}

bool StdWin::onMove(int x, int y)
{
	if (!visible) return false;
	if (FramedContainer::onMove(x, y)) return 1;
	if (dragging)
	{
		pos+=(s_vec2i(x,y)-oldPos);
		oldPos.set(x,y);
		return true;
	}
	return false;
}

void StdWin::setVisible(bool _visible)
{
	moveToFront = _visible;
	needNewTopEdit = true;
	Component::setVisible(_visible);
}

///////////////////////////////// StdBtWin //////////////////////////////////////
// Standard Button Window - StdWin with a close button in the title bar
////////////////////////////////////////////////////////////////////////////////

StdBtWin::StdBtWin(const wstring& _title, const s_texture* _header_tex,
                   s_font * _winfont, int headerSize) :
	StdWin(_title, _header_tex, _winfont, headerSize), hideBt(NULL)
{
	hideBt = new Button();
	hideBt->reshape(3,3,headerSize-6,headerSize-6);
	hideBt->setOnPressCallback(callback<void>(this, &StdBtWin::onHideBt));
	Container::addComponent(hideBt);
}

void StdBtWin::draw()
{
	if (!visible) return;

	hideBt->setPos(size[0] - hideBt->getSizex() - 3, 3);
	StdWin::draw();
}

void StdBtWin::onHideBt(void)
{
	setVisible(false);
	if (!onHideBtCallback.empty()) RUNCALLBACK(onHideBtCallback);
}


StdTransBtWin::StdTransBtWin(const wstring& _title, int _time_out,
                             const s_texture* _header_tex, s_font * _winfont,
                             int headerSize) :
	StdBtWin(_title, _header_tex, _winfont, headerSize)
{

	set_timeout(_time_out);
}

////////////////////////////// StdTranBtWin ////////////////////////////////////
// Standard Button Window with timed close
////////////////////////////////////////////////////////////////////////////////

void StdTransBtWin::update(int delta_time)
{
	if(timer_on) {
		time_left -= delta_time;
		if(time_left<=0) {
			visible=0;
			timer_on=0;
		}
	}

	//	printf("Time left: %d (%d)\n", time_left, timer_on);
}

// new timeout value
void StdTransBtWin::set_timeout(int _time_out) 
{
	if(_time_out==0) timer_on = 0;
	else timer_on = 1;

	time_left = _time_out;
}

///////////////////////////////// StdDlgWin ////////////////////////////////////
// Standard Button Window - StdWin with a close button in the title bar
////////////////////////////////////////////////////////////////////////////////

#define STDDLGWIN_BT_WIDTH 90
#define STDDLGWIN_BT_HEIGHT BUTTON_HEIGHT
#define STDDLGWIN_BT_BOTTOM_OFFSET 25
#define STDDLGWIN_MSG_SIDE_OFFSET 25
#define STDDLGWIN_MSG_TOP_OFFSET 10
#define STDDLGWIN_BT_BOTTOM_OFFSET 25
#define STDDLGWIN_BT_SEPARATION 25
#define STDDLGWIN_BT_ICON (32+15)
#define STDDLGWIN_BT_ICON_LEFT 20
#define STDDLGWIN_BT_ICON_TOP 20

StdDlgWin::StdDlgWin(const wstring& _title, const s_texture* _header_tex,
                     s_font * _winfont, int headerSize) :
	StdWin(_title, _header_tex, _winfont, headerSize), firstBt(NULL), secondBt(NULL), messageLabel(NULL), 
	inputEdit(NULL), hasIcon(false)
{
	reshape(300,200,400,100);

	numBtns = 1;
	firstBt = new LabeledButton(L"1");
	firstBt->setSize(STDDLGWIN_BT_WIDTH,STDDLGWIN_BT_HEIGHT);
	firstBt->setOnPressCallback(callback<void>(this, &StdDlgWin::onFirstBt));
	addComponent(firstBt);
	firstBt->setVisible(true);

	secondBt = new LabeledButton(L"2");
	secondBt->setSize(STDDLGWIN_BT_WIDTH,STDDLGWIN_BT_HEIGHT);
	secondBt->setOnPressCallback(callback<void>(this, &StdDlgWin::onSecondBt));
	addComponent(secondBt);

	blankIcon = new s_texture("bt_blank.png");
	questionIcon = new s_texture("bt_question.png");
	alertIcon = new s_texture("bt_alert.png");
	picture = new Picture(questionIcon, STDDLGWIN_BT_ICON_LEFT, STDDLGWIN_BT_ICON_TOP, 32, 32);
	addComponent(picture);

	messageLabel = new TextLabel();
	messageLabel->setPos(STDDLGWIN_MSG_SIDE_OFFSET,STDDLGWIN_MSG_TOP_OFFSET);
	addComponent(messageLabel);
	
	inputEdit = new EditBox();
	inputEdit->setPos(STDDLGWIN_MSG_SIDE_OFFSET,STDDLGWIN_MSG_TOP_OFFSET + 20);
	inputEdit->setSize(size[0] - 2*STDDLGWIN_MSG_SIDE_OFFSET, STDDLGWIN_BT_HEIGHT);
	inputEdit->setVisible(false);
	inputEdit->setOnReturnKeyCallback(callback<void>(this, &StdDlgWin::onInputReturnKey));
	addComponent(inputEdit);
	
	arrangeButtons();
	originalTitle = _title;
	setVisible(false);
		
	resetResponse();
}

StdDlgWin::~StdDlgWin()
{
	delete blankIcon;
	delete alertIcon;
}

void StdDlgWin::resetResponse(void)
{
	lastButton = BT_NOTSET;
	lastInput = L"";
	firstBtType = BT_NOTSET;	
	secondBtType = BT_NOTSET;	
}

void StdDlgWin::arrangeButtons(void)
{
	int firstLeft, secondLeft, top;
	
	if (numBtns == 2) firstLeft = (size[0] - (2*STDDLGWIN_BT_WIDTH + STDDLGWIN_BT_SEPARATION)) / 2;
	else firstLeft = (size[0] - STDDLGWIN_BT_WIDTH) / 2;
	secondLeft = firstLeft + STDDLGWIN_BT_WIDTH + STDDLGWIN_BT_SEPARATION;
	top = size[1] - STDDLGWIN_BT_BOTTOM_OFFSET - STDDLGWIN_BT_HEIGHT;

	if (hasIcon)
	{
		messageLabel->setPos(STDDLGWIN_MSG_SIDE_OFFSET + STDDLGWIN_BT_ICON, STDDLGWIN_MSG_TOP_OFFSET);
		inputEdit->setPos(STDDLGWIN_MSG_SIDE_OFFSET + STDDLGWIN_BT_ICON,STDDLGWIN_MSG_TOP_OFFSET + 20);
	}
	else
	{
		messageLabel->setPos(STDDLGWIN_MSG_SIDE_OFFSET,STDDLGWIN_MSG_TOP_OFFSET);
		inputEdit->setPos(STDDLGWIN_MSG_SIDE_OFFSET,STDDLGWIN_MSG_TOP_OFFSET + 20);
	}

	firstBt->setPos(firstLeft,top);
	secondBt->setPos(secondLeft,top);
	secondBt->setVisible((numBtns > 1));
	
	picture->setVisible(hasIcon);
}

void StdDlgWin::MessageBox(const wstring &_title, const wstring &_prompt, int _buttons, const string &_ID)
{
	lastID = _ID;
	lastType = STDDLGWIN_MSG;
	
	resetResponse();

	numBtns = 1;

	if (!_title.empty()) setTitle(_title); 
	else setTitle(originalTitle);

	messageLabel->setLabel(_prompt);
	inputEdit->setVisible(false);	

	if (_buttons & BT_NO)
	{
		secondBtType = BT_NO;
		secondBt->setLabel(_("No"));
		numBtns = 2;
	}
	else if (_buttons & BT_CANCEL)
	{
		secondBtType = BT_CANCEL;
		secondBt->setLabel(_("Cancel"));
		numBtns = 2;
	}

	hasIcon = (_buttons >= BT_ICON_BLANK);
	
	if (_buttons & BT_ICON_BLANK) picture->setTexture(blankIcon);
	else if (_buttons & BT_ICON_ALERT) picture->setTexture(alertIcon);
	else if (_buttons & BT_ICON_QUESTION) picture->setTexture(questionIcon);

	if (_buttons & BT_YES)
	{
		firstBtType = BT_YES;
		firstBt->setLabel(_("Yes"));
	}
	else
	{
		firstBtType = BT_OK;
		firstBt->setLabel(_("OK"));
	}		
	
	arrangeButtons();
	setFocus(true);
	setVisible(true);
}

void StdDlgWin::InputBox(const wstring &_title, const wstring &_prompt, const string &_ID)
{
	lastID = _ID;
	lastType = STDDLGWIN_INPUT;

	resetResponse();

	numBtns = 2;
	hasIcon = false;

	if (!_title.empty()) setTitle(_title.c_str());
	else setTitle(originalTitle.c_str()); 

	messageLabel->setLabel(_prompt);
	inputEdit->clearText();
	inputEdit->setVisible(true);	

	firstBtType = BT_OK;
	firstBt->setLabel(_("OK"));
	secondBtType = BT_CANCEL;
	secondBt->setLabel(_("Cancel"));

	arrangeButtons();
	setVisible(true);
	setFocus(true);
	inputEdit->setEditing(true);
}

void StdDlgWin::onInputReturnKey(void)
{
	inputEdit->setEditing(false);
	onFirstBt();
}
	
void StdDlgWin::onFirstBt(void)
{
	lastButton = firstBtType;
	lastInput = inputEdit->getText();
	setVisible(false);
	setFocus(false);
	if (!onCompleteCallback.empty()) RUNCALLBACK(onCompleteCallback);
}

void StdDlgWin::onSecondBt(void)
{
	lastButton = secondBtType;
	lastInput = inputEdit->getText();
	setVisible(false);
	setFocus(false);
	if (!onCompleteCallback.empty()) RUNCALLBACK(onCompleteCallback);
}

///////////////////////////////////// Tab //////////////////////////////////////
// Everything to handle tabs
////////////////////////////////////////////////////////////////////////////////

TabHeader::TabHeader(Component* c, const wstring& _label, s_font* _font) :
	LabeledButton(_label, _font), assoc(c), active(false)
{
}

void TabHeader::draw(void)
{
    if (!visible) return;

	painter.drawSquareFill(pos, size, painter.getBaseColor() * (0.7f + 0.3 * active));
	if (!active) painter.drawSquareEdge(pos,size);
	else
	{
		painter.drawLine(pos, s_vec2i(pos[0],pos[1]+size[1]));
		painter.drawLine(s_vec2i(pos[0]+1,pos[1]), s_vec2i(pos[0]+size[0]-1,pos[1]));
		painter.drawLine(s_vec2i(pos[0]+size[0]-1,pos[1]), pos+size-s_vec2i(1,0));
	}

	// Draw label
    glPushMatrix();
    glTranslatef(pos[0], pos[1], 0.f);
    Component::scissor->push(pos, size);
	
	label.setPos((size-label.getSize())/2 + s_vec2i(-1,-1));
	label.draw(0.7f + 0.3 * active);

    Component::scissor->pop();
	glPopMatrix();
}

void TabHeader::setActive(bool s)
{
	active = s;
	assoc->setVisible(active);
}

TabContainer::TabContainer(s_font* _font) : Container(), headerHeight(22)
{
	if (_font) painter.setFont(_font);
}

void TabContainer::addTab(Component* c, const wstring& name)
{
	Container* tempInside = new Container();
	tempInside->reshape(pos[0], pos[1]+headerHeight, size[0], size[1]-headerHeight);
	tempInside->addComponent(c);

	TabHeader* tempHead = new TabHeader(tempInside, name);
	tempHead->setPainter(painter);
	tempHead->reshape(getHeadersSize(),0,tempHead->getSizex(),headerHeight);
	headers.push_front(tempHead);

	addComponent(tempInside);
	addComponent(tempHead);
	select(tempHead);
}

void TabContainer::draw(void)
{
	if (!visible) return;

	painter.drawSquareFill(pos, size, painter.getBaseColor()/3);
	painter.drawLine(s_vec2i(pos[0]+getHeadersSize(), pos[1]+headerHeight-1),
		s_vec2i(pos[0]+size[0], pos[1]+headerHeight-1));

	Container::draw();
}

void TabContainer::setColorScheme(const s_color& baseColor, const s_color& textColor)
{
	if (!GUIColorSchemeMember) return;

	list<TabHeader*>::iterator iter = headers.begin();
	while (iter != headers.end())
	{
		(*iter)->setColorScheme(baseColor, textColor);
		iter++;
	}
	Container::setColorScheme(baseColor, textColor);
}

int TabContainer::getHeadersSize(void)
{
	int s = 0;
	list<TabHeader*>::iterator iter = headers.begin();
	while (iter != headers.end())
	{
		s+=(*iter)->getSizex();
        iter++;
    }
	return s;
}

bool TabContainer::onClic(int x, int y, S_GUI_VALUE button, S_GUI_VALUE state)
{
	if (!visible) return false;
	list<TabHeader*>::iterator iter = headers.begin();
	while (iter != headers.end())
	{
		if ((*iter)->onClic(x - pos[0], y - pos[1], button, state))
		{
			select(*iter);
			(*iter)->setInFront(true);
			return true;
		}
        iter++;
    }
	return Container::onClic(x, y, button, state);
}

void TabContainer::select(TabHeader* t)
{
	list<Component*>::iterator iter = childs.begin();
	while (iter != childs.end())
	{
		(*iter)->setVisible(false);
        iter++;
    }
	list<TabHeader*>::iterator iter2 = headers.begin();
	while (iter2 != headers.end())
	{
		(*iter2)->setVisible(true);
		if (*iter2==t) (*iter2)->setActive(true);
		else (*iter2)->setActive(false);
        iter2++;
    }
}

//////////////////////////////// CursorBar /////////////////////////////////////
// CursorBar
////////////////////////////////////////////////////////////////////////////////
#define CURSOR_SIZE 8
CursorBar::CursorBar(bool _vertical, float _min, float _max, float _val) : cursorBt(), sized(false)
{
	float tmpVal = _val;
	minValue = _min;
	maxValue = _max;
	range = maxValue - minValue;
	vertical = _vertical;

	if (tmpVal < minValue || tmpVal > maxValue) tmpVal = (minValue + maxValue)/2;
	setValue(tmpVal);
}

void CursorBar::draw(void)
{
    if (!visible) return;

	painter.drawSquareEdge(pos, size);
    glPushMatrix();
    glTranslatef(pos[0], pos[1], 0.f);
    Component::scissor->push(pos, size);

	if (!sized)
		adjustSize();

	cursorBt.draw();
    Component::scissor->pop();
	glPopMatrix();
	if (dragging)
	{
		ostringstream oss;
		oss.precision(2);
		oss << value;
		painter.print(pos[0]+2, pos[1]+2, oss.str());
	}
}

void CursorBar::adjustSize(void)
{
	int s, scrollOffset;
	
	if (vertical)
	{
		s = getSizey();
		setSize(SCROLL_SIZE,s);
	}
	else
	{
		s = getSizex();
		setSize(s,SCROLL_SIZE);
	}
	s = s - CURSOR_SIZE;
	
	scrollOffset = (int)((float)(value-minValue)/range*s)+1;
	
	if (value >= minValue + range)
		scrollOffset = s-1;
		
	if (vertical)
	{
		cursorBt.setSize(getSizex()-2, CURSOR_SIZE);
		cursorBt.setPos(1, scrollOffset);
	}
	else
	{
		cursorBt.setSize(CURSOR_SIZE, getSizey()-2);
		cursorBt.setPos(scrollOffset, 1);
	}
	sized = true;
}

void CursorBar::setValue(float _value)
{
	value = _value;
	sized = false;
}

bool CursorBar::onClic(int x, int y, S_GUI_VALUE bt, S_GUI_VALUE state)
{
	if (bt==S_GUI_MOUSE_LEFT && state==S_GUI_RELEASED)
	{
		dragging = false;
		return false;
	}
	
	if (visible && isIn(x,y) && state==S_GUI_PRESSED)
	{
		if (cursorBt.isIn(x-pos[0],y-pos[1])) 
		{
			oldPos.set(x-pos[0],y-pos[1]);
			oldValue = value;
			dragging = true;
		}
		return true;
	}
	return false;
}

bool CursorBar::onMove(int x, int y)
{
	float delta, v;
	
	if (!visible) return false;
	
	cursorBt.onMove(x-pos[0],y-pos[1]);
	
	if (!isIn(x, y)) 
	{
		dragging = false;
		return false;
	}

	if (!dragging) return false;

	if (vertical)
		delta = ((float)(y - pos[1]) - oldPos[1])/(size[1] - CURSOR_SIZE);
	else
		delta = ((float)(x - pos[0]) - oldPos[0])/(size[0] - CURSOR_SIZE);

	v = (float)oldValue + delta*range;
	
	if (v < minValue) v = minValue; 
	else if (v > maxValue) v = maxValue;
	
	setValue(v);
	if (!onChangeCallback.empty()) RUNCALLBACK(onChangeCallback);

	return false;
}

//////////////////////////////// IntIncDec /////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////

IntIncDec::IntIncDec(s_font* _font, const s_texture* tex_up,
		const s_texture* tex_down, int _min, int _max,
		int _init_value, int _inc, bool _loop) :
	Container(), value(_init_value), min(_min), max(_max), inc(_inc), btmore(NULL), btless(NULL), label(NULL), loop(_loop)
{
	label = new Label();
	if (_font) label->setFont(_font);
	label->setSize(30,10);
	label->setPos(9,0);
	btmore = new TexturedButton(tex_up);
	btmore->setSize(8,8);
	btmore->setPos(0,0);
	btmore->setBaseColor(painter.getTextColor());
	btmore->setOnPressCallback(callback<void>(this, &IntIncDec::inc_value));
	btless = new TexturedButton(tex_down);
	btless->setSize(8,8);
	btless->setPos(0,7);
	btless->setBaseColor(painter.getTextColor());
	btless->setOnPressCallback(callback<void>(this, &IntIncDec::dec_value));
	addComponent(btmore);
	addComponent(btless);
	addComponent(label);
	setSize(40,20);
}

IntIncDec::~IntIncDec()
{
}

void IntIncDec::draw()
{
	if (!visible) return;
	label->setLabel(StelUtility::doubleToWstring(value));
	Container::draw();
}

void IntIncDec::inc_value()
{
	value+=inc;
	if(value>max)
	{
		if (loop) value=min;
		else value=max;
	}
	if (!onPressCallback.empty()) onPressCallback();
}

void IntIncDec::dec_value()
{
	value-=inc;
	if(value<min)
	{
		if (loop) value=max;
		else value=min;
	} 
	if (!onPressCallback.empty()) onPressCallback();
}
		
IntIncDecVert::IntIncDecVert(s_font* _font, const s_texture* tex_up,
		const s_texture* tex_down, int _min, int _max,
		int _init_value, int _inc, bool _loop) : IntIncDec(_font, tex_up, tex_down, _min, _max, _init_value, _inc, loop)
{
	label->setPos(0,3);
	btmore->setPos(_max/10 * 8 + 8, 0);
	btless->setPos(_max/10 * 8 + 8, 8);
	setSize(_max/10 * 8 + 16,40);
}

FloatIncDec::FloatIncDec(s_font* _font, const s_texture* tex_up,
		const s_texture* tex_down, float _min, float _max,
		float _init_value, float _inc) :
	Container(), value(_init_value), min(_min), max(_max), inc(_inc), btmore(NULL), 
	btless(NULL), label(NULL), format(FORMAT_DEFAULT)
{
	label = new Label;
	if (_font) label->setFont(_font);
	label->setSize(30,10);
	label->setPos(9,0);
	btless = new TexturedButton(tex_up);
	btless->setSize(8,8);
	btless->setPos(0,0);
	btless->setBaseColor(painter.getTextColor());
	btless->setOnPressCallback(callback<void>(this, &FloatIncDec::inc_value));
	btmore = new TexturedButton(tex_down);
	btmore->setSize(8,8);
	btmore->setPos(0,7);
	btmore->setBaseColor(painter.getTextColor());
	btmore->setOnPressCallback(callback<void>(this, &FloatIncDec::dec_value));
	addComponent(btmore);
	addComponent(btless);
	addComponent(label);
	setSize(50,20);
}

FloatIncDec::~FloatIncDec()
{
}

void FloatIncDec::draw()
{
	if (!visible) return;
	
	if (format == FORMAT_DEFAULT)
	{
		label->setLabel(StelUtility::doubleToWstring(value));
	}
	else if (format == FORMAT_LONGITUDE || format == FORMAT_LATITUDE)
	{
		wstring l = StelUtility::printAngleDMS(value*M_PI/180.);
		wstring m = l.substr(1);
		if (format == FORMAT_LATITUDE)
		{
			if (l[0] == '+') m += _("N");
			if (l[0] == '-') m += _("S");
		}
		else
		{
			if (l[0] == '+') m += _("E");
			if (l[0] == '-') m += _("W");
		}
		label->setLabel(m); 
	}

	Container::draw();
}

void FloatIncDec::inc_value() 
{
	float v = value;
	
	if (format == FORMAT_LONGITUDE || format == FORMAT_LATITUDE)
	{
		v = (int)(v * 60);
		v = v / 60;
	}
	value = v;
	value += inc; 
	if(value > max) 
		value = max; 
	if (!onPressCallback.empty()) RUNCALLBACK(onPressCallback);
}

void FloatIncDec::dec_value() 
{
	float v = value;
	
	if (format == FORMAT_LONGITUDE || format == FORMAT_LATITUDE)
	{
		v = (int)(v * 60);
		v = v / 60;
	}
	value = v;
	value -= inc; 
	if(value < min) 
		value = min; 
	if (!onPressCallback.empty()) RUNCALLBACK(onPressCallback);
}

/////////////////////////////// Time_item //////////////////////////////////////
// Widget used to set time and date.
////////////////////////////////////////////////////////////////////////////////

Time_item::Time_item(s_font* _font, const s_texture* tex_up,
						const s_texture* tex_down, double _JD) :
						d(NULL), m(NULL), y(NULL), h(NULL), mn(NULL), s(NULL)
{
	if (_font) setFont(_font);

	// ranges are +1 and -1 from normal to allow rollover
	y = new IntIncDec(getFont(), tex_up, tex_down, -9999, 99999, 1930, 1);
	m = new IntIncDec(getFont(), tex_up, tex_down, 1, 12, 12, 1, true);
	d = new IntIncDec(getFont(), tex_up, tex_down, 1, 31, 11, 1, true);
	h = new IntIncDec(getFont(), tex_up, tex_down, 0, 23, 16, 1, true);
	mn= new IntIncDec(getFont(), tex_up, tex_down, 0, 59, 35, 1, true);
	s = new IntIncDec(getFont(), tex_up, tex_down, 0, 59, 23, 1, true);

	y->setOnPressCallback(callback<void>(this, &Time_item::onTimeChange));
	m->setOnPressCallback(callback<void>(this, &Time_item::onTimeChange));
	d->setOnPressCallback(callback<void>(this, &Time_item::onTimeChange));
	h->setOnPressCallback(callback<void>(this, &Time_item::onTimeChange));
	mn->setOnPressCallback(callback<void>(this, &Time_item::onTimeChange));
	s->setOnPressCallback(callback<void>(this, &Time_item::onTimeChange));

	Label* l1 = new Label(_("Year")); l1->setPos(5,5);
	y->setPos(50,6); y->setSize(50, 32);

	Label* l2 = new Label(_("Month")); l2->setPos(5,25);
	m->setPos(50,26); m->setSize(50, 32);

	Label* l3 = new Label(_("Day")); l3->setPos(5,45);
	d->setPos(50,44); d->setSize(50, 32);

	Label* l4 = new Label(_("Hour")); l4->setPos(130,5);
	h->setPos(190,6); h->setSize(50, 32);

	Label* l5 = new Label(_("Minutes")); l5->setPos(130,25);
	mn->setPos(190,26);mn->setSize(50, 32);

	Label* l6 = new Label(_("Seconds")); l6->setPos(130,45);
	s->setPos(190,46); s->setSize(50, 32);

	setSize(230, 65);

	addComponent(y);
	addComponent(m);
	addComponent(d);
	addComponent(h);
	addComponent(mn);
	addComponent(s);

	addComponent(l1);
	addComponent(l2);
	addComponent(l3);
	addComponent(l4);
	addComponent(l5);
	addComponent(l6);

	setJDay(_JD);
}

double Time_item::getJDay(void) const
{
	static int iy, im, id, ih, imn, is;

	iy = (int)y->getValue();
	im = (int)m->getValue();
	id = (int)d->getValue();
	ih = (int)h->getValue();
	imn = (int)mn->getValue();
	is = (int)s->getValue();

    if (im <= 2)
    {
        iy -= 1;
        im += 12;
    }

    // Correct for the lost days in Oct 1582 when the Gregorian calendar
    // replaced the Julian calendar.
    int B = -2;
    if (iy > 1582 || (iy == 1582 && (im > 10 || (im == 10 && id >= 15))))
    {
        B = iy / 400 - iy / 100;
    }

    return floor(365.25 * iy) + floor(30.6001 * (im + 1)) + B + 1720996.5 + id + ih / 24.0 + imn / 1440.0 + (double)is / 86400.0;
}

// for use with commands - no special characters, just the local date
string Time_item::getDateString(void)
{
	ostringstream os;
	os << y->getValue() << ":"
	   << m->getValue() << ":"
	   << d->getValue() << "T"
	   << h->getValue() << ":"
	   << mn->getValue() << ":"
	   << s->getValue();
	return os.str();
}

void Time_item::setJDay(double JD)
{
	static int iy, im, id, ih, imn, is;

    int a = (int) (JD + 0.5);
    double c;
    if (a < 2299161)
    {
        c = a + 1524;
    }
    else
    {
        double b = (int) ((a - 1867216.25) / 36524.25);
        c = a + b - (int) (b / 4) + 1525;
    }

    int dd = (int) ((c - 122.1) / 365.25);
    int e = (int) (365.25 * dd);
    int f = (int) ((c - e) / 30.6001);

    double dday = c - e - (int) (30.6001 * f) + ((JD + 0.5) - (int) (JD + 0.5));

    im = f - 1 - 12 * (int) (f / 14);
    iy = dd - 4715 - (int) ((7.0 + im) / 10.0);
    id = (int) dday;

    double dhour = (dday - id) * 24;
    ih = (int) dhour;

    double dminute = (dhour - ih) * 60;
    imn = (int) dminute;

    is = S_ROUND((dminute - imn) * 60);

	y->setValue(iy);
	m->setValue(im);
	d->setValue(id);
	h->setValue(ih);
	mn->setValue(imn);
	s->setValue(S_ROUND(is));
}

void Time_item::draw()
{
    if (!visible) return;
	painter.drawSquareEdge(pos, size);
	painter.drawSquareFill(pos, size);
	Container::draw();
}

void Time_item::onTimeChange(void)
{
	if (!onChangeTimeCallback.empty()) RUNCALLBACK(onChangeTimeCallback);
}

//////////////////////////////// Picture  //////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////

Picture::Picture(const s_texture * _imageTex,
                 int xpos, int ypos, int xsize, int ysize) :
	imageTex(_imageTex), imgcolor(s_color(1,1,1))
{
	setPos(xpos, ypos);
	setSize(xsize, ysize);
	setTexture(imageTex);
	showedges = false;
}

Picture::~Picture()
{
	if (imageTex) delete imageTex;
	imageTex = NULL;
}

void Picture::draw(void)
{
	if (!visible) return;

	painter.drawSquareFill(pos, size, imgcolor);
	if (showedges) painter.drawSquareEdge(pos, size);
}


///////////////////////////// MapPicture  //////////////////////////////////////
// MapPicture - for selecting the current location
////////////////////////////////////////////////////////////////////////////////

City::City(const string& _name, const string& _state, const string& _country, 
	double _longitude, double _latitude, float zone, int _showatzoom, int _altitude) :
	name(_name), state(_state), country(_country), latitude(_latitude), longitude(_longitude), showatzoom(_showatzoom), altitude(_altitude)
{
}

City_Mgr::City_Mgr(double _proximity) : proximity(_proximity)
{
}

City_Mgr::~City_Mgr()
{
	vector<City*>::iterator iter;
	for (iter = cities.begin();iter!=cities.end();iter++)
	{
		delete (*iter);
	}
}

void City_Mgr::addCity(const string& _name, const string& _state, 
	const string& _country, double _longitude, double _latitude, float _zone, int _showatzoom, int _altitude)
{
	City *city = new City(_name, _state, _country, _longitude, _latitude, _zone, _showatzoom, _altitude);
	cities.push_back(city);
}

int City_Mgr::getNearest(double _longitude, double _latitude)
{
	double dist, closest=10000.;
	int index = -1;
	int i;
	string name;

	if (cities.size() == 0) return -1;
	
	i = 0;
    while (i < (int)cities.size())
    {
		name = cities[i]->getName();
		dist = powf(_latitude - cities[i]->getLatitude(),2.f) +
			powf(_longitude - cities[i]->getLongitude(),2.f);
		dist = powf(dist,0.5f);
		if (index == -1) 
		{
			closest = dist;
			index = i;
		}
		else if (dist < closest) 
		{
			closest = dist;
			index = i;
		}
	    i++;
    }
    if (closest < proximity)
	    return index;
	else 
		return -1;
}

City *City_Mgr::getCity(unsigned int _index)
{
	if (_index < cities.size())
		return cities[_index];
	else
		return NULL;
}

void City_Mgr::setProximity(double _proximity)
{
	if (_proximity < 0)
		proximity = CITIES_PROXIMITY;
	else
		proximity = _proximity;
}


///////////////////////////// MapPicture  //////////////////////////////////////
// MapPicture - for selecting the current location
////////////////////////////////////////////////////////////////////////////////
#define POINTER_SIZE 10
#define CITY_SIZE 5
#define CITY_SELECT s_color(1,0,0)
#define CITY_HOVER s_color(1,1,0)
#define CITY_WITH_NAME s_color(1,.4,0)
#define CITY_WITHOUT_NAME s_color(.8,.6,0)

MapPicture::MapPicture(const s_texture *_imageTex,
                       const s_texture *_pointerTex,
                       const s_texture *_cityTex,
                       int xpos, int ypos, int xsize, int ysize) :
	Picture(_imageTex, xpos, ypos, xsize, ysize), pointer(NULL), panning(false), 
	dragging(false), zoom(1.f), sized(false), exact(false)
{
	pointer = new Picture(_pointerTex, 0, 0, POINTER_SIZE, POINTER_SIZE);
	pointer->setImgColor(CITY_SELECT);
	
	cityPointer = new Picture(_cityTex, 0, 0, CITY_SIZE, CITY_SIZE);
	cityPointer->setImgColor(CITY_HOVER);
	
	setShowEdge(false);  // draw our own
	cities.setProximity(30.f);
	nearestIndex = -1;
	pointerIndex = -1;
	city_name_font = NULL;
}

MapPicture::~MapPicture()
{
	delete pointer;
	pointer = NULL;

	if (city_name_font) delete city_name_font;
	city_name_font = NULL;
	
	delete cityPointer;
}

void MapPicture::set_font(float font_size, const string& font_name)
{
	city_name_font = new s_font(font_size, font_name);
	if (!city_name_font)
	{
		printf("Can't create city_name_font\n");
		exit(-1);
	}
	fontsize = font_size;
}

#define CITY_TYPE_NAMED 1
#define CITY_TYPE_UNNAMED 2
#define CITY_TYPE_HOVER 3
void MapPicture::drawCity(const s_vec2i& cityPos, int ctype)
{
	if (zoom == 1) 
		cityPointer->setSize((int)zoom, (int)zoom);
	else
		cityPointer->setSize((int)zoom/3, (int)zoom/3);

	if (ctype == CITY_TYPE_HOVER)
		cityPointer->setImgColor(CITY_HOVER);
	else if (ctype == CITY_TYPE_NAMED)
		cityPointer->setImgColor(CITY_WITH_NAME);
	else
		cityPointer->setImgColor(CITY_WITHOUT_NAME);

	cityPointer->setPos(cityPos[0]-cityPointer->getSizex()/2, cityPos[1]-cityPointer->getSizey()/2);
	cityPointer->draw();
}

void MapPicture::drawCityName(const s_vec2i& cityPos, const wstring& _name)
{
	int x, y, strLen;

	// don't draw if not in the current view (ok in the y axis!)
	if ((cityPos[0] > originalPos[0] + originalSize[0]) || (cityPos[0] < originalPos[0]))
		return;
		
	y = cityPos[1]-(int)(fontsize/2);
	strLen = (int)city_name_font->getStrLen(_name);
	x = cityPos[0] + cityPointer->getSizex()/2 + 1;
	if (x + strLen + 1 >= originalPos[0] + originalSize[0])
		x = cityPos[0] - cityPointer->getSizex()/2 - 1 - strLen;
	city_name_font->print(x, y, _name, 0);
}

void MapPicture::drawCities(void)
{
	s_vec2i cityPos;
	int cityzoom;
	
	for (unsigned int i=0; i < cities.size(); i++)
	{
		cityzoom = cities.getCity(i)->getShowAtZoom();
		cityPos[0] = pos[0] + getxFromLongitude(cities.getCity(i)->getLongitude());
		cityPos[1] = pos[1] + getyFromLatitude(cities.getCity(i)->getLatitude());
		// draw the city name if at least at that zoom level, or it is a selected city
		if (cityzoom != 0 && cityzoom <= (int)zoom || (pointerIndex != -1 && (int)i == pointerIndex))
		{
			drawCity(cityPos, CITY_TYPE_NAMED);
			if ((int)i == pointerIndex)
				glColor3fv(CITY_SELECT);
			else
				glColor3fv(CITY_WITH_NAME);
			drawCityName(cityPos, cities.getCity(i)->getNameI18());
		}
		else 
			drawCity(cityPos, CITY_TYPE_UNNAMED);
	}
}

void MapPicture::drawNearestCity(void)
{
	s_vec2i cityPos;
	int lastIndex = nearestIndex;
	
	if ((dragging && nearestIndex == pointerIndex && nearestIndex != -1) ||
		(!dragging && nearestIndex != -1))
	{
		cityPos[0] = pos[0] + getxFromLongitude(cities.getCity(nearestIndex)->getLongitude());
		cityPos[1] = pos[1] + getyFromLatitude(cities.getCity(nearestIndex)->getLatitude());
		cityPointer->setImgColor(CITY_HOVER);
		drawCity(cityPos, CITY_TYPE_HOVER);
		glColor3fv(CITY_HOVER);
		drawCityName(cityPos, cities.getCity(nearestIndex)->getNameI18());
		if (!onNearestCityCallback.empty()) RUNCALLBACK(onNearestCityCallback);
		return;
	}
	
	// if dragging - leave the city along
	if (dragging)
	{
		nearestIndex = -1;
		if (!onNearestCityCallback.empty()) RUNCALLBACK(onNearestCityCallback);
		return;
	}
	
	if (lastIndex != -1 && nearestIndex == -1)
		if (!onNearestCityCallback.empty()) RUNCALLBACK(onNearestCityCallback);
}

void MapPicture::draw(void)
{
	if (!visible) return;

	if (!sized)
	{
		originalSize = size;
		originalPos = pos;
		sized = true;
	}
	
    glPushMatrix();
    Component::scissor->push(originalPos, originalSize);

	Picture::draw();

	drawCities();
	drawNearestCity();
	
	if (zoom == 1) 
		pointer->setSize((int)zoom*6, (int)zoom*6);
	else
		pointer->setSize((int)(zoom*2), (int)(zoom*2));
	pointer->setPos(s_vec2i(pointerPos[0]+pos[0]-pointer->getSizex()/2, pointerPos[1]+pos[1]-pointer->getSizey()/2));
	pointer->draw();
	
    Component::scissor->pop();
    glPopMatrix();

	painter.drawSquareEdge(originalPos, originalSize);
}

bool MapPicture::isIn(int x, int y)
{
	return (originalPos[0]<=x && (originalSize[0]+originalPos[0])>=x && originalPos[1]<=y && (originalPos[1]+originalSize[1])>=y);
}

bool MapPicture::onKey(Uint16 k, S_GUI_VALUE s)
{
	if (s==S_GUI_PRESSED)
	{
		if (k == SDLK_PAGEUP) 
			zoomInOut(1.f);
		else if (k == SDLK_PAGEDOWN) 
			zoomInOut(-1.f);
		else
			return false;
		return true;
	}
	return false;
}

void MapPicture::calcPointerPos(int x, int y)
{
	if (SDL_GetModState() & KMOD_CTRL)
	{
		nearestIndex = cities.getNearest(getLongitudeFromx(x - pos[0]), getLatitudeFromy(y - pos[1]));
		if (nearestIndex != -1)
		{
			wstring n = cities.getCity(nearestIndex)->getNameI18();
			double lat, lon;
			exactLongitude = cities.getCity(nearestIndex)->getLongitude();
			exactLatitude = cities.getCity(nearestIndex)->getLatitude();
			exactAltitude = cities.getCity(nearestIndex)->getAltitude();
			lat = exactLatitude;
			lon = exactLongitude;
			pointerPos[0] = getxFromLongitude(exactLongitude);
			pointerPos[1] = getyFromLatitude(exactLatitude);
			exact = true;

			// lock onto this selected city			
			pointerIndex = nearestIndex;
			return;
		}
	}

	// just roaming
	pointerIndex = -1;
	pointerPos[0] = x - pos[0];
	pointerPos[1] = y - pos[1];
	exact = false;
}

bool MapPicture::onClic(int x, int y, S_GUI_VALUE button, S_GUI_VALUE state)
{
	if (!visible) return false;

 	if (!isIn(x,y)) 
	{
		dragging = false;
		panning = false;
		return false;
	}

	if (button==S_GUI_MOUSE_LEFT)
	{
		if (state == S_GUI_PRESSED)
		{
			dragging = true;
			calcPointerPos(x, y);
			if (!onPressCallback.empty()) RUNCALLBACK(onPressCallback);
		}
		else
			dragging = false;
	}
	if (button==S_GUI_MOUSE_RIGHT)
	{
		if (state == S_GUI_PRESSED)
		{
			panning = true;
			oldPos.set(x,y);
		}
		else
			panning = false;
	}
	else if (button == 	S_GUI_MOUSE_WHEELUP)
		zoomInOut(0.5f); //polls twice - press and release I assume
	else if (button == 	S_GUI_MOUSE_WHEELDOWN)
		zoomInOut(-0.5f);
	return true;
}

bool MapPicture::onMove(int x, int y)
{
	if (!visible) return false;
 	if (!isIn(x,y)) return false;

	cursorPos.set(x - pos[0], y - pos[1]);
	nearestIndex = cities.getNearest(getLongitudeFromx(x - pos[0]), getLatitudeFromy(y - pos[1]));

	if (dragging)
	{
		calcPointerPos(x, y);
		if (!onPressCallback.empty()) RUNCALLBACK(onPressCallback);
	}
	else if (panning)
	{
		// make sure can't go further than top left corner
		int l = pos[0]-(oldPos[0]-x);
		if (l > 0) l = 0;

		int t = pos[1]-(oldPos[1]-y);
		if (t > 0) t = 0;

		// check bottom corner
		int r = l + size[0];
		if (r < originalPos[0] + originalSize[0]) l = originalPos[0] + originalSize[0] - size[0];

		int b = t + size[1];
		if (b < originalPos[1] + originalSize[1]) t = originalPos[1] + originalSize[1] - size[1];

		setPos(l, t);
		
	}
	oldPos.set(x,y);
	return true;
}

void MapPicture::zoomInOut(float _step)
{
	float lastZoom = zoom;
	s_vec2i oldCursorPos = cursorPos;

	zoom += _step;
	if (zoom < 1.f) zoom = 1.f;
	else if (zoom > ZOOM_LIMIT) zoom = ZOOM_LIMIT;

	cursorPos[0] = (int)((float)cursorPos[0] * zoom / lastZoom);
	cursorPos[1] = (int)((float)cursorPos[1] * zoom / lastZoom);
	pointerPos[0] = (int)((float)pointerPos[0] * zoom / lastZoom);
	pointerPos[1] = (int)((float)pointerPos[1] * zoom / lastZoom);
	size[0] = (int)((float)originalSize[0] * zoom);
	size[1] = (int)((float)originalSize[1] * zoom);

	pos -= (cursorPos - oldCursorPos);

	if (pos[0] > originalPos[0])
		pos[0] = originalPos[0];

	if (pos[1] > originalPos[1])
		pos[1] = originalPos[1];

	if (pos[0] + size[0] < originalPos[0] + originalSize[0])
		pos[0] = originalPos[0] + originalSize[0] - size[0];

	if (pos[1] + size[1] < originalPos[1] + originalSize[1])
		pos[1] = originalPos[1] + originalSize[1] - size[1];
}

void MapPicture::findPosition(double longitude, double latitude)
{
	// get 1 second accuracy to locate the city
	cities.setProximity(1.f/3600);
	pointerIndex = cities.getNearest(longitude, latitude);
	cities.setProximity(-1);
}

string MapPicture::getLocationString(int index)
{
	string city, state, country;

	if (index == -1)
		return "";

	city = getCity(index) + ", ";
	state= getState(index);
	country = getCountry(index);
	
	if (state == "<>")
		state = "";
	else
		state += ", ";

	return city + state + country;
}

string MapPicture::getPositionString(void)
{
	if (pointerIndex == -1)
		return UNKNOWN_OBSERVATORY;
	else
		return getLocationString(pointerIndex);
}

string MapPicture::getCursorString(void)
{
	if (nearestIndex == -1)
		return "";
	else
		return getLocationString(nearestIndex);
}

int MapPicture::getPointerAltitude(void)
{
	if (exact)
		return exactAltitude;
	else 
		return 0; 
}

double MapPicture::getPointerLongitude(void)
{
	if (exact)
		return exactLongitude;
	else 
		return getLongitudeFromx(pointerPos[0]); 
}

double MapPicture::getPointerLatitude(void)
{
	if (exact)
		return exactLatitude;
	else 
		return getLatitudeFromy(pointerPos[1]); 
}

int MapPicture::getxFromLongitude(double _longitude)
{
	return (int)((_longitude+180.f)/360.f * size[0]);
}

int MapPicture::getyFromLatitude(double _latitude)
{
	return (size[1] - (int)((_latitude+90.f)/180.f * size[1]));
}

double MapPicture::getLongitudeFromx(int x)
{
	return (double)x/size[0]*360.f-180.f;
}

double MapPicture::getLatitudeFromy(int y)
{
	return (double)(1.f - (float)y/size[1])*180.f-90.f;
}

///////////////////////////// StringList ///////////////////////////////////////
// ClicList
////////////////////////////////////////////////////////////////////////////////

StringList::StringList()
{
	elemsSize = 0;
	current = items.end();
	itemSize = (int)painter.getFont()->getLineHeight() + (int)painter.getFont()->getDescent();
	size[0] = 100;
	size[1] = 100;
}

void StringList::draw(void)
{
	if (!visible) return;

	painter.drawSquareEdge(pos, size);

	vector<string>::iterator iter = items.begin();

	int y = 0;
	int x = pos[0];
	int descent = (int)painter.getFont()->getDescent();

	while(iter!=items.end())
	{
		if (iter==current) painter.print(x + 3, pos[1] + y + itemSize - descent, *iter, painter.getTextColor() * 2);
		else painter.print(x + 2, pos[1] + y + itemSize - descent,*iter);
		painter.drawLine(s_vec2i(x, pos[1]+y), s_vec2i(x+size[0], pos[1]+(int)y));
		y += itemSize;
		if (elemsSize > size[1] && y + 2*itemSize > size[1])
		{
			painter.drawLine(s_vec2i(x, pos[1]+y), s_vec2i(x+size[0], pos[1]+(int)y));
			painter.drawLine(s_vec2i(x+5, pos[1]+y+5), s_vec2i(x-5+size[0], pos[1]+(int)y+5));
			painter.drawLine(s_vec2i(x+5, pos[1]+y+5), s_vec2i(x+size[0]/2, pos[1]+size[1]-5));
			painter.drawLine(s_vec2i(x+size[0]/2, pos[1]+size[1]-5), s_vec2i(x-5+size[0], pos[1]+(int)y+5));
			return;
		}
		++iter;
	}
}

bool StringList::onClic(int x, int y, S_GUI_VALUE button, S_GUI_VALUE state)
{
	if (!visible || state!=S_GUI_PRESSED || !isIn(x,y)) return 0;
	int poss = (y-pos[1])/itemSize;
	if (items.begin()+poss == items.end() || (unsigned int)poss>items.size()) return 1;
	current = items.begin()+poss;
	if (!onPressCallback.empty()) RUNCALLBACK(onPressCallback);
	return 1;
}

void StringList::addItem(const string& newitem)
{
	items.push_back(newitem);
	current = items.begin();
	elemsSize+=itemSize;
}

const string StringList::getValue() const
{
	if (current==items.end()) return "";
	else return *current;
}

bool StringList::setValue(const string & s)
{
	vector<string>::iterator iter;
	for (iter=items.begin();iter!=items.end();++iter)
	{
		if (s==*iter)
		{
			current = iter;
			return true;
		}
	}
	return false;
}

///////////////////////////// Time_zone_item////////////////////////////////////
// Widget used to set time zone. Initialized from a file of type /usr/share/zoneinfo/zone.tab
////////////////////////////////////////////////////////////////////////////////

Time_zone_item::Time_zone_item(const string& zonetab_file)
{
	if (zonetab_file.empty())
	{
		cout << "Can't find file \"" << zonetab_file << "\"\n" ;
		exit(0);
	}

	ifstream is(zonetab_file.c_str());

	continents_names.setSize(100,150);

	string unused, tzname;
	char zoneline[256];
	int i;

	while (is.getline(zoneline, 256))
	{
		if (zoneline[0]=='#') continue;
		istringstream istr(zoneline);
		istr >> unused >> unused >> tzname;
		i = tzname.find("/");
		if (continents.find(tzname.substr(0,i))==continents.end())
		{
			continents.insert(pair<string, StringList >(tzname.substr(0,i),StringList()));
			continents[tzname.substr(0,i)].addItem(tzname.substr(i+1,tzname.size()));
			continents[tzname.substr(0,i)].setPos(105, 0);
			continents[tzname.substr(0,i)].setSize(150, 150);
			continents[tzname.substr(0,i)].setOnPressCallback(callback<void>(this, &Time_zone_item::onCityClic));
			continents_names.addItem(tzname.substr(0,i));
		}
		else
		{
			continents[tzname.substr(0,i)].addItem(tzname.substr(i+1,tzname.size()));
		}
	}

	is.close();


	size[0] = continents_names.getSizex() * 4;
	size[1] = continents_names.getSizey();

	continents_names.setOnPressCallback(callback<void>(this, &Time_zone_item::onContinentClic));

	addComponent(&continents_names);
	addComponent(&continents[continents_names.getValue()]);
}

void Time_zone_item::draw(void)
{
	if (!visible) return;

	Container::draw();
}

string Time_zone_item::gettz(void)
{
	if (continents.find(continents_names.getValue())!=continents.end())
		return continents_names.getValue() + "/" + continents[continents_names.getValue()].getValue();
	else return continents_names.getValue() + "/error" ;
}

void Time_zone_item::settz(const string& tz)
{
	int i = tz.find("/");
	continents_names.setValue(tz.substr(0,i));
	continents[continents_names.getValue()].setValue(tz.substr(i+1,tz.size()));
}

void Time_zone_item::onContinentClic(void)
{
	removeAllComponents();
	addComponent(&continents_names);
	addComponent(&continents[continents_names.getValue()]);
	if (!onPressCallback.empty()) RUNCALLBACK(onPressCallback);
}

void Time_zone_item::onCityClic(void)
{
	if (!onPressCallback.empty()) RUNCALLBACK(onPressCallback);
}
