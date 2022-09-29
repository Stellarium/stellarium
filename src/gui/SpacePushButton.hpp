/*
 * Stellarium
 * Copyright (C) 2021 Georg Zotti
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef SPACEPUSHBUTTON_HPP
#define SPACEPUSHBUTTON_HPP

#include <QPushButton>

//! @class SpacePushButton
//! A QPushButton which can be triggered by pressing (actually: releasing) Space when it has focus.
//! To use this class in the QtCreator UI designer, add a regular QPushButton to the UI,
//! then right-click on it and change its type to SpacePushButton.
//! Then it makes sense to put this button into a useful GUI tab order.
class SpacePushButton : public QPushButton
{
	Q_OBJECT
public:
	SpacePushButton(QWidget* parent=Q_NULLPTR);
	~SpacePushButton() Q_DECL_OVERRIDE {}
	
protected:
    //! This triggers the button on pressing the Space bar.
    virtual void keyReleaseEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
};

#endif // SPACEPUSHBUTTON_HPP
