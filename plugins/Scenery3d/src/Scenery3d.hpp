/*
 * Stellarium Scenery3d Plug-in
 * 
 * Copyright (C) 2011 Simon Parzer, Peter Neubauer, Georg Zotti
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
#include "OBJ.hpp"
#include "Heightmap.hpp"
#include "Frustum.hpp"
#include "Polyhedron.hpp"
#include "SceneInfo.hpp"
#include "ShaderManager.hpp"

#include <QMatrix4x4>

//predeclarations
class QOpenGLFramebufferObject;
class Scenery3dMgr;

//! Representation of a complete 3D scenery
class Scenery3d
{
public:
    //! Initializes an empty Scenery3d object.
    Scenery3d(Scenery3dMgr *parent);
    virtual ~Scenery3d();

    //! Sets the shaders for the plugin
    void setShaders(QOpenGLShaderProgram* debugShader = 0)
    {
        this->debugShader = debugShader;
    }

    //! Loads the specified scene
    void loadScene(const SceneInfo& scene);

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
    void setPixelLightingEnabled(const bool val) { shaderParameters.pixelLighting = val; }
    bool getShadowsEnabled(void) const { return shaderParameters.shadows; }
    void setShadowsEnabled(bool shadowsEnabled) { shaderParameters.shadows = shadowsEnabled; }
    bool getBumpsEnabled(void) const { return shaderParameters.bump; }
    void setBumpsEnabled(bool bumpsEnabled) { shaderParameters.bump = bumpsEnabled; }
    bool getTorchEnabled(void) const { return torchEnabled;}
    void setTorchEnabled(bool torchEnabled) { this->torchEnabled = torchEnabled; }
    bool getShadowsFilterEnabled(void) const { return shaderParameters.shadowFilter; }
    void setShadowsFilterEnabled(bool filterShadowsEnabled) { shaderParameters.shadowFilter = filterShadowsEnabled; }
    bool getShadowsFilterHQEnabled(void) const { return shaderParameters.shadowFilterHQ; }
    void setShadowsFilterHQEnabled(bool filterHQ) { shaderParameters.shadowFilterHQ = filterHQ; }
    bool getLocationInfoEnabled(void) const { return textEnabled; }
    void setLocationInfoEnabled(bool locationinfoenabled) { this->textEnabled = locationinfoenabled; }

    bool getGeometryShaderCubemapEnabled() { return useGSCubemapping; }
    void setGeometryShaderCubemapEnabled(bool val) { useGSCubemapping = val; reinitCubemapping = true; }
    bool isGeometryShaderCubemapSupported() { return supportsGSCubemapping; }

    uint getCubemapSize() const { return cubemapSize; }
    void setCubemapSize(uint size) { cubemapSize = size; }
    uint getShadowmapSize() const { return shadowmapSize; }
    void setShadowmapSize(uint size) { shadowmapSize = size; }
    float getTorchBrightness() const { return torchBrightness; }
    void setTorchBrightness(float brightness) { torchBrightness = brightness; }

    enum ShadowCaster { None, Sun, Moon, Venus };

    //! Returns the shader manager this instance uses
    ShaderMgr& getShaderManager()    {	    return shaderManager;    }

private:
    Scenery3dMgr* parent;
    SceneInfo currentScene;
    ShaderMgr shaderManager;

    float torchBrightness; // ^L toggle light brightness

    bool hasModels;             // flag to see if there's anything to draw
    bool textEnabled;           // switchable value (^K): display coordinates on screen. THIS IS NOT FOR DEBUGGING, BUT A PROGRAM FEATURE!
    bool torchEnabled;          // switchable value (^L): adds artificial ambient light
    bool debugEnabled;          // switchable value (^D): display debug graphics and debug texts on screen
    bool lightCamEnabled;       // switchable value: switches camera to light camera
    bool frustEnabled;
    bool venusOn;
    bool supportsGSCubemapping; //if the GL context supports geometry shader cubemapping
    bool useGSCubemapping;
    bool reinitCubemapping;

    unsigned int cubemapSize;            // configurable values, typically 512/1024/2048/4096
    unsigned int shadowmapSize;

    Vec3d absolutePosition;     // current eyepoint in model
    Vec3f movement;		// speed values for moving around the scenery
    float eye_height;

    StelCore* core;
    StelProjectorP altAzProjector;
    OBJ objModel;
    OBJ groundModel;
    Heightmap* heightmap;
    OBJ::vertexOrder objVertexOrder; // some OBJ files have left-handed coordinate counting or swapped axes. Allows accounting for those.

    Vec3d viewUp;
    Vec3d viewDir;
    Vec3d viewPos;
    int drawnTriangles;

    //QOpenGLFramebufferObject* cubeMap[6]; // front, right, left, back, top, bottom
    GLuint cubeMapTex; //GL_TEXTURE_CUBE_MAP
    GLuint cubeMapDepth;
    GLuint cubeRB; //renderbuffer for depth
    GLuint cubeFBO; //because of use that deviates very much from QOpenGLFramebufferObject typical usage, we manage the FBO ourselves
    QVector<Vec3f> cubeVertices;
    QVector<unsigned short> cubeIndices;
    QMatrix4x4 cubeRotation[6]; //rotational matrices for cube faces
    QMatrix4x4 cubeMVP[6]; //cube face MVP matrices

    QString lightMessage; // DEBUG/TEST ONLY. contains on-screen info on ambient/directional light strength and source.
    QString lightMessage2; // DEBUG/TEST ONLY. contains on-screen info on ambient/directional light strength and source.
    QString lightMessage3; // DEBUG/TEST ONLY. contains on-screen info on ambient/directional light strength and source.

    //Combines zRot and rot2grid from scene metadata
    Mat4d zRot2Grid;


    /// ---- Rendering information ----
    //final model view matrix for shader upload
    QMatrix4x4 modelViewMatrix;
    //currently valid projection matrix for shader upload
    QMatrix4x4 projectionMatrix;

    // a struct which encapsulates lighting info
    struct LightParameters
    {
	    ShadowCaster lightSource;
	    ShadowCaster shadowCaster;
	    QVector3D lightDirectionWorld;
	    QVector3D ambient;
	    QVector3D directional;
    } lightInfo;

    GlobalShaderParameters shaderParameters;

    //Debug shader
    QOpenGLShaderProgram* debugShader;

    //Scene AABB
    AABB sceneBoundingBox;

    //Shadow Map FBO handles
    QVector<GLuint> shadowFBOs;
    //Holds the shadow textures
    QVector<GLuint> shadowMapsArray;
    //Holds the shadow transformation matrix per split (Crop/Projection/View)
    QVector<QMatrix4x4> shadowCPM;
    //Number of splits for CSM
    int frustumSplits;
    //Weight for splitting the frustums
    float splitWeight;
    //Array holding the split frustums
    QVector<Frustum> frustumArray;
    //Vector holding the convex split bodies for focused shadow mapping
    QVector<Polyhedron> focusBodies;
    //Camera values
    float camNear, camFar, camFOV, camAspect;


    float parallaxScale;

    QFont debugTextFont;

    // --- initialization
    //! Initializes cubemapping to the currently set parameters (6tex/cubemap/GS approaches)
    void initCubemapping();
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
    //! Setup shadow map information + render
    bool generateShadowMap();
    //! Performs the actual rendering of the shadow map
    bool renderShadowMaps(const Vec3f &shadowDir);
    //! Generates a 6-sided cube map by drawing a view in each direction
    void generateCubeMap();
    //! Uses the StelPainter to draw a warped cube textured with our cubemap
    void drawFromCubeMap();
    //! This is the method that performs the actual drawing.
    //! If shading is true, a suitable shader for each material is selected and initialized. Submits 1 draw call for each StelModel.
    void drawArrays(bool shading=true, bool blendAlphaAdditive=false);


    // --- shading related stuff ---
    //! Calculates lighting related stuff and puts it in the lightInfo structure
    void calculateLighting();
    //! Sets uniforms constant over the whole pass (=projection matrix, lighting & shadow info)
    void setupPassUniforms(QOpenGLShaderProgram *shader);
    //! Sets up shader uniforms constant over the whole frame/side of cubemap (=modelview dependent stuff)
    void setupFrameUniforms(QOpenGLShaderProgram *shader);
    //! Sets up shader uniforms specific to one material
    void setupMaterialUniforms(QOpenGLShaderProgram *shader, const OBJ::Material& mat);

    //! Finds the correct light source out of Sun, Moon, Venus, and returns ambient and directional light components.
    Scenery3d::ShadowCaster calculateLightSource(float &ambientBrightness, float &diffuseBrightness, Vec3f &lightsourcePosition);

    //! @return height at -absolutePosition, which is the current eye point.
    float groundHeight();

    //Sets the scenes' AABB
    void setSceneAABB(const AABB &bbox);
    //Renders the Scene's AABB
    void renderSceneAABB(StelPainter &painter);
    //Renders the Frustum
    void renderFrustum(StelPainter &painter);

    //Save the Frustum to be able to move away from it and analyze it
    void saveFrusts();

    //Adjust the frustum to the loaded scene bounding box according to Zhang et al.
    void adjustFrustum();
    //Computes the frustum splits
    void computeFrustumSplits(float zNear, float zFar);
    //Computes the focus body for given frustum
    void computePolyhedron(Polyhedron& body, const Frustum& frustum, const Vec3f &shadowDir);
    //Computes the crop matrix to focus the light
    void computeCropMatrix(int frustumIndex, const Vec3f &shadowDir);
    //Computes the light projection values
    void computeOrthoProjVals(const Vec3f shadowDir, float &orthoExtent, float &orthoNear, float &orthoFar);

    inline QMatrix4x4 convertToQMatrix(const Mat4d& mat);

    //! Loads the model contained in the current scene
    void loadModel();
};

QMatrix4x4 Scenery3d::convertToQMatrix(const Mat4d &mat)
{
	return QMatrix4x4( mat.r[0], mat.r[4], mat.r[8],mat.r[12],
			   mat.r[1], mat.r[5], mat.r[9],mat.r[13],
			   mat.r[2], mat.r[6],mat.r[10],mat.r[14],
			   mat.r[3], mat.r[7],mat.r[11],mat.r[15] );
}

#endif
