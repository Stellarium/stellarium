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

#ifndef STELPAINTER_HPP
#define STELPAINTER_HPP

#include "StelOpenGL.hpp"
#include "VecMath.hpp"
#include "StelSphereGeometry.hpp"
#include "StelProjectorType.hpp"
#include "StelProjector.hpp"
#include <QString>
#include <QVarLengthArray>
#include <QFontMetrics>

class QOpenGLShaderProgram;

//! @class StelPainter
//! Provides functions for performing openGL drawing operations.
//! All coordinates are converted using the StelProjector instance passed at construction.
//! Because openGL is not thread safe, only one instance of StelPainter can exist at a time, enforcing thread safety.
//! As a coding rule, no openGL calls should be performed when no instance of StelPainter exist.
//! Typical usage is to create a local instance of StelPainter where drawing operations are needed.
class StelPainter : protected QOpenGLFunctions
{
public:
	friend class VertexArrayProjector;

	//! Define the drawing mode when drawing polygons
	enum SphericalPolygonDrawMode
	{
		SphericalPolygonDrawModeFill=0,				//!< Draw the interior of the polygon
		SphericalPolygonDrawModeBoundary=1,			//!< Draw the boundary of the polygon
		SphericalPolygonDrawModeTextureFill=2,			//!< Draw the interior of the polygon filled with the current texture
		SphericalPolygonDrawModeTextureFillColormodulated=3	//!< Draw the interior of the polygon filled with the current texture multiplied by vertex colors
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

	enum class DitheringMode
	{
		Disabled,    //!< Dithering disabled, will leave the infamous color bands
		Color565,    //!< 16-bit color (AKA High color) with R5_G6_B5 layout
		Color666,    //!< TN+film typical color depth in TrueColor mode
		Color888,    //!< 24-bit color (AKA True color)
		Color101010, //!< 30-bit color (AKA Deep color)
	};

	explicit StelPainter(const StelProjectorP& prj);
	~StelPainter();

	//! Returns a QOpenGLFunctions object suitable for drawing directly with OpenGL while this StelPainter is active.
	//! This is recommended to be used instead of QOpenGLContext::currentContext()->functions() when a StelPainter is available,
	//! and you only need to call a few GL functions directly.
	inline QOpenGLFunctions* glFuncs() { return this; }

	//! Return the instance of projector associated to this painter
	const StelProjectorP& getProjector() const {return prj;}
	void setProjector(const StelProjectorP& p);

	//! Fill with black around the viewport.
	void drawViewportShape(void);

	//! Draw the string at the given position and angle with the given font.
	//! If the gravity label flag is set, uses drawTextGravity180.
	//! @param x horizontal position of the lower left corner of the first character of the text in pixel.
	//! @param y vertical position of the lower left corner of the first character of the text in pixel.
	//! @param str the text to print.
	//! @param angleDeg rotation angle in degree. Rotation is around x,y.
	//! @param xshift shift in pixel in the rotated x direction.
	//! @param yshift shift in pixel in the rotated y direction.
	//! @param noGravity don't take into account the fact that the text should be written with gravity.
	//! @param v direction vector of object to draw. GZ20120826: Will draw only if this is in the visible hemisphere.
	void drawText(float x, float y, const QString& str, float angleDeg=0.f,
              float xshift=0.f, float yshift=0.f, bool noGravity=true);
	void drawText(const Vec3d& v, const QString& str, float angleDeg=0.f,
              float xshift=0.f, float yshift=0.f, bool noGravity=true);

	//! Draw the given SphericalRegion.
	//! @param region The SphericalRegion to draw.
	//! @param drawMode define whether to draw the outline or the fill or both.
	//! @param clippingCap if not set to Q_NULLPTR, tells the painter to try to clip part of the region outside the cap.
	//! @param doSubDivise if true tesselates the object to follow projection distortions.
	//! Typically set that to false if you think that the region is fully contained in the viewport.
	void drawSphericalRegion(const SphericalRegion* region, SphericalPolygonDrawMode drawMode=SphericalPolygonDrawModeFill, const SphericalCap* clippingCap=Q_NULLPTR, bool doSubDivise=true, double maxSqDistortion=5.);

	void drawGreatCircleArcs(const StelVertexArray& va, const SphericalCap* clippingCap=Q_NULLPTR);

	void drawSphericalTriangles(const StelVertexArray& va, bool textured, bool colored, const SphericalCap* clippingCap=Q_NULLPTR, bool doSubDivide=true, double maxSqDistortion=5.);

	//! Draw a small circle arc between points start and stop with rotation point in rotCenter.
	//! The angle between start and stop must be < 180 deg.
	//! The algorithm ensures that the line will look smooth, even for non linear distortion.
	//! Each time the small circle crosses the edge of the viewport, the viewportEdgeIntersectCallback is called with the
	//! screen 2d position, direction of the currently drawn arc toward the inside of the viewport.
	//! Example: A latitude circle has 0/0/sin(latitude) as rotCenter.
	//! If rotCenter is equal to 0,0,0, the method draws a great circle.
	void drawSmallCircleArc(const Vec3d& start, const Vec3d& stop, const Vec3d& rotCenter, void (*viewportEdgeIntersectCallback)(const Vec3d& screenPos, const Vec3d& direction, void* userData)=Q_NULLPTR, void* userData=Q_NULLPTR);

	//! Draw a great circle arc between points start and stop.
	//! The angle between start and stop must be < 180 deg.
	//! The algorithm ensures that the line will look smooth, even for non linear distortion.
	//! Each time the great circle crosses the edge of the viewport, the viewportEdgeIntersectCallback is called with the
	//! screen 2d position, direction of the currently drawn arc toward the inside of the viewport.
	//! @param clippingCap if not set to Q_NULLPTR, tells the painter to try to clip part of the region outside the cap.
	void drawGreatCircleArc(const Vec3d& start, const Vec3d& stop, const SphericalCap* clippingCap=Q_NULLPTR, void (*viewportEdgeIntersectCallback)(const Vec3d& screenPos, const Vec3d& direction, void* userData)=Q_NULLPTR, void* userData=Q_NULLPTR);

	//! Draw a curve defined by a list of points.
	//! The points should be already tesselated to ensure that the path will look smooth.
	//! The algorithm takes care of cutting the path if it crosses a viewport discontinuity.
	void drawPath(const QVector<Vec3d> &points, const QVector<Vec4f> &colors);

	//! Draw a simple circle, 2d viewport coordinates in pixel
	void drawCircle(float x, float y, float r);

	//! Draw a square using the current texture at the given projected 2d position.
	//! This method is not thread safe.
	//! @param x x position in the viewport in pixel.
	//! @param y y position in the viewport in pixel.
	//! @param radius the half size of a square side in pixel.
	//! @param v direction vector of object to draw. Will draw only if this is in the visible hemisphere.
	void drawSprite2dMode(float x, float y, float radius);
	void drawSprite2dMode(const Vec3d& v, float radius);

	//! Same as drawSprite2dMode but don't scale according to display device scaling. 
	void drawSprite2dModeNoDeviceScale(float x, float y, float radius);
	
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
	//! @param x1 x position of point 1 in the viewport in pixels. 0 is at left.
	//! @param y1 y position of point 1 in the viewport in pixels. 0 is at bottom.
	//! @param x2 x position of point 2 in the viewport in pixels. 0 is at left.
	//! @param y2 y position of point 2 in the viewport in pixels. 0 is at bottom.
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
	//! @param radius
	//! @param oneMinusOblateness
	//! @param slices: number of vertical segments ("meridian zones")
	//! @param stacks: number of horizontal segments ("latitude zones")
	//! @param orientInside: 1 to have normals point inside, e.g. for landscape horizons
	//! @param flipTexture: if texture should be mapped to inside of sphere, e.g. landscape horizons.
	//! @param topAngle GZ: new parameter. An opening angle [radians] at top of the sphere. Useful if there is an empty
	//!        region around the top pole, like for a spherical equirectangular horizon panorama (@class SphericalLandscape).
	//!        Example: your horizon line (pano photo) goes up to 26 degrees altitude (highest mountains/trees):
	//!        topAngle = 64 degrees = 64*M_PI/180.0f
	//! @param bottomAngle GZ: new parameter. An opening angle [radians] at bottom of the sphere. Useful if there is an empty
	//!        region around the bottom pole, like for a spherical equirectangular horizon panorama (SphericalLandscape class).
	//!        Example: your light pollution image (pano photo) goes down to just -5 degrees altitude (lowest street lamps below you):
	//!        bottomAngle = 95 degrees = 95*M_PI/180.0f
	void sSphere(const double radius, const double oneMinusOblateness, const unsigned int slices, const unsigned int stacks,
		     const bool orientInside = false, const bool flipTexture = false,
		     const float topAngle = 0.0f, const float bottomAngle = static_cast<float>(M_PI));

	//! Generate a StelVertexArray for a sphere.
	//! @param radius
	//! @param oneMinusOblateness
	//! @param slices: number of vertical segments ("meridian zones")
	//! @param stacks: number of horizontal segments ("latitude zones")
	//! @param orientInside: 1 to have normals point inside, e.g. for Milky Way, Zodiacal Light, etc.
	//! @param flipTexture: if texture should be mapped to inside of sphere, e.g. Milky Way.
	//! @param topAngle: An opening angle [radians] at top of the sphere. Useful if there is an empty
	//!        region around the top pole, like North Galactic Pole.
	//! @param bottomAngle: An opening angle [radians] at bottom of the sphere. Useful if there is an empty
	//!        region around the bottom pole, like South Galactic Pole.
	static StelVertexArray computeSphereNoLight(double radius, double oneMinusOblateness, unsigned int slices, unsigned int stacks,
			    int orientInside = 0, bool flipTexture = false,
			    float topAngle=0.0f, float bottomAngle=static_cast<float>(M_PI));

	//! Re-implementation of gluCylinder : glu is overridden for non-standard projection.
	void sCylinder(double radius, double height, int slices, int orientInside = 0);

	//! Draw a disk with a special texturing mode having texture center at center of disk.
	//! The disk is made up of concentric circles with increasing refinement.
	//! The number of slices of the outmost circle is (innerFanSlices<<level).
	//! @param radius the radius of the disk.
	//! @param innerFanSlices the number of slices.
	//! @param level the number of concentric circles.
	//! @param vertexArr the vertex array in which the resulting vertices are returned.
	//! @param texCoordArr the vertex array in which the resulting texture coordinates are returned.
	static void computeFanDisk(float radius, uint innerFanSlices, uint level, QVector<Vec3d>& vertexArr, QVector<Vec2f>& texCoordArr);

	//! Draw a fisheye texture in a sphere.
	void sSphereMap(double radius, unsigned int slices, unsigned int stacks, float textureFov = 2.f*static_cast<float>(M_PI), int orientInside = 0);

	//! Set the font to use for subsequent text drawing.
	void setFont(const QFont& font);

	//! Set the color to use for subsequent drawing.
	void setColor(float r, float g, float b, float a=1.f);

	//! Set the color to use for subsequent drawing.
	void setColor(Vec3f rgb, float a=1.f);

	//! Set the color to use for subsequent drawing.
	void setColor(Vec4f rgba);

	//! Get the color currently used for drawing.
	Vec4f getColor() const;

	//! Get the font metrics for the current font.
	QFontMetrics getFontMetrics() const;

	//! Enable OpenGL blending. By default, blending is disabled.
	//! The additional parameters specify the blending mode, the default parameters are suitable for
	//! "normal" blending operations that you want in most cases. Blending will be automatically disabled when
	//! the StelPainter is destroyed.
	void setBlending(bool enableBlending, GLenum blendSrc = GL_SRC_ALPHA, GLenum blendDst = GL_ONE_MINUS_SRC_ALPHA);

	void setDepthTest(bool enable);

	void setDepthMask(bool enable);

	//! Set the OpenGL GL_CULL_FACE state, by default face culling is disabled
	void setCullFace(bool enable);

	//! Enables/disables line smoothing. By default, smoothing is disabled.
	void setLineSmooth(bool enable);

	//! Sets the line width. Default is 1.0f.
	void setLineWidth(float width);
	//! Gets the line width.
	float getLineWidth() const {return glState.lineWidth;}

	//! Sets the color saturation effect value, from 0 (grayscale) to 1 (no effect).
	void setSaturation(float v) { saturation = v; }

	//! Create the OpenGL shaders programs used by the StelPainter.
	//! This method needs to be called once at init.
	static void initGLShaders();
	
	//! Delete the OpenGL shaders objects.
	//! This method needs to be called once before exit.
	static void deinitGLShaders();

	// Thoses methods should eventually be replaced by a single setVertexArray
	//! use instead of glVertexPointer
	void setVertexPointer(int size, GLenum type, const void* pointer) {
		vertexArray.size = size; vertexArray.type = type; vertexArray.pointer = pointer;
	}

	//! use instead of glTexCoordPointer
	void setTexCoordPointer(int size, GLenum type, const void* pointer)
	{
		texCoordArray.size = size; texCoordArray.type = type; texCoordArray.pointer = pointer;
	}

	//! use instead of glColorPointer
	void setColorPointer(int size, GLenum type, const void* pointer)
	{
		colorArray.size = size; colorArray.type = type; colorArray.pointer = pointer;
	}

	//! use instead of glNormalPointer
	void setNormalPointer(GLenum type, const void* pointer)
	{
		normalArray.size = 3; normalArray.type = type; normalArray.pointer = pointer;
	}

	//! Simulates glEnableClientState, basically you describe what data the ::drawFromArray call has available
	void enableClientStates(bool vertex, bool texture=false, bool color=false, bool normal=false);

	//! convenience method that enable and set all the given arrays.
	//! It is equivalent to calling enableClientStates() and set the array pointer for each array.
	void setArrays(const Vec3d* vertices, const Vec2f* texCoords=Q_NULLPTR, const Vec3f* colorArray=Q_NULLPTR, const Vec3f* normalArray=Q_NULLPTR);
	void setArrays(const Vec3f* vertices, const Vec2f* texCoords=Q_NULLPTR, const Vec3f* colorArray=Q_NULLPTR, const Vec3f* normalArray=Q_NULLPTR);

	//! Draws primitives using vertices from the arrays specified by setArrays() or enabled via enableClientStates().
	//! @param mode The type of primitive to draw.
	//! If @param indices is Q_NULLPTR, this operation will consume @param count values from the enabled arrays, starting at @param offset.
	//! Else it will consume @param count elements of @param indices, starting at @param offset, which are used to index into the
	//! enabled arrays.
	//! NOTE: Prefer to use drawStelVertexArray, else there are wrap-around rendering artifacts in a few projections.
	void drawFromArray(DrawingMode mode, int count, int offset=0, bool doProj=true, const unsigned short *indices=Q_NULLPTR);

	//! Draws the primitives defined in the StelVertexArray.
	//! @param checkDiscontinuity will check and suppress discontinuities if necessary.
	void drawStelVertexArray(const StelVertexArray& arr, bool checkDiscontinuity=true);

	//! Link an opengl program and show a message in case of error or warnings.
	//! @return true if the link was successful.
	static bool linkProg(class QOpenGLShaderProgram* prog, const QString& name);

	DitheringMode getDitheringMode() const { return ditheringMode; }

private:
	friend class StelTextureMgr;
	friend class StelTexture;

	//! Helper struct to track the GL state and restore it to canonical values on StelPainter creation/destruction
	struct GLState
	{
		GLState(QOpenGLFunctions *gl);

		bool blend;
		GLenum blendSrc, blendDst;
		bool depthTest;
		bool depthMask;
		bool cullFace;
		bool lineSmooth;
		GLfloat lineWidth;

		//! Applies the values stored here to set the GL state
		void apply();
		//! Resets the state to the default values (like a GLState was newly constructed)
		//! and calls apply()
		void reset();
	private:
		QOpenGLFunctions* gl;
	} glState;

	// From text-use-opengl-buffer
	static QCache<QByteArray, struct StringTexture> texCache;
	struct StringTexture* getTexTexture(const QString& str, int pixelSize) const;

	//! Struct describing one opengl array
	typedef struct ArrayDesc
	{
		ArrayDesc() : size(0), type(0), pointer(Q_NULLPTR), enabled(false) {}
		int size;				// The number of coordinates per vertex.
		GLenum type;				// The data type of each coordinate (GL_SHORT, GL_INT, GL_FLOAT, or GL_DOUBLE).
		const void* pointer;	// Pointer to the first coordinate of the first vertex in the array.
		bool enabled;			// Define whether the array is enabled or not.
	} ArrayDesc;

	//! Project an array using the current projection.
	//! @return a descriptor of the new array
	ArrayDesc projectArray(const ArrayDesc& array, int offset, int count, const unsigned short *indices=Q_NULLPTR);

	//! Project the passed triangle on the screen ensuring that it will look smooth, even for non linear distortion
	//! by splitting it into subtriangles. The resulting vertex arrays are appended to the passed out* ones.
	//! The size of each edge must be < 180 deg.
	//! @param vertices a pointer to an array of 3 vertices.
	//! @param edgeFlags a pointer to an array of 3 flags indicating whether the next segment is an edge.
	//! @param texturePos a pointer to an array of 3 texture coordinates, or Q_NULLPTR if the triangle should not be textured.
	//! @param colors a pointer to an array of 3 colors, or Q_NULLPTR if the triangle should not be vertex-colored. If texture and color coords are present, texture is modulated by vertex colors. (e.g. extinction)
	void projectSphericalTriangle(const SphericalCap* clippingCap, const Vec3d* vertices, QVarLengthArray<Vec3f, 4096>* outVertices,
			const Vec2f* texturePos=Q_NULLPTR, QVarLengthArray<Vec2f, 4096>* outTexturePos=Q_NULLPTR,
			const Vec3f* colors=Q_NULLPTR, QVarLengthArray<Vec3f, 4096>* outColors=Q_NULLPTR,
            double maxSqDistortion=5., int nbI=0,
            bool checkDisc1=true, bool checkDisc2=true, bool checkDisc3=true) const;

	void drawTextGravity180(float x, float y, const QString& str, float xshift = 0, float yshift = 0);

	// Used by the method below
	static QVector<Vec3f> smallCircleVertexArray;
	static QVector<Vec4f> smallCircleColorArray;
	void drawSmallCircleVertexArray();

	//! The associated instance of projector
	StelProjectorP prj;

#ifndef NDEBUG
	//! Mutex allowing thread safety
	static class QMutex* globalMutex;
#endif

	//! The used for text drawing
	QFont currentFont;

	Vec4f currentColor;
	//! Saturation effect adjustment.
	float saturation = 1.f;

	static QOpenGLShaderProgram* basicShaderProgram;
	struct BasicShaderVars {
		int projectionMatrix;
		int color;
		int vertex;
	};
	static BasicShaderVars basicShaderVars;

	static QOpenGLShaderProgram* colorShaderProgram;
	static BasicShaderVars colorShaderVars;

	static QOpenGLShaderProgram* texturesShaderProgram;
	struct TexturesShaderVars {
		int projectionMatrix;
		int texCoord;
		int vertex;
		int texColor;
		int texture;
		int bayerPattern;
		int rgbMaxValue;
	};
	static TexturesShaderVars texturesShaderVars;
	static QOpenGLShaderProgram* texturesColorShaderProgram;
	struct TexturesColorShaderVars {
		int projectionMatrix;
		int texCoord;
		int vertex;
		int color;
		int texture;
		int bayerPattern;
		int rgbMaxValue;
		int saturation;
	};
	static TexturesColorShaderVars texturesColorShaderVars;

	GLuint bayerPatternTex=0;
	DitheringMode ditheringMode;
	static DitheringMode parseDitheringMode(QString const& s);

	//! The descriptor for the current opengl vertex array
	ArrayDesc vertexArray;
	//! The descriptor for the current opengl texture coordinate array
	ArrayDesc texCoordArray;
	//! The descriptor for the current opengl normal array
	ArrayDesc normalArray;
	//! The descriptor for the current opengl color array
	ArrayDesc colorArray;
};

Q_DECLARE_METATYPE(StelPainter::DitheringMode)

#endif // STELPAINTER_HPP

