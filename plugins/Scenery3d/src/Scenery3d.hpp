/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011-15 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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

#ifndef _SCENERY3D_HPP_
#define _SCENERY3D_HPP_

#include "StelGui.hpp"
#include "StelModule.hpp"
#include "StelPainter.hpp"
#include "Landscape.hpp"
#include "SolarSystem.hpp"

#include "OBJ.hpp"
#include "Heightmap.hpp"
#include "Frustum.hpp"
#include "Polyhedron.hpp"
#include "S3DEnum.hpp"
#include "SceneInfo.hpp"
#include "ShaderManager.hpp"

#include <QMatrix4x4>

//predeclarations
class QOpenGLFramebufferObject;
class Scenery3dMgr;
class LandscapeMgr;

//! Representation of a complete 3D scenery
class Scenery3d
{
public:
	//! Initializes an empty Scenery3d object.
	Scenery3d(Scenery3dMgr *parent);
	virtual ~Scenery3d();

	//! Loads the specified scene
	bool loadScene(const SceneInfo& scene);

	//! Walk/Fly Navigation with Ctrl+Cursor and Ctrl+PgUp/Dn keys.
	//! Pressing Ctrl-Alt: 5x, Ctrl-Shift: 10x speedup; Ctrl-Shift-Alt: 50x!
	//! To allow fine control, zoom in.
	//! If you release Ctrl key while pressing cursor key, movement will continue.
	void handleKeys(QKeyEvent* e);

	//! Update method, called by Scenery3dMgr.
	//! Shifts observer position due to movement through the landscape.
	void update(double deltaTime);
	//! Draw observer grid coordinates as text.
	void drawCoordinatesText();
	//! Draw some text output. This can be filled as needed by development.
	void drawDebug();

	//! Draw scenery, called by Scenery3dMgr.
	void draw(StelCore* core);
	//! Performs initialization that requires an valid OpenGL context
	void init();

	//! Gets the current scene's metadata
	SceneInfo getCurrentScene() { return currentScene; }

	bool getDebugEnabled() const { return debugEnabled; }
	void setDebugEnabled(bool debugEnabled) { this->debugEnabled = debugEnabled; }
	bool getPixelLightingEnabled() const { return shaderParameters.pixelLighting; }
	void setPixelLightingEnabled(const bool val) { shaderParameters.pixelLighting = val; invalidateCubemap(); }
	bool getShadowsEnabled(void) const { return shaderParameters.shadows; }
	void setShadowsEnabled(bool shadowsEnabled) { shaderParameters.shadows = shadowsEnabled; reinitShadowmapping = true; invalidateCubemap(); }
	bool getUseSimpleShadows() const {return simpleShadows; }
	void setUseSimpleShadows(bool val) { simpleShadows = val; reinitShadowmapping = true; invalidateCubemap(); }
	bool getBumpsEnabled(void) const { return shaderParameters.bump; }
	void setBumpsEnabled(bool bumpsEnabled) { shaderParameters.bump = bumpsEnabled; invalidateCubemap(); }
	bool getTorchEnabled(void) const { return shaderParameters.torchLight; }
	void setTorchEnabled(bool torchEnabled) { shaderParameters.torchLight = torchEnabled; invalidateCubemap(); }
	S3DEnum::ShadowFilterQuality getShadowFilterQuality() const { return shaderParameters.shadowFilterQuality; }
	void setShadowFilterQuality(S3DEnum::ShadowFilterQuality quality) { shaderParameters.shadowFilterQuality = quality; reinitShadowmapping=true; invalidateCubemap();}
	bool getPCSS() const { return shaderParameters.pcss; }
	void setPCSS(bool val) { shaderParameters.pcss = val; reinitShadowmapping = true; invalidateCubemap(); }
	bool getLocationInfoEnabled(void) const { return textEnabled; }
	void setLocationInfoEnabled(bool locationinfoenabled) { this->textEnabled = locationinfoenabled; }

	bool getLazyCubemapEnabled() const { return lazyDrawing; }
	void setLazyCubemapEnabled(bool val) { lazyDrawing = val; }
	double getLazyCubemapInterval() const { return lazyInterval; }
	void setLazyCubemapInterval(double val) { lazyInterval = val; }

	//This has the be the most crazy method name in the plugin
	void getLazyCubemapUpdateOnlyDominantFaceOnMoving(bool &val, bool &alsoSecondDominantFace) { val = updateOnlyDominantOnMoving; alsoSecondDominantFace = updateSecondDominantOnMoving; }
	void setLazyCubemapUpdateOnlyDominantFaceOnMoving(bool val, bool alsoSecondDominantFace) { updateOnlyDominantOnMoving = val; updateSecondDominantOnMoving = alsoSecondDominantFace; }

	//! Does a cubemap redraw at the next possible opportunity when lazy-drawing is enabled.
	inline void invalidateCubemap() {  lastCubemapUpdate = 0.0; }

	S3DEnum::CubemappingMode getCubemappingMode() const { return cubemappingMode; }
	//! Changes cubemapping mode and forces re-initialization on next draw call.
	//! This may not set the actual mode to the parameter, call getCubemappingMode to find out what was set.
	void setCubemappingMode(S3DEnum::CubemappingMode mode)
	{
		if(mode == S3DEnum::CM_CUBEMAP_GSACCEL && !isGeometryShaderCubemapSupported())
		{
			//fallback to 6 Textures mode
			mode = S3DEnum::CM_TEXTURES;
		}
		if(mode == S3DEnum::CM_CUBEMAP && isANGLEContext())
		{
			//Cubemap mode disabled on ANGLE because of an implementation bug with Qt 5.4's version
			mode = S3DEnum::CM_TEXTURES;
		}
		cubemappingMode = mode; reinitCubemapping = true;
	}

	void setUseFullCubemapShadows(bool val) { fullCubemapShadows = val; invalidateCubemap();}
	bool getUseFullCubemapShadows() const { return fullCubemapShadows; }

	uint getCubemapSize() const { return cubemapSize; }
	//! Note: This may not set the size to the desired one because of hardware limits, call getCubemapSize to receive the value set after this call.
	void setCubemapSize(uint size) { cubemapSize = (size > maximumFramebufferSize ? maximumFramebufferSize : size); reinitCubemapping = true; }
	uint getShadowmapSize() const { return shadowmapSize; }
	//! Note: This may not set the size to the desired one because of hardware limits, call getShadowmapSize to receive the value set after this call.
	void setShadowmapSize(uint size) { shadowmapSize = (size > maximumFramebufferSize ? maximumFramebufferSize : size); reinitShadowmapping = true; }
	float getTorchBrightness() const { return torchBrightness; }
	void setTorchBrightness(float brightness) { torchBrightness = brightness; invalidateCubemap(); }
	float getTorchRange() const { return torchRange; }
	void setTorchRange(float range) { torchRange = range; invalidateCubemap(); }

	void setLoadCancel(bool val) { loadCancel = val; }

	//! Sets the observer position to the specified grid coordinates.
	//! The height is assumed to be at the feet, so make sure to set the eye height with setEyeHeight before, if necessary.
	void setGridPosition(Vec3d pos);
	//! Gets the current position on the scene's grid (height at feet)
	Vec3d getCurrentGridPosition() const;

	//! Sets the observer eye height.
	void setEyeHeight(const float eyeheight) { eye_height = eyeheight; }
	//! Gets the current observer eye height (vertical difference from feet to camera position).
	float getEyeHeight() const { return eye_height; }

	enum ShadowCaster { None, Sun, Moon, Venus };

	//! Returns the shader manager this instance uses
	ShaderMgr& getShaderManager()    {	    return shaderManager;    }

	//! Loads the model into GL and sets the loaded scene to be the current one
	void finalizeLoad();

	//these are some properties that determine the features supported in the current GL context
	//available after init() is called
	bool isGeometryShaderCubemapSupported() { return supportsGSCubemapping; }
	bool areShadowsSupported() { return supportsShadows; }
	bool isShadowFilteringSupported() { return supportsShadowFiltering; }
	bool isANGLEContext() { return isANGLE; }
	unsigned int getMaximumFramebufferSize() { return maximumFramebufferSize; }

private:
	Scenery3dMgr* parent;
	SceneInfo currentScene,loadingScene;
	ShaderMgr shaderManager;
	PlanetP sun,moon,venus;

	bool supportsGSCubemapping; //if the GL context supports geometry shader cubemapping
	bool supportsShadows; //if shadows are supported
	bool supportsShadowFiltering; //if shadow filtering is supported
	bool isANGLE; //true if running on ANGLE
	unsigned int maximumFramebufferSize;

	float torchBrightness; // toggle light brightness
	float torchRange; // used to calculate attenuation like in the second form at http://framebunker.com/blog/lighting-2-attenuation/

	bool textEnabled;           // switchable value: display coordinates on screen. THIS IS NOT FOR DEBUGGING, BUT A PROGRAM FEATURE!
	bool debugEnabled;          // switchable value: display debug graphics and debug texts on screen
	bool fixShadowData; //for debugging, fixes all shadow mapping related data (shadowmap contents, matrices, frustums, focus bodies...) at their current values
	bool simpleShadows;
	bool fullCubemapShadows;
	S3DEnum::CubemappingMode cubemappingMode;
	bool reinitCubemapping,reinitShadowmapping;

	bool loadCancel; //true if loading process should be canceled

	unsigned int cubemapSize;            // configurable values, typically 512/1024/2048/4096
	unsigned int shadowmapSize;

	Vec3d absolutePosition;     // current eyepoint in model (note: this is actually the negated position, i.e. the value used to translate the view matrix)
	Vec3d moveVector;           // position change in scene coords
	Vec3f movement;		// speed values for moving around the scenery
	float eye_height;

	StelCore* core;
	LandscapeMgr* landscapeMgr;
	StelProjectorP altAzProjector;
	QSharedPointer<OBJ> objModel, objModelLoad, groundModel, groundModelLoad;
	Heightmap* heightmap;
	Heightmap* heightmapLoad;

	Vec3d mainViewUp;
	Vec3d mainViewDir;
	Vec3d viewPos;

	int drawnTriangles,drawnModels;
	int materialSwitches, shaderSwitches;

	/// ---- Cubemapping variables ----
	bool requiresCubemap; //true if cubemapping is required (if projection is anything else than Perspective)
	bool cubemappingUsedLastFrame; //true if cubemapping was used for the last frame. Used to determine if a projection switch occured.
	bool lazyDrawing; //if lazy-drawing mode is enabled
	bool updateOnlyDominantOnMoving; //if movement updates only dominant face directly
	bool updateSecondDominantOnMoving; //if movement also updates the second-most dominant face
	bool needsMovementEndUpdate;
	bool needsCubemapUpdate; //if the draw-call has to recreate the cubemap completely
	bool needsMovementUpdate; //if the draw-call has to recreate either the whole cubemap or the dominant face
	double lazyInterval; //the lazy-drawing time interval
	double lastCubemapUpdate; //when the last lazy draw happened (JDay)
	qint64 lastCubemapUpdateRealTime; //when the last lazy draw happened (real system time, QDateTime::currentMSecsSinceEpoch)
	qint64 lastMovementEndRealTime; //the timepoint when the last movement was stopped
	GLuint cubeMapCubeTex; //GL_TEXTURE_CUBE_MAP, used in CUBEMAP or CUBEMAP_GSACCEL modes
	GLuint cubeMapCubeDepth; //this is a depth-cubemap, only used in CUBEMAP_GSACCEL mode
	GLuint cubeMapTex[6]; //GL_TEXTURE_2D, for "legacy" TEXTURES mode
	GLuint cubeRB; //renderbuffer for depth of a single face in TEXTURES and CUBEMAP modes (attached to multiple FBOs)
	int dominantFace,secondDominantFace;

	//because of use that deviates very much from QOpenGLFramebufferObject typical usage, we manage the FBOs ourselves
	GLuint cubeFBO; //used in CUBEMAP_GSACCEL mode - only a single FBO exists, with a cubemap for color and one for depth
	GLuint cubeSideFBO[6]; //used in TEXTURES and CUBEMAP mode, 6 textures/cube faces for color and a shared depth renderbuffer (we don't require the depth after rendering)

	bool cubeMappingCreated; //true if any cubemapping objects have been initialized and need to be cleaned up eventually

	//cube geometry
	QVector<Vec3f> cubeVertices, transformedCubeVertices;
	QVector<Vec2f> cubeTexcoords;
	QOpenGLBuffer cubeVertexBuffer;
	QOpenGLBuffer cubeIndexBuffer;
	int cubeIndexCount;

	//cube rendering matrices
	QMatrix4x4 cubeRotation[6]; //rotational matrices for cube faces. The cam position is added to this in each frame.
	QMatrix4x4 cubeMVP[6]; //cube face MVP matrices

	/// ---- Rendering information ----
	//final model view matrix for shader upload
	QMatrix4x4 modelViewMatrix;
	//currently valid projection matrix for shader upload
	QMatrix4x4 projectionMatrix;

	// a struct which encapsulates lighting info
	struct LightParameters
	{
		QMatrix4x4 shadowModelView;

		ShadowCaster lightSource;
		Vec3f lightDirectionV3f;
		QVector3D lightDirectionWorld;
		QVector3D ambient;
		QVector3D directional;
		QVector3D specular;
		QVector3D emissive;

		QVector3D torchDiffuse;
		float torchAttenuation;
	} lightInfo;

	GlobalShaderParameters shaderParameters;

	//Scene AABB
	AABB sceneBoundingBox;

	//Shadow Map FBO handles
	QVector<GLuint> shadowFBOs;
	//Holds the shadow textures
	QVector<GLuint> shadowMapsArray;
	//Holds the shadow transformation matrix per split (Crop/Projection/View)
	QVector<QMatrix4x4> shadowCPM;
	//Holds the xy-scaling of the orthographic light cam + pos of near/far planes in view coords
	//Needed for consistent shadow filter sizes and PCSS effect
	QVector<QVector4D> shadowFrustumSize;
	// Frustum of the view camera, constrainted to the shadowFarZ instead of the camFarZ
	Frustum camFrustShadow;
	//near/far planes for the orthographic light that fits the whole scene
	float lightOrthoNear;
	float lightOrthoFar;
	//Array holding the split frustums
	QVector<Frustum> frustumArray;
	//Vector holding the convex split bodies for focused shadow mapping
	QVector<Polyhedron> focusBodies;

	float parallaxScale;

	QFont debugTextFont;

	QString lightMessage; // DEBUG/TEST ONLY. contains on-screen info on ambient/directional light strength and source.
	QString lightMessage2; // DEBUG/TEST ONLY. contains on-screen info on ambient/directional light strength and source.
	QString lightMessage3; // DEBUG/TEST ONLY. contains on-screen info on ambient/directional light strength and source.

	// --- initialization
	//! Determines what features the current opengl context supports
	void determineFeatureSupport();
	//! Initializes cubemapping to the currently set parameters (6tex/cubemap/GS approaches)
	bool initCubemapping();
	//! Cleans up cubemapping related objects
	void deleteCubemapping();

	//! Re-initializes shadowmapping related objects
	bool initShadowmapping();
	//! Cleans up shadowmapping related objects
	void deleteShadowmapping();

	void calcCubeMVP();

	// --- drawing methods ---
	//! Basic setup for default perspective drawing. Standard OpenGL forward rendering.
	void drawDirect();
	//! When another projection than perspective is selected, rendering is performed using a cubemap.
	void drawWithCubeMap();
	//! Performs the actual rendering of the shadow map
	bool renderShadowMaps();
	//! Creates shadowmaps for the specified cubemap face
	void renderShadowMapsForFace(int face);
	//! Generates a 6-sided cube map by drawing a view in each direction
	void generateCubeMap();
	//! Uses a geometry shader to render 6 faces in 1 pass
	void renderIntoCubemapGeometryShader();
	//! Uses 6 traditional rendering passes to render into a cubemap or 6 textures.
	void renderIntoCubemapSixPasses();
	//! Uses the StelPainter to draw a warped cube textured with our cubemap
	void drawFromCubeMap();
	//! This is the method that performs the actual drawing.
	//! If shading is true, a suitable shader for each material is selected and initialized. Submits 1 draw call for each StelModel.
	//! @return false on shader errors
	bool drawArrays(bool shading=true, bool blendAlphaAdditive=false);


	// --- shading related stuff ---
	//! Finds out the shadow caster and determines shadow parameters for current frame
	void calculateShadowCaster();
	//! Calculates lighting related stuff and puts it in the lightInfo structure
	void calculateLighting();
	//! Sets uniforms constant over the whole pass (=projection matrix, lighting & shadow info)
	void setupPassUniforms(QOpenGLShaderProgram *shader);
	//! Sets up shader uniforms constant over the whole frame/side of cubemap (=modelview dependent stuff)
	void setupFrameUniforms(QOpenGLShaderProgram *shader);
	//! Sets up shader uniforms specific to one material
	void setupMaterialUniforms(QOpenGLShaderProgram *shader, const OBJ::Material& mat);

	//! Finds the correct light source out of Sun, Moon, Venus, and returns ambient and directional light components.
	Scenery3d::ShadowCaster calculateLightSource(float &ambientBrightness, float &diffuseBrightness, Vec3f &lightsourcePosition, float &emissiveFactor);

	//! @return height at -absolutePosition, which is the current eye point.
	float groundHeight();

	//Sets the scenes' AABB
	void setSceneAABB(const AABB &bbox);

	//Save the Frustum to be able to move away from it and analyze it
	void saveFrusts();

	//! Adjust the frustum to the loaded scene bounding box according to Zhang et al.
	void adjustShadowFrustum(const Vec3d viewPos, const Vec3d viewDir, const Vec3d viewUp, const float fov, const float aspect);
	//Computes the frustum splits
	void computeFrustumSplits(const Vec3d viewPos, const Vec3d viewDir, const Vec3d viewUp);
	//Computes the focus body for given frustum
	void computePolyhedron(Polyhedron& body, const Frustum& frustum, const Vec3f &shadowDir);
	//Computes the crop matrix to focus the light
	void computeCropMatrix(QMatrix4x4& cropMatrix, QVector4D &orthoScale, Polyhedron &focusBody, const QMatrix4x4 &lightProj, const QMatrix4x4 &lightMVP);
	//Computes the light projection values
	void computeOrthoProjVals(const Vec3f shadowDir, float &orthoExtent, float &orthoNear, float &orthoFar);
};

#endif
