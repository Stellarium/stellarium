/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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

#ifndef STELDIALOGSEPARATE_HPP
#define STELDIALOGSEPARATE_HPP

#include "StelDialog.hpp"
#include <QDialog>

class QGraphicsColorizeEffect;
class NightCover;

//! @class StelDialogSeparate Base class for windows NOT embedded in the graphics scene.
//! Some GUI elements like QWebEngineView don't like to be embedded into our MainView.
//! Another motivation may be to have windows on another screen.
//!
//! The dialog behaves almost like @class StelDialog. However, given how night mode is applied as screen effect,
//! this dialog does not adhere to the global night mode. Therefore a semitransparent QWidget (@class NightCover)
//! is placed on top. This does not filter away all blue light, but at least dims bright non-red colors.
//!
class StelDialogSeparate : public StelDialog
{
	Q_OBJECT
public:
	StelDialogSeparate(QString dialogName="Default", QObject* parent=Q_NULLPTR);
	virtual ~StelDialogSeparate() Q_DECL_OVERRIDE;

public slots:
	//! On the first call with "true" populates the window contents.
	virtual void setVisible(bool) Q_DECL_OVERRIDE;

protected slots:
	virtual void updateNightModeProperty(bool n) Q_DECL_OVERRIDE;

protected:
	NightCover *nightCover;
	QGraphicsColorizeEffect *nightModeEffect; // This seems not to work properly!
};

//! This class allows storing size changes when its sizeChanged() signal is connected to some handler.
class CustomDialog : public QDialog
{
	Q_OBJECT
public:
	CustomDialog(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags())
	: QDialog(parent, f)
	{
		setFocusPolicy(Qt::StrongFocus);
	}

signals:
	void sizeChanged(QSizeF);

protected:
	virtual void resizeEvent(QResizeEvent *event)
	{
		if (event->size() != event->oldSize())
		{
			emit sizeChanged(event->size());
		}
		QDialog::resizeEvent(event);
	}
};

#endif // STELDIALOGSEPARATE_HPP
