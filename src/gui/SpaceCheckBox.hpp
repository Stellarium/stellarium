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

#ifndef SPACECHECKBOX_HPP
#define SPACECHECKBOX_HPP

#include <QCheckBox>

//! @class SpaceCheckBox
//! A QCheckBox which can be toggled by pressing (actually releasing) Space when it has focus.
//! To use this class in the QtCreator UI designer, add a regular QCheckBox to the UI,
//! then right-click on it and change its type to SpaceCheckBox.
//! Then it makes sense to put this checkbox into a useful GUI tab order.
class SpaceCheckBox : public QCheckBox
{
	Q_OBJECT
public:
	SpaceCheckBox(QWidget* parent=Q_NULLPTR);
	~SpaceCheckBox() Q_DECL_OVERRIDE {}
	
protected:
    //! This toggles the checkbox on releasing the Space bar.
    virtual void keyReleaseEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
};

#endif // SPACECHECKBOX_HPP
