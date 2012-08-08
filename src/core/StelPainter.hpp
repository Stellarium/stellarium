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
	friend class VertexArrayProjector;

	//! Define the shade model when interpolating polygons
	enum ShadeModel
	{
		ShadeModelFlat=0x1D00,		//!< GL_FLAT
		ShadeModelSmooth=0x1D01	//!< GL_SMOOTH
	};

	//! Define the drawing mode when drawing vertex
	enum DrawingMode
	{
		Points                      = 0x0000, //!< GL_POINTS
		Lines                       = 0x0001, //!< GL_LINES
		LineLoop                    = 0x0002, //!< GL_LINE_LOOP
		LineStrip                   = 0x0003, //!< GL_LINE_STRIP
		Triangles                   = 0x0004, //!< GL_TRIANGLES
		TriangleStrip               = 0x0005, //!< GL_TRIANGLE_STRIP
		TriangleFan                 = 0x0006  //!< GL_TRIANGLE_FAN
	};

	explicit StelPainter(const StelProjectorP& prj);
	~StelPainter();

	//! Return the instance of projector associated to this painter
	const StelProjectorP& getProjector() const {return prj;}
	void setProjector(const StelProjectorP& p);

	//! Draw the string at the given position and angle with the given font.
	//! If the gravity label flag is set, uses drawTextGravity180.
	//! @param x horizontal position of the lower left corner of the first character of the text in pixel.
	//! @param y horizontal position of the lower left corner of the first character of the text in pixel.
	//! @param str the text to print.
	//! @param angleDeg rotation angle in degree. Rotation is around x,y.
	//! @param xshift shift in pixel in the rotated x direction.
	//! @param yshift shift in pixel in the rotated y direction.
	//! @param noGravity don't take into account the fact that the text should be written with gravity.
	void drawText(float x, float y, const QString& str, float angleDeg=0.f,
			  float xshift=0.f, float yshift=0.f, bool noGravity=true);
	void drawText(const Vec3d& v, const QString& str, float angleDeg=0.f,
			  float xshift=0.f, float yshift=0.f, bool noGravity=true);

	//! Draw the given SphericalRegion.
	//! @param region The SphericalRegion to draw.
	//! @param drawMode define whether to draw the outline or the fill or both.
	//! @param clippingCap if not set to NULL, tells the painter to try to clip part of the region outside the cap.
	//! @param doSubDivise if true tesselates the object to follow projection distortions.
	//! Typically set that to false if you think that the region is fully contained in the viewport.
	void drawSphericalRegion(const SphericalRegion* region, 
	                         SphericalPolygonDrawMode drawMode=SphericalPolygonDrawModeFill,
	                         const SphericalCap* clippingCap=NULL, 
	                         bool doSubDivise=true,
	                         double maxSqDistortion=5.);

	//! Set the font to use for subsequent text drawing.
	void setFont(const QFont& font);

	//! Set the color to use for subsequent drawing.
	void setColor(float r, float g, float b, float a=1.f);

	//! Get the color currently used for drawing.
	Vec4f getColor() const;

	//! Get the font metrics for the current font.
	QFontMetrics getFontMetrics() const;

	//! Get some informations about the OS openGL capacities and set the GLContext which will be used by Stellarium.
	//! This method needs to be called once at init.
	static void initSystemGLInfo(QGLContext* ctx);

	//GL-REFACTOR: Sets shader globals used by StelPainter. 
	//Will be removed as StelPainter is refactored.
#ifdef STELPAINTER_GL2
	static void TEMPSpecifyShaders(QGLShaderProgram *plain,
	                               QGLShaderProgram *color,
	                               QGLShaderProgram *texture,
	                               QGLShaderProgram *colorTexture);
#endif
	
	//! Set the QPainter to use for performing some drawing operations.
	static void setQPainter(QPainter* qPainter);

	//! Swap the OpenGL buffers. You normally don't need to do that.
	static void swapBuffer();

	//! Make sure that our GL context is current and valid.
	static void makeMainGLContextCurrent();

	// The following methods try to reflect the API of the incoming QGLPainter class

	//! Sets the point size to use with draw().
	//! This function has no effect if a shader program is in use, or on OpenGL/ES 2.0. Shader programs must set the
	//! point size in the vertex shader.
	void setPointSize(qreal size);

	//! Define the current shade model used when interpolating between vertex.
	void setShadeModel(ShadeModel m);

	//! Set whether texturing is enabled.
	void enableTexture2d(bool b);

	// Thoses methods should eventually be replaced by a single setVertexArray
	//! use instead of glVertexPointer
	void setVertexPointer(int size, int type, const void* pointer) {
		vertexArray.size = size; vertexArray.type = type; vertexArray.pointer = pointer;
	}

	//! use instead of glTexCoordPointer
	void setTexCoordPointer(int size, int type, const void* pointer)
	{
		texCoordArray.size = size; texCoordArray.type = type; texCoordArray.pointer = pointer;
	}

	//! use instead of glColorPointer
	void setColorPointer(int size, int type, const void* pointer)
	{
		colorArray.size = size; colorArray.type = type; colorArray.pointer = pointer;
	}

	//! use instead of glNormalPointer
	void setNormalPointer(int type, const void* pointer)
	{
		normalArray.size = 3; normalArray.type = type; normalArray.pointer = pointer;
	}

	//! use instead of glEnableClient
	void enableClientStates(bool vertex, bool texture=false, bool color=false, bool normal=false);

	//! convenience method that enable and set all the given arrays.
	//! It is equivalent to calling enableClientState and set the array pointer for each arrays.
	void setArrays(const Vec3d* vertice, const Vec2f* texCoords=NULL, const Vec3f* colorArray=NULL, const Vec3f* normalArray=NULL);

	//! Draws primitives using vertices from the arrays specified by setVertexArray().
	//! The type of primitive to draw is specified by mode.
	//! If indices is NULL, this operation will consume count values from the enabled arrays, starting at offset.
	//! Else it will consume count elements of indices, starting at offset, which are used to index into the
	//! enabled arrays.
	void drawFromArray(DrawingMode mode, int count, int offset=0, bool doProj=true, const unsigned int* indices=NULL);

	//! Draws the primitives defined in the StelVertexArray.
	//! @param checkDiscontinuity will check and suppress discontinuities if necessary.
	void drawStelVertexArray(const StelVertexArray& arr, bool checkDiscontinuity=true);

private:

	friend class StelTextureMgr;
	friend class StelTexture;
	//! Struct describing one opengl array
	typedef struct
	{
		int size;				// The number of coordinates per vertex.
		int type;				// The data type of each coordinate (GL_SHORT, GL_INT, GL_FLOAT, or GL_DOUBLE).
		const void* pointer;	// Pointer to the first coordinate of the first vertex in the array.
		bool enabled;			// Define whether the array is enabled or not.
	} ArrayDesc;

	//! Project an array using the current projection.
	//! @return a descriptor of the new array
	ArrayDesc projectArray(const ArrayDesc& array, int offset, int count, const unsigned int* indices=NULL);

	void drawTextGravity180(float x, float y, const QString& str, float xshift = 0, float yshift = 0);

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

	//! Whether ARB_texture_non_power_of_two is supported on this card
	static bool isNoPowerOfTwoAllowed;

#ifdef STELPAINTER_GL2
	Vec4f currentColor;
	bool texture2dEnabled;
	static QGLShaderProgram* basicShaderProgram;
	struct BasicShaderVars {
		int projectionMatrix;
		int globalColor;
		int vertex;
	};
	static BasicShaderVars basicShaderVars;
	static QGLShaderProgram* colorShaderProgram;
	static QGLShaderProgram* texturesShaderProgram;
	struct TexturesShaderVars {
		int projectionMatrix;
		int texCoord;
		int vertex;
		int globalColor;
		int texture;
	};
	static TexturesShaderVars texturesShaderVars;
	static QGLShaderProgram* texturesColorShaderProgram;
	struct TexturesColorShaderVars {
		int projectionMatrix;
		int texCoord;
		int vertex;
		int color;
		int texture;
	};
	static TexturesColorShaderVars texturesColorShaderVars;
#endif

	//! The descriptor for the current opengl vertex array
	ArrayDesc vertexArray;
	//! The descriptor for the current opengl texture coordinate array
	ArrayDesc texCoordArray;
	//! The descriptor for the current opengl normal array
	ArrayDesc normalArray;
	//! The descriptor for the current opengl color array
	ArrayDesc colorArray;
};

#endif // _STELPAINTER_HPP_

