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

#ifdef USE_OPENGL_ES2
 #define STELPAINTER_GL2 1
#endif

#ifdef STELPAINTER_GL2
class QGLShaderProgram;
#endif

class QPainter;
class QGLContext;

class StelPainterLight
{
public:
	StelPainterLight(int alight=0) : light(alight), enabled(false) {}

	void setPosition(const Vec4f& v);
	Vec4f& getPosition() {return position;}

	void setDiffuse(const Vec4f& v);
	Vec4f& getDiffuse() {return diffuse;}

	void setSpecular(const Vec4f& v);
	Vec4f& getSpecular() {return specular;}

	void setAmbient(const Vec4f& v);
	Vec4f& getAmbient() {return ambient;}

	void setEnable(bool v);
	void enable();
	void disable();
	bool isEnabled() const {return enabled;}

private:
	int light;
	Vec4f position;
	Vec4f diffuse;
	Vec4f specular;
	Vec4f ambient;
	bool enabled;
};


class StelPainterMaterial
{
public:
	StelPainterMaterial();

	void setSpecular(const Vec4f& v);
	Vec4f& getSpecular() {return specular;}

	void setAmbient(const Vec4f& v);
	Vec4f& getAmbient() {return ambient;}

	void setEmission(const Vec4f& v);
	Vec4f& getEmission() {return emission;}

	void setShininess(float v);
	float getShininess() {return shininess;}
private:
	Vec4f specular;
	Vec4f ambient;
	Vec4f emission;
	float shininess;
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

	//! Define the drawing mode when drawing polygons
	enum SphericalPolygonDrawMode
	{
		SphericalPolygonDrawModeFill=0,			//!< Draw the interior of the polygon
		SphericalPolygonDrawModeBoundary=1,		//!< Draw the boundary of the polygon
		SphericalPolygonDrawModeTextureFill=2	//!< Draw the interior of the polygon filled with the current texture
	};

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

	//! Fill with black around the viewport.
	void drawViewportShape();

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
	void drawSphericalRegion(const SphericalRegion* region, SphericalPolygonDrawMode drawMode=SphericalPolygonDrawModeFill, const SphericalCap* clippingCap=NULL, bool doSubDivise=true, double maxSqDistortion=5.);

	void drawGreatCircleArcs(const StelVertexArray& va, const SphericalCap* clippingCap=NULL);

	void drawSphericalTriangles(const StelVertexArray& va, bool textured, const SphericalCap* clippingCap=NULL, bool doSubDivide=true, double maxSqDistortion=5.);

	//! Draw a small circle arc between points start and stop with rotation point in rotCenter.
	//! The angle between start and stop must be < 180 deg.
	//! The algorithm ensures that the line will look smooth, even for non linear distortion.
	//! Each time the small circle crosses the edge of the viewport, the viewportEdgeIntersectCallback is called with the
	//! screen 2d position, direction of the currently drawn arc toward the inside of the viewport.
	//! If rotCenter is equal to 0,0,0, the method draws a great circle.
	void drawSmallCircleArc(const Vec3d& start, const Vec3d& stop, const Vec3d& rotCenter, void (*viewportEdgeIntersectCallback)(const Vec3d& screenPos, const Vec3d& direction, void* userData)=NULL, void* userData=NULL);

	//! Draw a great circle arc between points start and stop.
	//! The angle between start and stop must be < 180 deg.
	//! The algorithm ensures that the line will look smooth, even for non linear distortion.
	//! Each time the small circle crosses the edge of the viewport, the viewportEdgeIntersectCallback is called with the
	//! screen 2d position, direction of the currently drawn arc toward the inside of the viewport.
	//! @param clippingCap if not set to NULL, tells the painter to try to clip part of the region outside the cap.
	void drawGreatCircleArc(const Vec3d& start, const Vec3d& stop, const SphericalCap* clippingCap=NULL, void (*viewportEdgeIntersectCallback)(const Vec3d& screenPos, const Vec3d& direction, void* userData)=NULL, void* userData=NULL);

	//! Draw a simple circle, 2d viewport coordinates in pixel
	void drawCircle(float x, float y, float r);

	//! Draw a square using the current texture at the given projected 2d position.
	//! This method is not thread safe.
	//! @param x x position in the viewport in pixel.
	//! @param y y position in the viewport in pixel.
	//! @param radius the half size of a square side in pixel.
	void drawSprite2dMode(float x, float y, float radius);
	void drawSprite2dMode(const Vec3d& v, float radius);

	//! Draw a rotated square using the current texture at the given projected 2d position.
	//! This method is not thread safe.
	//! @param x x position in the viewport in pixel.
	//! @param y y position in the viewport in pixel.
	//! @param radius the half size of a square side in pixel.
	//! @param rotation rotation angle in degree.
	void drawSprite2dMode(float x, float y, float radius, float rotation);

	//! Draw a GL_POINT at the given position.
	//! @param x x position in the viewport in pixels.
	//! @param y y position in the viewport in pixels.
	void drawPoint2d(float x, float y);

	//! Draw a line between the 2 points.
	//! @param x1 x position of point 1 in the viewport in pixels.
	//! @param y1 y position of point 1 in the viewport in pixels.
	//! @param x2 x position of point 2 in the viewport in pixels.
	//! @param y2 y position of point 2 in the viewport in pixels.
	void drawLine2d(float x1, float y1, float x2, float y2);

	//! Draw a rectangle using the current texture at the given projected 2d position.
	//! This method is not thread safe.
	//! @param x x position of the top left corner in the viewport in pixel.
	//! @param y y position of the tope left corner in the viewport in pixel.
	//! @param width width in pixel.
	//! @param height height in pixel.
	//! @param textured whether the current texture should be used for painting.
	void drawRect2d(float x, float y, float width, float height, bool textured=true);

	//! Re-implementation of gluSphere : glu is overridden for non-standard projection.
	void sSphere(float radius, float oneMinusOblateness, int slices, int stacks, int orientInside = 0, bool flipTexture = false);

	//! Generate a StelVertexArray for a sphere.
	static StelVertexArray computeSphereNoLight(float radius, float oneMinusOblateness, int slices, int stacks, int orientInside = 0, bool flipTexture = false);

	//! Re-implementation of gluCylinder : glu is overridden for non-standard projection.
	void sCylinder(float radius, float height, int slices, int orientInside = 0);

	//! Draw a disk with a special texturing mode having texture center at center of disk.
	//! The disk is made up of concentric circles with increasing refinement.
	//! The number of slices of the outmost circle is (innerFanSlices<<level).
	//! @param radius the radius of the disk.
	//! @param innerFanSlices the number of slices.
	//! @param level the numbe of concentric circles.
	//! @param vertexArr the vertex array in which the resulting vertices are returned.
	//! @param texCoordArr the vertex array in which the resulting texture coordinates are returned.
	static void computeFanDisk(float radius, int innerFanSlices, int level, QVector<double>& vertexArr, QVector<float>& texCoordArr);

	//! Draw a ring with a radial texturing.
	void sRing(float rMin, float rMax, int slices, int stacks, int orientInside);

	//! Draw a fisheye texture in a sphere.
	void sSphereMap(float radius, int slices, int stacks, float textureFov = 2.f*M_PI, int orientInside = 0);

	//! Set the font to use for subsequent text drawing.
	void setFont(const QFont& font);

	//! Set the color to use for subsequent drawing.
	void setColor(float r, float g, float b, float a=1.f);

	//! Get the color currently used for drawing.
	Vec4f getColor() const;

	//! Get the light
	StelPainterLight& getLight() {return light;}

	//! Get the material
	StelPainterMaterial& getMaterial() {return material;}

	//! Get the font metrics for the current font.
	QFontMetrics getFontMetrics() const;

	//! Get some informations about the OS openGL capacities and set the GLContext which will be used by Stellarium.
	//! This method needs to be called once at init.
	static void initSystemGLInfo(QGLContext* ctx);

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

	//! Project the passed triangle on the screen ensuring that it will look smooth, even for non linear distortion
	//! by splitting it into subtriangles. The resulting vertex arrays are appended to the passed out* ones.
	//! The size of each edge must be < 180 deg.
	//! @param vertices a pointer to an array of 3 vertices.
	//! @param edgeFlags a pointer to an array of 3 flags indicating whether the next segment is an edge.
	//! @param texturePos a pointer to an array of 3 texture coordinates, or NULL if the triangle should not be textured.
	void projectSphericalTriangle(const SphericalCap* clippingCap, const Vec3d* vertices, QVarLengthArray<Vec3f, 4096>* outVertices,
			const Vec2f* texturePos=NULL, QVarLengthArray<Vec2f, 4096>* outTexturePos=NULL, double maxSqDistortion=5., int nbI=0,
			bool checkDisc1=true, bool checkDisc2=true, bool checkDisc3=true) const;

	void drawTextGravity180(float x, float y, const QString& str, float xshift = 0, float yshift = 0);

	// Used by the method below
	static QVector<Vec2f> smallCircleVertexArray;
	void drawSmallCircleVertexArray();

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
		int color;
		int vertex;
	};
	static BasicShaderVars basicShaderVars;
	static QGLShaderProgram* colorShaderProgram;
	static QGLShaderProgram* texturesShaderProgram;
	struct TexturesShaderVars {
		int projectionMatrix;
		int texCoord;
		int vertex;
		int texColor;
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

	//! the single light used by the painter
	StelPainterLight light;

	//! The material used by the painter
	StelPainterMaterial material;
};

#endif // _STELPAINTER_HPP_

