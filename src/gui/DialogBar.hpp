/*
 * Stellarium
 * Copyright (C) 2008 Guillaume Chereau
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


#ifndef _DIALOGBAR_H_
#define _DIALOGBAR_H_

 
#include <QFrame>

/**
	@author guillaume,,, <guillaume@hellokitty>
*/
class DialogBar : public QFrame
{
Q_OBJECT
public:
    QPoint mouse_pos;

    DialogBar(QWidget *parent = 0);

    ~DialogBar();

    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    
public slots:
    void close_parent() {dynamic_cast<QWidget*>(parent())->close();}
};

#endif
