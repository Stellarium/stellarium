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

#include "stellarium.h"
#include "vecmath.h"
#include "SFont.hpp"
#include "Mapping.hpp"
#include "callbacks.hpp"

class InitParser;

// Class which handle projection modes and projection matrix
// Overide some function usually handled by glu
class Projector
{
public:
	enum PROJECTOR_TYPE
	{
		PERSPECTIVE_PROJECTOR    = 0,
        ORTHOGRAPHIC_PROJECTOR   = 1,
        EQUAL_AREA_PROJECTOR     = 2,
		FISHEYE_PROJECTOR        = 3,
        STEREOGRAPHIC_PROJECTOR  = 4,
		CYLINDER_PROJECTOR       = 5,
        SPHERIC_MIRROR_PROJECTOR = 6
	};
	
    ///////////////////////////////////////////////////////////////////////////
    // Main factory constructor
    static Projector *create(PROJECTOR_TYPE type, const Vec4i& viewport, double _fov = 60.);
	virtual ~Projector();
	void init(const InitParser& conf);
	// Set the standard modelview matrices used for projection
	void set_modelview_matrices(const Mat4d& _mat_earth_equ_to_eye,
				    const Mat4d& _mat_helio_to_eye,
				    const Mat4d& _mat_local_to_eye,
				    const Mat4d& _mat_j2000_to_eye);	
	// Flags
	void setFlagGravityLabels(bool gravity) { gravityLabels = gravity; }
	bool getFlagGravityLabels() const { return gravityLabels; }
	
	///////////////////////////////////////////////////////////////////////////
	// Methods for choosing PROJECTION matrix type
    static const char *typeToString(PROJECTOR_TYPE type);
    static PROJECTOR_TYPE stringToType(const string &s);
    
   	virtual PROJECTOR_TYPE getType(void) const {return PERSPECTIVE_PROJECTOR;}
   	//! Get the projection type
	string getProjectionType(void) const {return Projector::typeToString(getType());}
	
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
	virtual void setViewport(int x, int y, int w, int h);
	void setViewportSize(int w, int h)
	{
		if (w==getViewportWidth() && h==getViewportHeight())
			return;
		setViewportWidth(w);
		setViewportHeight(h);
	}
	void setViewport(const Vec4i& v) {setViewport(v[0], v[1], v[2], v[3]);}
	
	//! Set the horizontal viewport offset in pixels
	void setViewportPosX(int x) {setViewport(x, vec_viewport[1], vec_viewport[2], vec_viewport[3]);}
	int getViewportPosX(void) const {return vec_viewport[0];}
	
	//! Get/Set the vertical viewport offset in pixels
	void setViewportPosY(int y) {setViewport(vec_viewport[0], y, vec_viewport[2], vec_viewport[3]);}
	int getViewportPosY(void) const {return vec_viewport[1];}
	
	void setViewportWidth(int width) {setViewport(vec_viewport[0], vec_viewport[1], width, vec_viewport[3]);}
	void setViewportHeight(int height) {setViewport(vec_viewport[0], vec_viewport[1], vec_viewport[2], height);}
	

	int getViewportWidth(void) const {return vec_viewport[2];}
	int getViewportHeight(void) const {return vec_viewport[3];}
	const Vec4i& getViewport(void) const {return vec_viewport;}
	
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
		init_project_matrix();
		glFrontFace(needGlFrontFaceCW()?GL_CW:GL_CCW); 
	}
	void setFlipVert(bool flip) {
		flip_vert = flip ? -1.0 : 1.0;
		init_project_matrix();
		glFrontFace(needGlFrontFaceCW()?GL_CW:GL_CCW); 
	}
	virtual bool needGlFrontFaceCW(void) const
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
		{return (pos[1]>vec_viewport[1] && pos[1]<(vec_viewport[1] + vec_viewport[3]) &&
				pos[0]>vec_viewport[0] && pos[0]<(vec_viewport[0] + vec_viewport[2]));}

	//! Project the vector v from the current frame into the viewport
	//! @param v the vector in the current frame
	//! @param win the projected vector in the viewport 2D frame
	//! @return true if the projected coordinate is valid
	inline bool project(const Vec3d& v, Vec3d& win) const;

	//! Project the vector v from the viewport frame into the current frame 
	//! @param win the vector in the viewport 2D frame
	//! @param v the projected vector in the current frame
	//! @return true if the projected coordinate is valid
	inline bool unProject(const Vec3d& win, Vec3d& v) const;

	//! Project the vector v from the current frame into the viewport
	//! @param v the vector in the current frame
	//! @param win the projected vector in the viewport 2D frame
	//! @return true if the projected point is inside the viewport
	inline bool projectCheck(const Vec3d& v, Vec3d& win) const;

	//! Project the vectors v1 and v2 from the current frame into the viewport.
	//! @param v1 the first vector in the current frame
	//! @param v2 the second vector in the current frame
	//! @param win1 the first projected vector in the viewport 2D frame
	//! @param win2 the second projected vector in the viewport 2D frame
	//! @return true if at least one of the projected vector is within the viewport
	inline bool projectLineCheck(const Vec3d& v1, Vec3d& win1, const Vec3d& v2, Vec3d& win2) const;

	//! Set the frame in which we want to draw from now on
	//! The frame will be the current one until this method is called again
	//! @param frameName a string which can be e.g. "local", "helio", "earthequ", "j2000"
	void setCurrentFrame(const std::string& frameName);

	//! Set the current projection mapping to use
	//! The mapping must have been registered before beeing used
	//! @param projectionName a string which can be e.g. "perspective", "stereographic", "fisheye", "cylinder"
	void setCurrentProjection(const std::string& projectionName);
	
	// Set the drawing mode in 2D for drawing inside the viewport only.
	// Use reset_perspective_projection() to restore previous projection mode
	void set_orthographic_projection(void) const;

	// Restore the previous projection mode after a call to set_orthographic_projection()
	void reset_perspective_projection(void) const;
	
	// Return in vector "win" the projection on the screen of point v in earth equatorial coordinate
	// according to the current modelview and projection matrices (reimplementation of gluProject)
	// Return true if the z screen coordinate is < 1, ie if it isn't behind the observer
	// except for the _check version which return true if the projected point is inside the screen
	inline bool project_earth_equ(const Vec3d& v, Vec3d& win) const
		{return project_custom(v, win, mat_earth_equ_to_eye);}

	inline bool project_earth_equ_check(const Vec3d& v, Vec3d& win) const
		{return project_custom_check(v, win, mat_earth_equ_to_eye);}

	inline bool project_earth_equ_line_check(const Vec3d& v1, Vec3d& win1, const Vec3d& v2, Vec3d& win2) const
		{return project_custom_line_check(v1, win1, v2, win2, mat_earth_equ_to_eye);}

	inline void unproject_earth_equ(double x, double y, Vec3d& v) const
		{unproject(x, y, inv_mat_earth_equ_to_eye, v);}

	inline void unproject_j2000(double x, double y, Vec3d& v) const
		{unproject(x, y, inv_mat_j2000_to_eye, v);}

	// taking account of precession
	inline bool project_j2000(const Vec3d& v, Vec3d& win) const
		{return project_custom(v, win, mat_j2000_to_eye);}

	inline bool project_j2000_check(const Vec3d& v, Vec3d& win) const
		{return project_custom_check(v, win, mat_j2000_to_eye);}

	inline bool project_j2000_line_check(const Vec3d& v1, Vec3d& win1, const Vec3d& v2, Vec3d& win2) const
		{return project_custom_line_check(v1, win1, v2, win2, mat_j2000_to_eye);}

	// Same function with input vector v in heliocentric coordinate
	inline bool project_helio_check(const Vec3d& v, Vec3d& win) const
		{return project_custom_check(v, win, mat_helio_to_eye);}

	inline bool project_helio(const Vec3d& v, Vec3d& win) const
		{return project_custom(v, win, mat_helio_to_eye);}

	inline bool project_helio_line_check(const Vec3d& v1, Vec3d& win1, const Vec3d& v2, Vec3d& win2) const
		{return project_custom_line_check(v1, win1, v2, win2, mat_helio_to_eye);}

	inline void unproject_helio(double x, double y, Vec3d& v) const
		{return unproject(x, y, inv_mat_helio_to_eye, v);}

	// Same function with input vector v in local coordinate
	inline bool project_local(const Vec3d& v, Vec3d& win) const
		{return project_custom(v, win, mat_local_to_eye);}

	inline bool project_local_check(const Vec3d& v, Vec3d& win) const
		{return project_custom_check(v, win, mat_local_to_eye);}

	inline void unproject_local(double x, double y, Vec3d& v) const
		{unproject(x, y, inv_mat_local_to_eye, v);}
		
	// Same function but using a custom modelview matrix
	virtual bool project_custom(const Vec3d& v, Vec3d& win, const Mat4d& mat) const
	{
		gluProject(v[0],v[1],v[2],mat,mat_projection,vec_viewport,&win[0],&win[1],&win[2]);
		return (win[2]<1.);
	}

    bool project_custom_check(const Vec3f& v, Vec3d& win, const Mat4d& mat) const
		{return (project_custom(v, win, mat) && check_in_viewport(win));}
	// project two points and make sure both are in front of viewer and that at least one is on screen

    bool project_custom_line_check(const Vec3f& v1, Vec3d& win1, 
					       const Vec3f& v2, Vec3d& win2, const Mat4d& mat) const
		{return project_custom(v1, win1, mat) && project_custom(v2, win2, mat) && 
		   (check_in_viewport(win1) || check_in_viewport(win2));}
		   

	///////////////////////////////////////////////////////////////////////////
	// Standard methods for drawing primitives
	
	///////////////////////////////////////////////////////////////////////////
	// Callback versions
	boost::callback<void, Vec3d&> sVertex3v;
	boost::callback<void, double, double> sVertex2;
	
	///////////////////////////////////////////////////////////////////////////
	
	// Fill with black around the viewport
	void draw_viewport_shape(void);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods for linear mode
	
	// Reimplementation of gluSphere : glu is overrided for non standard projection
	virtual void sSphere(GLdouble radius, GLdouble one_minus_oblateness,
		GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;

	// Draw a disk with a special texturing mode having texture center at center
	virtual void sDisk(GLdouble radius, GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;	
		
	// Draw a ring with a radial texturing
	virtual void sRing(GLdouble r_min, GLdouble r_max,
	                   GLint slices, GLint stacks,
	                   const Mat4d& mat, int orient_inside) const;

	// Draw a fisheye texture in a sphere
	virtual void sSphere_map(GLdouble radius, GLint slices, GLint stacks,
		const Mat4d& mat, double texture_fov = 2.*M_PI, int orient_inside = 0) const;

	// Reimplementation of gluCylinder : glu is overrided for non standard projection
	virtual void sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;

	virtual void sVertex3(double x, double y, double z, const Mat4d& mat) const
	{glVertex3d(x,y,z);}

	void print_gravity180(SFont* font, float x, float y, const wstring& str, 
			      bool speed_optimize = 1, float xshift = 0, float yshift = 0) const;
	void print_gravity180(SFont* font, float x, float y, const string& str, 
			      bool speed_optimize = 1, float xshift = 0, float yshift = 0) const
	{
	   	print_gravity180(font, x, y, StelUtils::stringToWstring(str), speed_optimize, xshift, yshift);
	}

	void drawParallelJ2000(const Vec3d& start, double length) const;


	///////////////////////////////////////////////////////////////////////////
	// Drawing methods for general (non-linear) mode
	
	// Reimplementation of gluSphere : glu is overrided for non standard projection
	void sSphereGeneral(GLdouble radius, GLdouble one_minus_oblateness,
		GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;

	// Reimplementation of gluCylinder : glu is overrided for non standard projection
	void sCylinderGeneral(GLdouble radius, GLdouble height, GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;

	// Override glVertex3f and glVertex3d
	void sVertex3General(double x, double y, double z, const Mat4d& mat) const;



protected:
	Projector(const Vec4i& viewport, double _fov = 60.);

	// Init the viewing matrix from the fov, the clipping planes and screen ratio
	// The function is a reimplementation of gluPerspective
	virtual void init_project_matrix(void);

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
	
	// transformation from screen 2D point x,y to object
	// m is here the already inverted full tranfo matrix
	// assertion: the length of the output vector is always = 1.0
	virtual
    void unproject(double x, double y, const Mat4d& m, Vec3d& v) const
	{
		v.set(	(x - vec_viewport[0]) * 2. / vec_viewport[2] - 1.0,
				(y - vec_viewport[1]) * 2. / vec_viewport[3] - 1.0,
				1.0);
		v.transfo4d(m);
		v.normalize();
	}
	bool gravityLabels;			// should label text align with the horizon?
	
	
	Mat4d modelViewMatrix;			// openGL MODELVIEW Matrix
	Mat4d inverseModelViewMatrix;	// inverse of it
	
	// Callbacks
	boost::callback<bool, Vec3d&> projectForward;
	boost::callback<bool, Vec3d&> projectBackward;
	
	std::map<std::string, Mapping> projectionMapping;
};

#endif // _PROJECTOR_H_
