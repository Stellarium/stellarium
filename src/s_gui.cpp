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

#include <iostream>
#include <sstream>

#include "s_gui.h"

using namespace s_gui;

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
	font(NULL)
{
}

Painter::Painter(const s_texture* _tex1, const s_font* _font,
	const s_color& _baseColor, const s_color& _textColor) :
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

// Draw a cross with the default base color
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
void Painter::print(int x, int y, const string& str) const
{
    glColor4fv(textColor);
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

int Component::isIn(int x, int y)
{
	return (pos[0]<=x && (size[0]+pos[0])>=x && pos[1]<=y && (pos[1]+size[1])>=y);
}

void Component::initScissor(int winW, int winH)
{
	if (scissor) delete scissor;
	scissor = new Scissor(winW, winH);
}


CallbackComponent::CallbackComponent() : Component(), is_mouse_over(0)
{
}

int CallbackComponent::onClic(int x, int y, S_GUI_VALUE bt, S_GUI_VALUE state)
{
	if (!visible) return 0;
	if (state==S_GUI_PRESSED && bt==S_GUI_MOUSE_LEFT && isIn(x, y))
	{
		if (!onPressCallback.empty()) onPressCallback();
	}
	return 0;
}

int CallbackComponent::onMove(int x, int y)
{
	if (!visible) return 0;
	if (isIn(x, y))
	{
		if (!onMouseInOutCallback.empty() && !is_mouse_over)
		{
			is_mouse_over = 1;
			onMouseInOutCallback();
		}
		is_mouse_over = 1;
	}
	else
	{
		if (is_mouse_over)
		{
			if (!onMouseInOutCallback.empty() && is_mouse_over)
			{
				is_mouse_over = 0;
				onMouseInOutCallback();
			}
		}
		is_mouse_over = 0;
	}
	return 0;
}

//////////////////////////////// Container /////////////////////////////////////
// Manages hierarchical components : send signals ad actions  to childrens
////////////////////////////////////////////////////////////////////////////////

Container::Container() : CallbackComponent()
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

    Component::scissor->push(pos, size);

	while (iter != childs.end())
	{
		(*iter)->draw();
		iter++;
	}

    Component::scissor->pop();

    glPopMatrix();
}

int Container::onClic(int x, int y, S_GUI_VALUE button, S_GUI_VALUE state)
{
	if (!visible) return 0;
	list<Component*>::iterator iter = childs.begin();
	while (iter != childs.end())
	{
		if ((*iter)->onClic(x - pos[0], y - pos[1], button, state))
		{
			// The signal has been intercepted
			// Set the component in first position in the objects list
			childs.push_front(*iter);
			childs.erase(iter);
			return 1;
		}
        iter++;
    }
	return CallbackComponent::onClic(x, y, button, state);
}

int Container::onMove(int x, int y)
{
	if (!visible) return 0;
	list<Component*>::iterator iter = childs.begin();
	while (iter != childs.end())
	{
		if ((*iter)->onMove(x - pos[0], y - pos[1])) return 1;	// The signal has been intercepted
        iter++;
    }
	return CallbackComponent::onMove(x, y);
}

int Container::onKey(SDLKey k, S_GUI_VALUE s)
{
	if (!visible) return 0;
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
    if (!visible) return;
	painter.drawSquareFill(pos, size);
	//painter.drawSquareEdge(pos, size);

	Container::draw();
}


////////////////////////////////// Button //////////////////////////////////////
// Simplest button with one press callback
////////////////////////////////////////////////////////////////////////////////

Button::Button() : CallbackComponent()
{
	size.set(10,10);
}

void Button::draw()
{
	if (!visible) return;
	painter.drawSquareEdge(pos, size, painter.getBaseColor() * (1.f + 0.4 * is_mouse_over));
}

int Button::onClic(int x, int y, S_GUI_VALUE bt, S_GUI_VALUE state)
{
	if (!visible) return 0;
	CallbackComponent::onClic(x,y,bt,state);
	if (state==S_GUI_PRESSED && bt==S_GUI_MOUSE_LEFT && isIn(x, y))	return 1;
	return 0;
}

void FilledButton::draw()
{
	if (!visible) return;
	painter.drawSquareFill(pos, size);
	Button::draw();
}


//////////////////////////////////// CheckBox //////////////////////////////////
// Button with a cross on it
////////////////////////////////////////////////////////////////////////////////

CheckBox::CheckBox(int state) : Button(), isChecked(state)
{
}

void CheckBox::draw()
{
	if (!visible) return;
	if (isChecked) painter.drawCross(pos, size);
	Button::draw();
}

int CheckBox::onClic(int x, int y, S_GUI_VALUE bt, S_GUI_VALUE state)
{
	if (!visible) return 0;
	if (state==S_GUI_PRESSED && bt==S_GUI_MOUSE_LEFT && isIn(x, y))
	{
		if (isChecked) isChecked = 0;
		else isChecked = 1;
	}
	return Button::onClic(x,y,bt,state);
}


LabeledCheckBox::LabeledCheckBox(int state, const string& label) : Container(), checkbx(NULL), lbl(NULL)
{
	checkbx = new CheckBox(state);
	addComponent(checkbx);
	lbl = new Label(label);
	lbl->setPos(checkbx->getSizex()+4, 0);
	addComponent(lbl);
	setSize(checkbx->getSizex() + lbl->getSizex() + 2,
		checkbx->getSizey()>lbl->getSizey() ? checkbx->getSizey() : lbl->getSizey());
}

FlagButton::FlagButton(int state, const s_texture* tex, const string& specificTexName) : CheckBox(state)
{
	if (tex) setTexture(tex);
	if (!specificTexName.empty()) specific_tex = new s_texture(specificTexName);
	setSize(32,32);
}

FlagButton::~FlagButton()
{
	if (specific_tex) delete specific_tex;
}

void FlagButton::draw()
{
	if (!visible) return;
	if (isChecked)
	{
		if (specific_tex) painter.drawSquareFill(pos, size, painter.getBaseColor()*2, specific_tex);
		else painter.drawSquareFill(pos, size, painter.getBaseColor()*1.5);
	}
	else
	{
		if (specific_tex) painter.drawSquareFill(pos, size, painter.getBaseColor()*0.5, specific_tex);
		else painter.drawSquareFill(pos, size);
	}
	Button::draw();
}


//////////////////////////////////// Label /////////////////////////////////////
// Text label
////////////////////////////////////////////////////////////////////////////////

Label::Label(const string& _label, const s_font * _font)
{
	setLabel(_label);
	if (_font) painter.setFont(_font);
	adjustSize();
}

Label::~Label()
{
}

void Label::setLabel(const string& _label)
{
    label = _label;
}

void Label::draw()
{
	if (!visible) return;
    glPushMatrix();
    if (painter.getFont())
    {
		painter.print(pos[0], pos[1], label);
    }
    glPopMatrix();
	//painter.drawSquareEdge(pos, size);
}

void Label::adjustSize(void)
{
	size[0] = (int)ceilf(painter.getFont()->getStrLen(label));
	size[1] = (int)ceilf(painter.getFont()->getLineHeight());
}


/////////////////////////////// LabeledButton //////////////////////////////////
// Button with text on it
////////////////////////////////////////////////////////////////////////////////

LabeledButton::LabeledButton(const string& _label, const s_font* font) : Button(), label(_label, font)
{
	setSize(label.getSize()+s_vec2i(4,2));
}

LabeledButton::~LabeledButton()
{
}

void LabeledButton::draw(void)
{
    if (!visible) return;
	Button::draw();
    glPushMatrix();
    glTranslatef(pos[0], pos[1], 0.f);
    Component::scissor->push(pos, size);
	label.setPos((size-label.getSize())/2);
	label.draw();
    Component::scissor->pop();
	glPopMatrix();
}


////////////////////////////////// TextLabel ///////////////////////////////////
// A text bloc
////////////////////////////////////////////////////////////////////////////////

TextLabel::TextLabel(const string& _label, const s_font* _font) : Container()
{
	if (_font) painter.setFont(_font);
	setLabel(_label);
	adjustSize();
}

TextLabel::~TextLabel()
{
}

void TextLabel::setLabel(const string& _label)
{
    label = _label;
    childs.clear();

    Label * tempLabel;
    string pch;

    int i = 0;
	int lineHeight = (int)painter.getFont()->getLineHeight()+1;

	istringstream is(label);
    while (getline(is, pch))
    {
        tempLabel = new Label(pch);
		tempLabel->setPainter(painter);
        tempLabel->setPos(0,i*lineHeight);
		tempLabel->adjustSize();
        addComponent(tempLabel);
        ++i;
    }
}

void TextLabel::adjustSize(void)
{
	int maxX = 0;
	list<Component*>::iterator iter = childs.begin();
	while (iter != childs.end())
	{
		if ((*iter)->getSizex()>maxX) maxX = (*iter)->getSizex();
        iter++;
    }
	setSize(maxX,childs.size()*((int)painter.getFont()->getLineHeight()+1));
}

void TextLabel::setTextColor(const s_color& c)
{
	list<Component*>::iterator iter = childs.begin();
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

StdWin::StdWin(const char * _title, s_texture* _header_tex, s_font * _winfont, int headerSize) :
	FramedContainer(), titleLabel(NULL), header_tex(NULL), dragging(0)
{
	if (_header_tex) header_tex = _header_tex;
	if (_winfont) painter.setFont(_winfont);
	setFrameSize(1,1,1,headerSize);
	titleLabel = new Label();
	setTitle(_title);
	Container::addComponent(titleLabel);
}

void StdWin::setTitle(const char * _title)
{
	titleLabel->setLabel(_title);
	titleLabel->adjustSize();
}

void StdWin::draw()
{
	if (!visible) return;
	titleLabel->setPos((size[0] - titleLabel->getSizex())/2, (frameSize[3]-titleLabel->getSizey())/2 + 1);
	painter.drawSquareFill(pos, size);
	painter.drawSquareFill(pos, s_vec2i(size[0], frameSize[3]));
	FramedContainer::draw();
}

int StdWin::onClic(int x, int y, S_GUI_VALUE bt, S_GUI_VALUE state)
{
	if (!visible) return 0;
	if (FramedContainer::onClic(x, y, bt, state)) return 1;
	if (state==S_GUI_RELEASED && bt==S_GUI_MOUSE_LEFT)
	{
		dragging = 0;
	}
	if (isIn(x, y))
	{
		if (state==S_GUI_PRESSED && bt==S_GUI_MOUSE_LEFT)
		{
			dragging = 1;
			oldpos.set(x,y);
		}
		return 1;
	}
	return 0;
}

int StdWin::onMove(int x, int y)
{
	if (!visible) return 0;
	if (FramedContainer::onMove(x, y)) return 1;
	if (dragging)
	{
		pos+=(s_vec2i(x,y)-oldpos);
		oldpos.set(x,y);
	}
	return 0;
}

StdBtWin::StdBtWin(const char * _title, s_texture* _header_tex, s_font * _winfont, int headerSize) :
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
	visible=0;
	if (!onHideBtCallback.empty()) onHideBtCallback();
}


///////////////////////////////////// Tab //////////////////////////////////////
// Everything to handle tabs
////////////////////////////////////////////////////////////////////////////////

TabHeader::TabHeader(Component* c, const string& _label, const s_font* _font) :
	LabeledButton(_label, _font), assoc(c)
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
	// Draw laeb
    glPushMatrix();
    glTranslatef(pos[0], pos[1], 0.f);
    Component::scissor->push(pos, size);
	label.setPos((size-label.getSize())/2 + s_vec2i(1,0));
	label.draw();
    Component::scissor->pop();
	glPopMatrix();
}

void TabHeader::setActive(int s)
{
	active = s;
	if (active) assoc->setVisible(1);
	else assoc->setVisible(0);
}

TabContainer::TabContainer(const s_font* _font) : Container(), headerHeight(20)
{
	if (_font) painter.setFont(_font);

}

void TabContainer::addTab(Component* c, const string& name)
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
	painter.drawSquareFill(pos, size);
	painter.drawLine(s_vec2i(pos[0]+getHeadersSize(), pos[1]+headerHeight-1),
		s_vec2i(pos[0]+size[0], pos[1]+headerHeight-1));
	Container::draw();
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

int TabContainer::onClic(int x, int y, S_GUI_VALUE button, S_GUI_VALUE state)
{
	if (!visible) return 0;
	list<TabHeader*>::iterator iter = headers.begin();
	while (iter != headers.end())
	{
		if ((*iter)->onClic(x - pos[0], y - pos[1], button, state))
		{
			select(*iter);
			return 1;
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
		(*iter)->setVisible(0);
        iter++;
    }
	list<TabHeader*>::iterator iter2 = headers.begin();
	while (iter2 != headers.end())
	{
		(*iter2)->setVisible(1);
		if (*iter2==t) (*iter2)->setActive(1);
		else (*iter2)->setActive(0);
        iter2++;
    }
}


// Cursor Bar

CursorBar::CursorBar(float _min, float _max, float _val) : cursor(), minBar(_min), maxBar(_max)
{
	float tmpVal = _val;
	if (tmpVal<minBar && tmpVal>maxBar) tmpVal = (minBar + maxBar)/2;
	setSize(100,15);
	cursor.setSize(8, getSizey());
	setValue(tmpVal);
	oldPos = cursor.getPos();
}

void CursorBar::draw(void)
{
    if (!visible) return;
	painter.drawSquareEdge(pos, size);
    glPushMatrix();
    glTranslatef(pos[0], pos[1], 0.f);
    Component::scissor->push(pos, size);
	cursor.draw();
    Component::scissor->pop();
	glPopMatrix();
	if (dragging)
	{
		char temp[255];
		sprintf(temp,"%.2f",barVal);
		painter.print(pos[0]+2, pos[1]+2, temp);
	}
}

void CursorBar::setValue(float _barVal)
{
	barVal = _barVal;
	if (barVal<minBar) barVal = minBar;
	if (barVal>maxBar) barVal = maxBar;
	cursor.setPos((int)((barVal-minBar)/(maxBar-minBar) * (size[0] - cursor.getSizex())), 0);
}

int CursorBar::onClic(int x, int y, S_GUI_VALUE bt, S_GUI_VALUE state)
{
	if (bt==S_GUI_MOUSE_LEFT && state==S_GUI_RELEASED)
	{
		dragging = 0;
		return 0;
	}
	if (visible && isIn(x,y) && state==S_GUI_PRESSED)
	{
		if (cursor.isIn(x-pos[0],y-pos[1])) dragging = 1;
		return 1;
	}
	return 0;
}

int CursorBar::onMove(int x, int y)
{
	if (!visible) return 0;
	x = x - pos[0];
	y = y - pos[1];
	cursor.onMove(x,y);
	if (!dragging) return 0;
	if (x<0) x=0;
	if (x>size[0]-cursor.getSizex()) x=size[0]-cursor.getSizex();
	cursor.setPos(cursor.getPosx()+x-oldPos[0], cursor.getPosy());
	if (cursor.getPosx()<0) cursor.setPos(0,cursor.getPosy());
	if (cursor.getPosx()>size[0]-cursor.getSizex()) cursor.setPos(size[0]-cursor.getSizex(),cursor.getPosy());
	barVal=(float)cursor.getPosx()/(size[0]-cursor.getSizex()) * (maxBar-minBar);
	if (!onChangeCallback.empty()) onChangeCallback();
	oldPos.set(x,y);
	return 0;
}



DecimalIncDec::DecimalIncDec(float _min, float _max, float _inc, float _init_value, const s_font* _font) :
	Container(), value(_init_value), min(_min), max(_max), inc(_inc), btmore(NULL), btless(NULL)
{
	if (_font) painter.setFont(_font);
	btless = new LabeledButton("-", painter.getFont());
	btless->setSize(10,10);
	btless->setOnPressCallback(callback<void>(this, &DecimalIncDec::dec_value));
	label.setPos(btless->getSizex(),0);
	label.setSize(30,10);
	btmore = new LabeledButton("+", painter.getFont());
	btmore->setSize(10,10);
	btmore->setOnPressCallback(callback<void>(this, &DecimalIncDec::inc_value));
	btmore->setPos(label.getPosx() + label.getSizex(),0);
	addComponent(btmore);
	addComponent(btless);
	addComponent(&label);
	setSize(100,50);
}

DecimalIncDec::~DecimalIncDec()
{
	if (btmore) delete btmore;
	if (btless) delete btless;
}

void DecimalIncDec::draw()
{
	static char str[255];
	snprintf(str, 255, "% .2f",value);
	label.setLabel(str);
	btmore->setPos(label.getPosx() + label.getSizex(),0);

	Container::draw();
}

/*
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
