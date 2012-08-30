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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _STELMAINWINDOW_HPP_
#define _STELMAINWINDOW_HPP_

#include <QMainWindow>
#include <QSettings>

//! @class StelMainWindow
//! Reimplement a QMainWindow for Stellarium.
//! It is the class in charge of switching betwee fullscreen or windowed mode.
class StelMainWindow : public QMainWindow
{
	Q_OBJECT
public:
	StelMainWindow();

	//! Get the StelMainWindow singleton instance.
	//! @return the StelMainWindow singleton instance
	static StelMainWindow& getInstance() {Q_ASSERT(singleton); return *singleton;}

	//! Performs various initialization including the init of the StelMainGraphicsView instance.
	void init(QSettings* settings);

	void deinit();

	//! Set the application title for the current language.
	//! This is useful for e.g. chinese.
	void initTitleI18n();

public slots:
	//! Alternate fullscreen mode/windowed mode if possible
	void toggleFullScreen();

	//! Get whether fullscreen is activated or not
	bool getFullScreen() const;
	//!	Set whether fullscreen is activated or not
	void setFullScreen(bool);

protected:
	//! Reimplemented to delete textures before the renderer disappears
	virtual void closeEvent(QCloseEvent* event);

private:
	//! The StelMainWindow singleton
	static StelMainWindow* singleton;

	class StelMainGraphicsView* mainGraphicsView;
};

#endif // _STELMAINWINDOW_HPP_
