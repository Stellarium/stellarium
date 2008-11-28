/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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

#ifndef _STELMAINWINDOW_HPP_
#define _STELMAINWINDOW_HPP_

#include <QMainWindow>

#include <QSettings>
#include "StelApp.hpp"

//! @class StelMainWindow
//! Reimplement a QMainWindow for Stellarium.
//! It is the class in charge of switching betwee fullscreen or windowed mode.
class StelMainWindow : public QMainWindow
{
	Q_OBJECT;
public:
	StelMainWindow(QWidget* parent);
	
	//! Get the StelMainWindow singleton instance.
	//! @return the StelMainWindow singleton instance
	static StelMainWindow& getInstance() {Q_ASSERT(singleton); return *singleton;}

	//! Performs various initialization incluing the init of the StelMainGraphicsView instance.
	void init();
	
public slots:
	//! Alternate fullscreen mode/windowed mode if possible
	void toggleFullScreen();

	//! Get whether fullscreen is activated or not
	bool getFullScreen() const;
	//!	Set whether fullscreen is activated or not
	void setFullScreen(bool);
	
protected:	
	//! Reimplemented to delete openGL textures before the GLContext disappears
 	virtual void closeEvent(QCloseEvent* event);
 	
	virtual void resizeEvent(QResizeEvent* event);
		
private:
	//! The StelMainWindow singleton
	static StelMainWindow* singleton;
	
	//! Used to prevent resize events saving to the config file until after main init is complete.
	bool initComplete;
};

#endif // _STELMAINWINDOW_HPP_
