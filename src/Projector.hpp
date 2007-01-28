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

#ifndef _PROJECTOR_H_
#define _PROJECTOR_H_

#include <vector>

#include "stellarium.h"
#include "vecmath.h"
#include "SFont.hpp"
#include "callbacks.hpp"

class InitParser;
class Mapping;

//! Class which handle projection in stellarium. It overrides a number of openGL functions
//! to enable non-linear projection, such as fisheye or stereographic projections.
//! This class also provide drawing primitives that are optimized according to the projection mode.
class Projector
{
public:
	//! Supported reference frame types
	enum FRAME_TYPE
	{
		FRAME_LOCAL,
		FRAME_HELIO,
		FRAME_EARTH_EQU,
		FRAME_J2000
	};
	
    ///////////////////////////////////////////////////////////////////////////
    // Main constructor
	Projector(const Vec4i& viewport, double _fov = 60.);
	~Projector();
	
	void init(const InitParser& conf);
	
	// Set the standard modelview matrices used for projection
	void set_modelview_matrices(const Mat4d& _mat_earth_equ_to_eye,
				    const Mat4d& _mat_helio_to_eye,
				    const Mat4d& _mat_local_to_eye,
				    const Mat4d& _mat_j2000_to_eye);	
	// Flags
	void setFlagGravityLabels(bool gravity) { gravityLabels = gravity; }
	bool getFlagGravityLabels() const { return gravityLabels; }

	
	//! Register a new projection mapping
	template<class MappingClass> void registerProjectionMapping(MappingClass c)
	{
		projectionMapping[c.getName()] = c.getMapping();
	}
	
	///////////////////////////////////////////////////////////////////////////
	// Methods for controlling viewport and mask 
	enum PROJECTOR_MASK_TYPE
	{
		DISK,
		NONE
	};
	
	static const char *maskTypeToString(PROJECTOR_MASK_TYPE type);
    static PROJECTOR_MASK_TYPE stringToMaskType(const string &s);
    
	//! Get the type of the mask if any
	PROJECTOR_MASK_TYPE getMaskType(void) const {return maskType;}
	void setMaskType(PROJECTOR_MASK_TYPE m) {maskType = m; }
	
	//! Get and set to define and get viewport size
	void setViewport(int x, int y, int w, int h);
	void setViewport(const Vec4i& v) {setViewport(v[0], v[1], v[2], v[3]);}
	const Vec4i& getViewport(void) const {return vec_viewport;}
	
	//! Set the horizontal viewport offset in pixels
	void setViewportPosX(int x) {setViewport(x, vec_viewport[1], vec_viewport[2], vec_viewport[3]);}
	int getViewportPosX(void) const {return vec_viewport[0];}
	
	//! Get/Set the vertical viewport offset in pixels
	void setViewportPosY(int y) {setViewport(vec_viewport[0], y, vec_viewport[2], vec_viewport[3]);}
	int getViewportPosY(void) const {return vec_viewport[1];}
	
	//! Get/Set the viewport size in pixels
	void setViewportWidth(int width) {setViewport(vec_viewport[0], vec_viewport[1], width, vec_viewport[3]);}
	void setViewportHeight(int height) {setViewport(vec_viewport[0], vec_viewport[1], vec_viewport[2], height);}
	int getViewportWidth(void) const {return vec_viewport[2];}
	int getViewportHeight(void) const {return vec_viewport[3];}
	
	//! Return a polygon matching precisely the real viewport defined by the area on the screen 
	//! where projection is valid. Normally, nothing should be drawn outside this area.
	//! This viewport is usually the rectangle defined by the screen, but in case of non-linear
	//! projection, it can also be a more complex shape.
	std::vector<Vec2d> getViewportVertices() const;
	
	//! Maximize viewport according to passed screen values
	void setMaximizedViewport(int screenW, int screenH) {setViewport(0, 0, screenW, screenH);}

	//! Set a centered squared viewport with passed vertical and horizontal offset
	void setSquareViewport(int screenW, int screenH, int hoffset, int voffset)
	{
		int m = MY_MIN(screenW, screenH);
		setViewport((screenW-m)/2+hoffset, (screenH-m)/2+voffset, m, m);
	}
	
	//! Set whether a disk mask must be drawn over the viewport
	void setViewportMaskDisk(void) {setMaskType(Projector::DISK);}
	//! Get whether a disk mask must be drawn over the viewport
	bool getViewportMaskDisk(void) const {return getMaskType()==Projector::DISK;}

	//! Set whether no mask must be drawn over the viewport
	void setViewportMaskNone(void) {setMaskType(Projector::NONE);}
	
	//! Set the current openGL viewport to projector's viewport
	void applyViewport(void) const {glViewport(vec_viewport[0], vec_viewport[1], vec_viewport[2], vec_viewport[3]);}	
	
	void set_clipping_planes(double znear, double zfar);
	void get_clipping_planes(double* zn, double* zf) const {*zn = zNear; *zf = zFar;}
	
	///////////////////////////////////////////////////////////////////////////
	// Methods for controlling the PROJECTION matrix
	bool getFlipHorz(void) const {return (flip_horz < 0.0);}
	bool getFlipVert(void) const {return (flip_vert < 0.0);}
	void setFlipHorz(bool flip) {
		flip_horz = flip ? -1.0 : 1.0;
		glFrontFace(needGlFrontFaceCW()?GL_CW:GL_CCW); 
	}
	void setFlipVert(bool flip) {
		flip_vert = flip ? -1.0 : 1.0;
		glFrontFace(needGlFrontFaceCW()?GL_CW:GL_CCW); 
	}
	bool needGlFrontFaceCW(void) const
		{return (flip_horz*flip_vert < 0.0);}

	//! Set the Field of View in degree
	void setFov(double f);
	//! Get the Field of View in degree
	double getFov(void) const {return fov;}
	double getPixelPerRad(void) const {return view_scaling_factor;}

	//! Set the maximum Field of View in degree
	void setMaxFov(double max);
	//! Get the maximum Field of View in degree
	double getMaxFov(void) const {return max_fov;}
	//! Return the initial default FOV in degree
	double getInitFov() const {return initFov;}
	

	///////////////////////////////////////////////////////////////////////////
	// Full projection methods
	// Return true if the 2D pos is inside the viewport
	bool check_in_viewport(const Vec3d& pos) const
		{return (pos[1]>=vec_viewport[1] && pos[1]<=(vec_viewport[1] + vec_viewport[3]) &&
				pos[0]>=vec_viewport[0] && pos[0]<=(vec_viewport[0] + vec_viewport[2]));}

	//! Project the vector v from the current frame into the viewport
	//! @param v the vector in the current frame
	//! @param win the projected vector in the viewport 2D frame
	//! @return true if the projected coordinate is valid
	bool project(const Vec3d& v, Vec3d& win) const;

	//! Project the vector v from the current frame into the viewport
	//! @param v the vector in the current frame
	//! @param win the projected vector in the viewport 2D frame
	//! @return true if the projected point is inside the viewport
	bool projectCheck(const Vec3d& v, Vec3d& win) const {return (project(v, win) && check_in_viewport(win));}

	//! Project the vector v from the viewport frame into the current frame 
	//! @param win the vector in the viewport 2D frame
	//! @param v the projected vector in the current frame
	//! @return true if the projected coordinate is valid
	bool unProject(const Vec3d& win, Vec3d& v) const {return unProject(win[0], win[1], v);}
	bool unProject(double x, double y, Vec3d& v) const;

	//! Project the vectors v1 and v2 from the current frame into the viewport.
	//! @param v1 the first vector in the current frame
	//! @param v2 the second vector in the current frame
	//! @param win1 the first projected vector in the viewport 2D frame
	//! @param win2 the second projected vector in the viewport 2D frame
	//! @return true if at least one of the projected vector is within the viewport
	bool projectLineCheck(const Vec3d& v1, Vec3d& win1, const Vec3d& v2, Vec3d& win2) const
		{return project(v1, win1) && project(v2, win2) && (check_in_viewport(win1) || check_in_viewport(win2));}

	//! Set the frame in which we want to draw from now on
	//! The frame will be the current one until this method or setCustomFrame is called again
	//! @param frameType the type
	void setCurrentFrame(FRAME_TYPE frameType) const;

	//! Set a custom model view matrix, it is valid until the next call to setCurrentFrame or setCustomFrame
	//! @param the openGL MODELVIEW matrix to use
	void setCustomFrame(const Mat4d&) const;

	//! Set the current projection mapping to use
	//! The mapping must have been registered before beeing used
	//! @param projectionName a string which can be e.g. "perspective", "stereographic", "fisheye", "cylinder"
	void setCurrentProjection(const std::string& projectionName);
	
	//! Get the current projection mapping name
	std::string getCurrentProjection() {return currentProjectionType;}
	
	//! Set the drawing mode in 2D for drawing inside the viewport only.
	//! All the drawing primitives of the Projection class won't work properly
	//! until the normal mode is reset by calling the unset2dDrawMode() method
	//! Use unset2dDrawMode() to restore previous projection mode
	void set2dDrawMode(void) const;

	// Restore the previous projection mode after a call to set2dDrawMode()
	void unset2dDrawMode(void) const;
		   

	///////////////////////////////////////////////////////////////////////////
	// Standard methods for drawing primitives in general (non-linear) mode
	///////////////////////////////////////////////////////////////////////////

	// Fill with black around the viewport
	void draw_viewport_shape(void);
	
	//! Generalisation of glVertex3v.
 	//! This method assumes that we are in orthographic projection mode, which is true for special projections.
	void drawVertex3v(const Vec3d& v) const;
	void drawVertex3(double x, double y, double z) const {drawVertex3v(Vec3d(x, y, z));}
	
	// Draw a disk with a special texturing mode having texture center at center
	void sDisk(GLdouble radius, GLint slices, GLint stacks, int orient_inside = 0) const;	
		
	// Draw a ring with a radial texturing
	void sRing(GLdouble r_min, GLdouble r_max, GLint slices, GLint stacks, int orient_inside) const;

	// Draw a fisheye texture in a sphere
	void sSphere_map(GLdouble radius, GLint slices, GLint stacks, double texture_fov = 2.*M_PI, int orient_inside = 0) const;

	void drawTextGravity180(SFont* font, float x, float y, const wstring& str, 
			      bool speed_optimize = 1, float xshift = 0, float yshift = 0) const;
	void drawTextGravity180(SFont* font, float x, float y, const string& str, 
			      bool speed_optimize = 1, float xshift = 0, float yshift = 0) const
		{drawTextGravity180(font, x, y, StelUtils::stringToWstring(str), speed_optimize, xshift, yshift);}

	//! Draw the string at the given position and angle with the given font
	//! @param x horizontal position of the lower left corner of the first character of the text in pixel
	//! @param y horizontal position of the lower left corner of the first character of the text in pixel
	//! @param str the text to print
	//! @param angleDeg rotation angle in degree. Rotation is around x,y
	//! @param xshift shift in pixel in the rotated x direction
	//! @param yshift shift in pixel in the rotated y direction
	void drawText(const SFont* font, float x, float y, const string& str, float angleDeg=0.f, float xshift=0.f, float yshift=0.f) const;

	//! Draw a parallel arc in the current frame, starting from point start
	//!  going in the positive longitude direction and with the given length in radian.
	//! @param start the starting position of the parallel in the current frame
	//! @param length the angular length in radian (or distance on the unit sphere)
	//! @param labelAxis if true display a label indicating the latitude at begining and at the end of the arc
	//! @param nbSeg if not==-1,indicate how many line segments should be used for drawing the arc, if==-1
	//!   this value is automatically adjusted to prevent seeing the curve as a polygon
	void drawParallel(const Vec3d& start, double length, bool labelAxis=false, const SFont* font=NULL, int nbSeg=-1) const;
	
	//! Draw a meridian arc in the current frame, starting from point start
	//!  going in the positive latitude direction if longitude is in [0;180], in the negative direction
	//!  otherwise, and with the given length in radian. The length can be up to 2 pi.
	//! @param start the starting position of the meridian in the current frame
	//! @param length the angular length in radian (or distance on the unit sphere)
	//! @param labelAxis if true display a label indicating the longitude at begining and at the end of the arc
	//! @param nbSeg if not==-1,indicate how many line segments should be used for drawing the arc, if==-1
	//!  this value is automatically adjusted to prevent seeing the curve as a polygon
	void drawMeridian(const Vec3d& start, double length, bool labelAxis=false, const SFont* font=NULL, int nbSeg=-1) const;

	//! Draw a square using the current texture at the given position
	//! @param pos the center of the sprite in the current frame
	//! @param size the size of a square side in pixel
	void drawSprite(const Vec3d& pos, double size) const;
	
	//! Draw a square using the current texture at the given projected 2d position
	//! @param x x position in the viewport in pixel
	//! @param y y position in the viewport in pixel
	//! @param size the size of a square side in pixel
	void drawSprite2dMode(double x, double y, double size) const;
	
	//! Draw a rotated square using the current texture at the given projected 2d position
	//! @param x x position in the viewport in pixel
	//! @param y y position in the viewport in pixel
	//! @param size the size of a square side in pixel
	//! @param rotation rotation angle in degree
	void drawSprite2dMode(double x, double y, double size, double rotation) const;
	
	// Reimplementation of gluSphere : glu is overrided for non standard projection
	void sSphere(GLdouble radius, GLdouble one_minus_oblateness, GLint slices, GLint stacks, int orient_inside = 0) const;

	// Reimplementation of gluCylinder : glu is overrided for non standard projection
	void sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks, int orient_inside = 0) const;

	///////////////////////////////////////////////////////////////////////////
	// Methods for linear mode
	///////////////////////////////////////////////////////////////////////////
	
	// Reimplementation of gluCylinder
	void sCylinderLinear(GLdouble radius, GLdouble height, GLint slices, GLint stacks, int orient_inside = 0) const;
	
	// Reimplementation of gluSphere
	void sSphereLinear(GLdouble radius, GLdouble one_minus_oblateness, GLint slices, GLint stacks, int orient_inside = 0) const;


private:

	// Init the viewing matrix from the fov, the clipping planes and screen ratio
	// The function is a reimplementation of gluPerspective
	void init_project_matrix(void);

	//! The current projector mask
	PROJECTOR_MASK_TYPE maskType;

	double initFov;				// initial default FOV in degree
	double fov;					// Field of view in degree
	double min_fov;				// Minimum fov in degree
	double max_fov;				// Maximum fov in degree
	double zNear, zFar;			// Near and far clipping planes
	Vec4i vec_viewport;			// Viewport parameters
	Mat4d mat_projection;		// Projection matrix

	Vec3d center;				// Viewport center in screen pixel
	double view_scaling_factor;	// ??
	double flip_horz,flip_vert;	// Whether to flip in horizontal or vertical directions

	Mat4d mat_earth_equ_to_eye;		// Modelview Matrix for earth equatorial projection
	Mat4d mat_j2000_to_eye;         // for precessed equ coords
	Mat4d mat_helio_to_eye;			// Modelview Matrix for earth equatorial projection
	Mat4d mat_local_to_eye;			// Modelview Matrix for earth equatorial projection
	Mat4d inv_mat_earth_equ_to_eye;	// Inverse of mat_projection*mat_earth_equ_to_eye
	Mat4d inv_mat_j2000_to_eye;		// Inverse of mat_projection*mat_earth_equ_to_eye
	Mat4d inv_mat_helio_to_eye;		// Inverse of mat_projection*mat_helio_to_eye
	Mat4d inv_mat_local_to_eye;		// Inverse of mat_projection*mat_local_to_eye
	
	bool gravityLabels;			// should label text align with the horizon?
	
	mutable Mat4d modelViewMatrix;			// openGL MODELVIEW Matrix
	mutable Mat4d inverseModelViewMatrix;	// inverse of it
	
	// Callbacks
	boost::callback<bool, Vec3d&> projectForward;
	boost::callback<bool, Vec3d&> projectBackward;
	
	std::map<std::string, Mapping> projectionMapping;
	
	std::string currentProjectionType;	// Type of the projection currently used
};

#endif // _PROJECTOR_H_
