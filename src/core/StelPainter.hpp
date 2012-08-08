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
#include "VecMath.hpp"
#include "StelSphereGeometry.hpp"
#include "StelProjectorType.hpp"
#include "StelProjector.hpp"
#include <QString>
#include <QVarLengthArray>
#include <QFontMetrics>

//GL-REFACTOR: This will be defined while we refactor the GL2 code.
#define STELPAINTER_GL2 1
 
#ifdef USE_OPENGL_ES2
 #define STELPAINTER_GL2 1
#endif

#ifdef STELPAINTER_GL2
class QGLShaderProgram;
#endif

class QPainter;
class QGLContext;

//GL-REFACTOR: Remove this once it's not used
//! Define the drawing mode when drawing polygons
enum SphericalPolygonDrawMode
{
	SphericalPolygonDrawModeFill=0,        //!< Draw the interior of the polygon
	SphericalPolygonDrawModeBoundary=1,    //!< Draw the boundary of the polygon
	SphericalPolygonDrawModeTextureFill=2  //!< Draw the interior of the polygon filled with the current texture
};

//! @class StelPainter
//! Provides functions for performing openGL drawing operations.
//! All coordinates are converted using the StelProjector instance passed at construction.
//! Because openGL is not thread safe, only one instance of StelPainter can exist at a time, enforcing thread safety.
//! As a coding rule, no openGL calls should be performed when no instance of StelPainter exist.
//! Typical usage is to create a local instance of StelPainter where drawing operations are needed.
class StelPainter
{
public:
	//! Return the instance of projector associated to this painter
	const StelProjectorP& getProjector() const {return prj;}
	void setProjector(const StelProjectorP& p);

	//! Get some informations about the OS openGL capacities and set the GLContext which will be used by Stellarium.
	//! This method needs to be called once at init.
	static void initSystemGLInfo(QGLContext* ctx);

	//! Set the QPainter to use for performing some drawing operations.
	static void setQPainter(QPainter* qPainter);

	//! Make sure that our GL context is current and valid.
	static void makeMainGLContextCurrent();

private:
	//! The associated instance of projector
	StelProjectorP prj;

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

