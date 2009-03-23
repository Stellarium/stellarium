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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _STELPAINTER_HPP_
#define _STELPAINTER_HPP_

#include "GLee.h"
#include "fixx11h.h"
#include "VecMath.hpp"
#include "StelSphereGeometry.hpp"
#include "StelProjectorType.hpp"
#include "StelProjector.hpp"
#include <QString>

class StelFont;

//! @class StelPainter
//! Provides functions for performing openGL drawing operations.
//! All coordinates are converted using the StelProjector instance passed at construction.
//! Because openGL is not thread safe, only one instance of StelPainter can exist at a time, enforcing thread safety.
//! As a coding rule, no openGL calls should be performed when no instance of StelPainter exist.
//! Typical usage is to create a local instance of StelPainter where drawing operations are needed.

class StelPainter
{
public:
	
	explicit StelPainter(const StelProjectorP& prj);
	~StelPainter();
	
	//! Return the instance of projector associated to this painter
	const StelProjectorP getProjector() const {return prj;}
	
	//! Fill with black around the viewport.
	void drawViewportShape(void) const;
	
	//! Generalisation of glVertex3v for non-linear projections. 
	//! This method does not manage the lighting operations properly.
	void drawVertex3v(const Vec3d& v) const
	{
		Vec3d win;
		prj->project(v, win);
		glVertex3dv(win);
	}
	//! Convenience function.
	//! @sa drawVertex3v
	void drawVertex3(double x, double y, double z) const {drawVertex3v(Vec3d(x, y, z));}

	//! Draw the string at the given position and angle with the given font.
	//! If the gravity label flag is set, uses drawTextGravity180.
	//! @param font the font to use for display
	//! @param x horizontal position of the lower left corner of the first character of the text in pixel.
	//! @param y horizontal position of the lower left corner of the first character of the text in pixel.
	//! @param str the text to print.
	//! @param angleDeg rotation angle in degree. Rotation is around x,y.
	//! @param xshift shift in pixel in the rotated x direction.
	//! @param yshift shift in pixel in the rotated y direction.
	//! @param noGravity don't take into account the fact that the text should be written with gravity.
	void drawText(const StelFont* font, float x, float y, const QString& str, float angleDeg=0.f, 
		      float xshift=0.f, float yshift=0.f, bool noGravity=true) const;
	
	//! Draw the given polygon.
	//! @param poly The polygon to draw.
	void drawPolygon(const StelGeom::Polygon& poly) const;
	
	//! Draw the given SphericalPolygon.
	//! @param spoly The SphericalPolygon to draw.
	//! @param outlineOnly if true, display only the outline of the polygon, else fill the polygon.
	void drawSphericalPolygon(const StelGeom::SphericalPolygon& spoly, bool outlineOnly=false) const;
	
	//! Draw a small circle arc between points start and stop with rotation point in rotCenter.
	//! The angle between start and stop must be < 180 deg.
	//! The algorithm ensures that the line will look smooth, even for non linear distortion.
	//! Each time the small circle crosses the edge of the viewport, the viewportEdgeIntersectCallback is called with the
	//! screen 2d position, direction of the currently drawn arc toward the inside of the viewport.
	//! If rotCenter is equal to 0,0,0, the method draws a great circle.
	void drawSmallCircleArc(const Vec3d& start, const Vec3d& stop, const Vec3d& rotCenter, void (*viewportEdgeIntersectCallback)(const Vec3d& screenPos, const Vec3d& direction, const void* userData)=NULL, const void* userData=NULL) const;
	
	//! Draw a simple circle, 2d viewport coordinates in pixel
	void drawCircle(double x,double y,double r) const;

	//! Draw a square using the current texture at the given projected 2d position.
	//! @param x x position in the viewport in pixel.
	//! @param y y position in the viewport in pixel.
	//! @param size the size of a square side in pixel.
	void drawSprite2dMode(double x, double y, double size) const;
	
	//! Draw a rotated square using the current texture at the given projected 2d position.
	//! @param x x position in the viewport in pixel.
	//! @param y y position in the viewport in pixel.
	//! @param size the size of a square side in pixel.
	//! @param rotation rotation angle in degree.
	void drawSprite2dMode(double x, double y, double size, double rotation) const;
	
	//! Draw a rotated rectangle using the current texture at the given projected 2d position.
	//! @param x x position in the viewport in pixel.
	//! @param y y position in the viewport in pixel.
	//! @param sizex the size of the rectangle x side in pixel.
	//! @param sizey the size of the rectangle y side in pixel.
	//! @param rotation rotation angle in degree.
	void drawRectSprite2dMode(double x, double y, double sizex, double sizey, double rotation) const;
	
	//! Draw a GL_POINT at the given position.
	//! @param x x position in the viewport in pixels.
	//! @param y y position in the viewport in pixels.
	void drawPoint2d(double x, double y) const;
	
	//! Re-implementation of gluSphere : glu is overridden for non-standard projection.
	void sSphere(GLdouble radius, GLdouble oneMinusOblateness,
	             GLint slices, GLint stacks, int orientInside = 0) const;

	//! Re-implementation of gluCylinder : glu is overridden for non-standard projection.
	void sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks, int orientInside = 0) const;

	//! Draw a disk with a special texturing mode having texture center at center of disk.
	//! The disk is made up of concentric circles with increasing refinement.
	//! The number of slices of the outmost circle is (innerFanSlices<<level).
	//! @param radius the radius of the disk.
	//! @param innerFanSlices the number of slices.
	//! @param level the numbe of concentric circles.
	void sFanDisk(double radius,int innerFanSlices,int level) const;

	//! Draw a disk with a special texturing mode having texture center at center.
	//! @param radius the radius of the disk.
	//! @param slices the number of slices.
	//! @param stacks ???
	//! @param orientInside ???
	void sDisk(GLdouble radius, GLint slices, GLint stacks, int orientInside = 0) const;
	
	//! Draw a ring with a radial texturing.
	void sRing(GLdouble rMin, GLdouble rMax, GLint slices, GLint stacks, int orientInside) const;

	//! Draw a fisheye texture in a sphere.
	void sSphereMap(GLdouble radius, GLint slices, GLint stacks,
	                 double textureFov = 2.*M_PI, int orientInside = 0) const;

	//! Get some informations about the OS openGL capacities.
	//! This method needs to be called once at init
	static void initSystemGLInfo();
	
private:
	
	//! Project the passed triangle on the screen ensuring that it will look smooth, even for non linear distortion
	//! by splitting it into subtriangles. The resulting vertex arrays are appended to the passed out* ones.
	//! The size of each edge must be < 180 deg.
	//! @param vertices a pointer to an array of 3 vertices.
	//! @param edgeFlags a pointer to an array of 3 flags indicating whether the next segment is an edge.
	//! @param texturePos a pointer to an array of 3 texture coordinates, or NULL if the triangle should not be textured.
	void projectSphericalTriangle(const Vec3d* vertices, QVector<Vec3d>* outVertices,
								  	const bool* edgeFlags=NULL, QVector<bool>* outEdgeFlags=NULL,
		  							const Vec2d* texturePos=NULL, QVector<Vec2d>* outTexturePos=NULL,int nbI=0,
									bool checkDisc1=true, bool checkDisc2=true, bool checkDisc3=true) const;
	
	//! Switch to native OpenGL painting, i.e not using QPainter.
	//! After this call revertToQtPainting() MUST be called.
	void switchToNativeOpenGLPainting();

	//! Revert openGL state so that Qt painting works again.
	void revertToQtPainting();
	
	void drawTextGravity180(const StelFont* font, float x, float y, const QString& str, 
			      bool speedOptimize = 1, float xshift = 0, float yshift = 0) const;
		
	//! Init the real openGL Matrices to a 2d orthographic projection
	void initGlMatrixOrtho2d(void) const;
	
	//! The assoaciated instance of projector
	const StelProjectorP prj;
	
	//! Whether the GL_POINT_SPRITE extension is available and activated
	static bool flagGlPointSprite;
	
	//! Mutex allowing thread safety
	static class QMutex* globalMutex;
};

#endif // _STELPAINTER_HPP_

