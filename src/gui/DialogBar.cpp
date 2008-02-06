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


#include "DialogBar.hpp"
#include <QMouseEvent>
#include <iostream>

#include <QHBoxLayout>
#include <QToolButton>
#include <QGLWidget>

DialogBar::DialogBar(QWidget *parent)
 : QFrame(parent)
{
    QHBoxLayout* hboxLayout = new QHBoxLayout(this);
    hboxLayout->setContentsMargins(-1, 2, 4, 2);
    QSpacerItem* spacerItem = new QSpacerItem(441, 20, QSizePolicy::Expanding,
QSizePolicy::Minimum);

    hboxLayout->addItem(spacerItem);

    QToolButton* toolButton = new QToolButton(this);
    toolButton->setMinimumSize(QSize(16, 16));
    toolButton->setMaximumSize(QSize(16, 16));
    hboxLayout->addWidget(toolButton);
    
    // Connect the bouton event to the closing of the window
    QObject::connect(toolButton, SIGNAL(clicked()), this, SLOT(close_parent()));
}


DialogBar::~DialogBar()
{
}


void DialogBar::mousePressEvent(QMouseEvent *event)
{
  mouse_pos = event->pos();
}

void DialogBar::mouseReleaseEvent(QMouseEvent *event)
{
}

void DialogBar::mouseMoveEvent(QMouseEvent *event)
{
  QPoint dpos = event->pos() - mouse_pos;
  QWidget* p = dynamic_cast<QWidget*>(parent());
  p->move(p->pos() + dpos);
  // p->repaint(QRect());
  // QWidget* p2 = dynamic_cast<QWidget*>(p->parent());
  // QGLWidget* gw = p2->findChild<QGLWidget*>();
  // p2->update();
}
