/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
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

#ifndef _PROJECTOR_HPP_
#define _PROJECTOR_HPP_

#include "GLee.h"
#include "fixx11h.h"
#include "vecmath.h"
#include "Mapping.hpp"
#include "SphereGeometry.hpp"

#include <QString>
#include <QObject>
#include <QList>
#include <QMap>

class SFont;

//! @class Projector
//! Provides functions for drawing operations which are performed with some sort of 
//! "projection" according to the current projection mode.  This projection 
//! distorts the shape of the objects to be drawn and make the necessary calls 
//! to OpenGL to draw the required object. This class overrides a number of openGL 
//! functions to enable non-linear projection, such as fisheye or stereographic 
//! projections. This class also provide drawing primitives that are optimized 
//! according to the projection mode.
class Projector : public QObject
{
	Q_OBJECT;

public:
	
	///////////////////////////////////////////////////////////////////////////
	// Main constructor
	Projector(const Vector4<GLint>& viewport, double _fov = 60.);
	~Projector();
	
	//! Initialise the Projector.
	//! - Sets the viewport size according to the window/screen size and settings 
	//!   in the ini parser object.
	//! - Sets the maximum field of view for each projection type.
	//! - Register each projection type.
	//! - Sets the flag to use gravity labels or not according to the ini parser 
	//!   object.
	//! - Sets the default projection mode and field of view.
	//! - Sets whether to use GL points or a spite, according to the ini parser
	//!   object and the detected hardware capabilities.
	void init();
	
	//! Get the current state of the flag which decides whether to 
	//! arrage labels so that they are aligned with the bottom of a 2d 
	//! screen, or a 3d dome.
	bool getFlagGravityLabels() const { return gravityLabels; }
	
	//! Set up the view port dimensions and position.
	//! Define viewport size, center(relative to lower left corner)
	//! and diameter of FOV disk.
	//! @param x The x-position of the viewport.
	//! @param y The y-position of the viewport.
	//! @param w The width of the viewport.
	//! @param h The height of the viewport.
	//! @param cx The center of the viewport in the x axis (relative to left edge).
	//! @param cy The center of the viewport in the y axis (relative to bottom edge).
	//! @param fovDiam The field of view diameter.
	void setViewport(int x, int y, int w, int h, double cx, double cy, double fovDiam);

	//! Get the lower left corner of the viewport and the width, height.
	const Vector4<GLint>& getViewport(void) const {return viewportXywh;}

	//! Get the center of the viewport relative to the lower left corner.
	Vec2d getViewportCenter(void) const
	{
		return Vec2d(viewportCenter[0]-viewportXywh[0],viewportCenter[1]-viewportXywh[1]);
	}

	//! Get the diameter of the FOV disk in pixels
	double getViewportFovDiameter(void) const {return viewportFovDiameter;}
	
	//! Get the horizontal viewport offset in pixels.
	int getViewportPosX(void) const {return viewportXywh[0];}
	//! Get the vertical viewport offset in pixels.
	int getViewportPosY(void) const {return viewportXywh[1];}
	//! Get the viewport width in pixels.
	int getViewportWidth(void) const {return viewportXywh[2];}
	//! Get the viewport height in pixels.
	int getViewportHeight(void) const {return viewportXywh[3];}
	
	//! Get the maximum ratio between the viewport height and width
	float getViewportRatio() const {return getViewportWidth()>getViewportHeight() ? getViewportWidth()/getViewportHeight() : getViewportHeight()/getViewportWidth();}
	
	//! Handle the resizing of the window.
	void windowHasBeenResized(int width,int height);
	
	//! Return a convex polygon on the sphere which includes the viewport in the current frame.
	//! @param marginX an extra margin in pixel which extends the polygon size in the X direction
	//! @param marginY an extra margin in pixel which extends the polygon size in the Y direction
	//! @TODO Should be unified with unprojectViewport
	StelGeom::ConvexPolygon getViewportConvexPolygon(double marginX=0., double marginY=0.) const;

	//! Un-project the entire viewport depending on mapping, maskType,
	//! viewportFovDiameter, viewportCenter, and viewport dimensions.
	//! @TODO Should be unified with getViewportConvexPolygon
	StelGeom::ConvexS unprojectViewport(void) const;

	//! Set the near and far clipping planes.
	void setClippingPlanes(double znear, double zfar);
	//! Get the near and far clipping planes.
	void getClippingPlanes(double* zn, double* zf) const {*zn = zNear; *zf = zFar;}
	
	///////////////////////////////////////////////////////////////////////////
	// Methods for controlling the PROJECTION matrix
	
	//! Get whether front faces need to be oriented in the clockwise direction
	bool needGlFrontFaceCW(void) const {return (flipHorz*flipVert < 0.0);}

	//! Set the Field of View in degrees.
	void setFov(double f);
	//! Get the Field of View in degrees.
	double getFov(void) const {return fov;}
	
	//! Get size of a radian in pixels at the center of the viewport disk
	double getPixelPerRadAtCenter(void) const {return pixelPerRad;}

	//! Set the maximum field of View in degrees.
	void setMaxFov(double max);
	//! Get the maximum field of View in degrees.
	double getMaxFov(void) const {return maxFov;}
	//! Return the initial default FOV in degree.
	double getInitFov() const {return initFov;}
	
	
	///////////////////////////////////////////////////////////////////////////
	// Full projection methods
	//! Check to see if a 2d position is inside the viewport.
	//! TODO Optimize by storing viewportXywh[1] + viewportXywh[3] and viewportXywh[0] + viewportXywh[2] already computed
	bool checkInViewport(const Vec3d& pos) const
		{return (pos[1]>=viewportXywh[1] && pos[0]>=viewportXywh[0] &&
		pos[1]<=(viewportXywh[1] + viewportXywh[3]) && pos[0]<=(viewportXywh[0] + viewportXywh[2]));}

	//! Return the position where the 2 2D point p1 and p2 cross the viewport edge
	//! P1 must be inside the viewport and P2 outside (check with checkInViewport() before calling this method)
	Vec3d viewPortIntersect(const Vec3d& p1, const Vec3d& p2) const
	{
		Vec3d v1=p1;
		Vec3d v2=p2;
		Vec3d v;
		for (int i=0;i<8;++i)
		{
			v=(v1+v2)*0.5;
			if (!checkInViewport(v))
				v2=v;
			else
				v1=v;
		}
		return v;
	}
		
	//! Project the vector v from the current frame into the viewport.
	//! @param v the vector in the current frame.
	//! @param win the projected vector in the viewport 2D frame.
	//! @return true if the projected coordinate is valid.
	bool project(const Vec3d& v, Vec3d& win) const
	{
		// really important speedup:
		win[0] = modelViewMatrix.r[0]*v[0] + modelViewMatrix.r[4]*v[1]
				+ modelViewMatrix.r[8]*v[2] + modelViewMatrix.r[12];
		win[1] = modelViewMatrix.r[1]*v[0] + modelViewMatrix.r[5]*v[1]
				+ modelViewMatrix.r[9]*v[2] + modelViewMatrix.r[13];
		win[2] = modelViewMatrix.r[2]*v[0] + modelViewMatrix.r[6]*v[1]
				+ modelViewMatrix.r[10]*v[2] + modelViewMatrix.r[14];
		const bool rval = mapping->forward(win);
		// very important: even when the projected point comes from an
		// invisible region of the sky (rval=false), we must finish
		// reprojecting, so that OpenGl can successfully eliminate
		// polygons by culling.
		win[0] = viewportCenter[0] + flipHorz * pixelPerRad * win[0];
		win[1] = viewportCenter[1] + flipVert * pixelPerRad * win[1];
		win[2] = (win[2] - zNear) / (zNear - zFar);
		return rval;
	}

	//! Project the vector v from the current frame into the viewport.
	//! @param v the vector in the current frame.
	//! @param win the projected vector in the viewport 2D frame.
	//! @return true if the projected point is inside the viewport.
	bool projectCheck(const Vec3d& v, Vec3d& win) const {return (project(v, win) && checkInViewport(win));}

	//! Project the vector v from the viewport frame into the current frame.
	//! @param win the vector in the viewport 2D frame.
	//! @param v the projected vector in the current frame.
	//! @return true if the projected coordinate is valid.
	bool unProject(const Vec3d& win, Vec3d& v) const {return unProject(win[0], win[1], v);}
	bool unProject(double x, double y, Vec3d& v) const;

	//! Project the vectors v1 and v2 from the current frame into the viewport.
	//! @param v1 the first vector in the current frame.
	//! @param v2 the second vector in the current frame.
	//! @param win1 the first projected vector in the viewport 2D frame.
	//! @param win2 the second projected vector in the viewport 2D frame.
	//! @return true if at least one of the projected vector is within the viewport.
	bool projectLineCheck(const Vec3d& v1, Vec3d& win1, const Vec3d& v2, Vec3d& win2) const
		{return project(v1, win1) && project(v2, win2) && (checkInViewport(win1) || checkInViewport(win2));}

	//! Set a custom model view matrix.
	//! The new setting remains active until the next call to setCurrentFrame or setCustomFrame.
	//! @param m the openGL MODELVIEW matrix to use.
	void setModelViewMatrix(const Mat4d& m);
	Mat4d getModelViewMatrix() const {return modelViewMatrix;}

	//! Set the current projection mapping to use.
	//! The mapping must have been registered before being used.
	//! @param mappingId a string which can be e.g. "perspective", "stereographic", "fisheye", "cylinder".
	void setCurrentMapping(const QString& mappingId);

	//! Get the current Mapping used by the Projection
	const Mapping& getCurrentMapping(void) const {return *mapping;}

	//! Get the list of all the registered mappings
	//! @return a map associating each mappingId to its instance
	static QMap<QString,const Mapping*>& getAllMappings() {return projectionMappings;}
	
	//! Register a new projection mapping.
	static void registerProjectionMapping(Mapping *c);
	
	///////////////////////////////////////////////////////////////////////////
	//! @enum ProjectorMaskType Methods for controlling viewport and mask.
	enum ProjectorMaskType
	{
		Disk,	//!< For disk viewport mode (circular mask to seem like bins/telescope)
  		None	//!< Regular - no mask.
	};
	
	//! Get a string description of a ProjectorMaskType.
	static const QString maskTypeToString(ProjectorMaskType type);
	//! Get a ProjectorMaskType from a string description.
	static ProjectorMaskType stringToMaskType(const QString &s);
	
	//! Get the current type of the mask if any.
	ProjectorMaskType getMaskType(void) const {return maskType;}
	//! Set the mask type.
	void setMaskType(ProjectorMaskType m) {maskType = m; }
	
	///////////////////////////////////////////////////////////////////////////
	// Standard methods for drawing primitives in general (non-linear) mode
	///////////////////////////////////////////////////////////////////////////
	//! Fill with black around the viewport.
	void drawViewportShape(void);
	
	//! Generalisation of glVertex3v for non-linear projections. 
	//! This method does not manage the lighting operations properly.
	void drawVertex3v(const Vec3d& v) const
	{
		Vec3d win;
		project(v, win);
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
	void drawText(const SFont* font, float x, float y, const QString& str, float angleDeg=0.f, 
		      float xshift=0.f, float yshift=0.f, bool noGravity=true) const;
	
	//! Draw the given polygon
	//! @param poly The polygon to draw
	void drawPolygon(const StelGeom::Polygon& poly) const;
	
	//! Draw a small circle arc between points start and stop with rotation point in rotCenter
	//! The angle between start and stop must be < 180 deg
	//! Each time the small circle crosses the edge of the viewport, the viewportEdgeIntersectCallback is called with the
	//! screen 2d position, direction of the currently drawn arc toward the inside of the viewport
	void drawSmallCircleArc(const Vec3d& start, const Vec3d& stop, const Vec3d& rotCenter, void (*viewportEdgeIntersectCallback)(const Vec3d& screenPos, const Vec3d& direction, const void* userData)=NULL, const void* userData=NULL) const;
	
	//! Draw a parallel arc in the current frame.  The arc start from point start
	//! going in the positive longitude direction and with the given length in radian.
	//! @param start the starting position of the parallel in the current frame.
	//! @param length the angular length in radian (or distance on the unit sphere).
	//! @param labelAxis if true display a label indicating the latitude at begining and at the end of the arc.
	//! @param textColor color to use for rendering text. If NULL use the current openGL painting color.
	//! @param nbSeg if not==-1,indicate how many line segments should be used for drawing the arc, if==-1
	//! this value is automatically adjusted to prevent seeing the curve as a polygon.
	//! @param font the font to use for display
	void drawParallel(const Vec3d& start, double length, bool labelAxis=false, 
			  const SFont* font=NULL, const Vec4f* textColor=NULL, int nbSeg=-1) const;
	
	//! Draw a meridian arc in the current frame. The arc starts from point start
	//! going in the positive latitude direction if longitude is in [0;180], in the negative direction
	//! otherwise, and with the given length in radian. The length can be up to 2 pi.
	//! @param start the starting position of the meridian in the current frame.
	//! @param length the angular length in radian (or distance on the unit sphere).
	//! @param labelAxis if true display a label indicating the longitude at begining and at the end of the arc.
	//! @param textColor color to use for rendering text. If NULL use the current openGL painting color.
	//! @param nbSeg if not==-1,indicate how many line segments should be used for drawing the arc, if==-1
	//! this value is automatically adjusted to prevent seeing the curve as a polygon.
	//! @param font the font to use for display
	//! @param useDMS if true display label in DD:MM:SS. Normal is HH:MM:SS
	void drawMeridian(const Vec3d& start, double length, bool labelAxis=false, 
					  const SFont* font=NULL, const Vec4f* textColor=NULL, int nbSeg=-1, bool useDMS=false) const;

	//! draw a simple circle, 2d viewport coordinates in pixel
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

public slots:
	//! Set the flag with decides whether to arrage labels so that
	//! they are aligned with the bottom of a 2d screen, or a 3d dome.
	void setFlagGravityLabels(bool gravity) { gravityLabels = gravity; }
	//! Get the state of the horizontal flip.
	//! @return True if flipped horizontally, else false.
	bool getFlipHorz(void) const {return (flipHorz < 0.0);}
	//! Get the state of the vertical flip.
	//! @return True if flipped vertically, else false.
	bool getFlipVert(void) const {return (flipVert < 0.0);}
	//! Set the horizontal flip status.
	//! @param flip The new value (true = flipped, false = unflipped).
	void setFlipHorz(bool flip) {
		flipHorz = flip ? -1.0 : 1.0;
		glFrontFace(needGlFrontFaceCW()?GL_CW:GL_CCW); 
	}
	//! Set the vertical flip status.
	//! @param flip The new value (true = flipped, false = unflipped).
	void setFlipVert(bool flip) {
		flipVert = flip ? -1.0 : 1.0;
		glFrontFace(needGlFrontFaceCW()?GL_CW:GL_CCW); 
	}

	//! Set the initial field of view.  Updates configuration file.
	//! @param fov the new value for initial field of view in decimal degrees.
	void setInitFov(double fov) {initFov=fov;}

private:
	
	void drawTextGravity180(const SFont* font, float x, float y, const QString& str, 
			      bool speedOptimize = 1, float xshift = 0, float yshift = 0) const;
		
	//! Init the real openGL Matrices to a 2d orthographic projection
	void initGlMatrixOrtho2d(void) const;
	
	//! The current projector mask
	ProjectorMaskType maskType;

	double initFov;                // initial default FOV in degree
	double fov;                    // Field of view in degree
	double minFov;                 // Minimum fov in degree
	double maxFov;                 // Maximum fov in degree
	double zNear, zFar;            // Near and far clipping planes

	Vector4<GLint> viewportXywh;   // Viewport parameters
	Vec2d viewportCenter;          // Viewport center in screen pixel
	double viewportFovDiameter;    // diameter of the FOV disk in pixel
	double pixelPerRad;            // pixel per rad at the center of the viewport disk
	double flipHorz,flipVert;      // Whether to flip in horizontal or vertical directions
	bool gravityLabels;            // should label text align with the horizon?
	bool flagGlPointSprite;        // Whether the GL_POINT_SPRITE extension is available and activated
	
	Mat4d modelViewMatrix;         // openGL MODELVIEW Matrix
	
	const Mapping *mapping;
	QString currentProjectionType; // Type of the projection currently used
	
	// List of all the available projections
	static QMap<QString, const Mapping*> projectionMappings;
};

#endif // _PROJECTOR_HPP_

