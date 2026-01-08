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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */


#ifndef DIALOG_HPP
#define DIALOG_HPP

#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <QMouseEvent>

//! \class TitleBar
//! A title bar control used in windows derived from StelDialog.
//! 
//! As window classes derived from StelDialog are basic QWidgets, they have no
//! title bar. A TitleBar control needs to be used in each window's design
//! to allow the user to move them.
//! 
//! Typically, the frame should contain a centered label displaying the window's
//! title and a button for closing the window (connected to the 
//! StelDialog::close() slot).

class TitleBar : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle)
public:
    QPoint mousePos;

    TitleBar(QWidget* parent = nullptr)
        : QFrame(parent)
    {
        setVisible(false);
    }

    void setTitle(const QString& title) {
        m_title = title;
        QWidget* p = parentWidget();
        if (p)
            p->setWindowTitle(title);
    }

    QString title() const { return m_title; }
protected:
    void mousePressEvent(QMouseEvent *event){};
    void mouseReleaseEvent(QMouseEvent *event){};
    void mouseMoveEvent(QMouseEvent *event){};
signals:
    void closeClicked();
    void movedTo(QPoint newPosition);
protected:
    bool moving = false;
    QString m_title;
};

class ResizeFrame : public QFrame
{
Q_OBJECT
public:
	QPoint mousePos;
  
	ResizeFrame(QWidget* parent) : QFrame(parent) {}
  
	void mousePressEvent(QMouseEvent *event) override {
		mousePos = event->pos();
	}
	void mouseMoveEvent(QMouseEvent *event) override;
};

#endif // DIALOG_HPP
