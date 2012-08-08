/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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

#ifndef _STELPAINTER_HPP_
#define _STELPAINTER_HPP_

class QPainter;
class QGLContext;

//! @class StelPainter
//! Provides functions for performing openGL drawing operations.
//! All coordinates are converted using the StelProjector instance passed at construction.
//! Because openGL is not thread safe, only one instance of StelPainter can exist at a time, enforcing thread safety.
//! As a coding rule, no openGL calls should be performed when no instance of StelPainter exist.
//! Typical usage is to create a local instance of StelPainter where drawing operations are needed.
class StelPainter
{
public:
	//! Get some informations about the OS openGL capacities and set the GLContext which will be used by Stellarium.
	//! This method needs to be called once at init.
	static void initSystemGLInfo(QGLContext* ctx);

	//! Set the QPainter to use for performing some drawing operations.
	static void setQPainter(QPainter* qPainter);

	//! Make sure that our GL context is current and valid.
	static void makeMainGLContextCurrent();

private:
#ifndef NDEBUG
	//! Mutex allowing thread safety
	static class QMutex* globalMutex;
#endif

	//! The QPainter to use for some drawing operations.
	static QPainter* qPainter;

	//! The main GL Context used by Stellarium.
	static QGLContext* glContext;
};

#endif // _STELPAINTER_HPP_

