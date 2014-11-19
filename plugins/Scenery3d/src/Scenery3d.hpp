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

//predeclarations
class QOpenGLFramebufferObject;

//! Representation of a complete 3D scenery
class Scenery3d
{
public:
    //! Initializes an empty Scenery3d object.
    Scenery3d();
    virtual ~Scenery3d();

    //! Sets the shaders for the plugin
    void setShaders(QOpenGLShaderProgram* shadowShader = 0, QOpenGLShaderProgram* bumpShader = 0, QOpenGLShaderProgram* univShader = 0, QOpenGLShaderProgram* debugShader = 0)
    {
        this->shadowShader = shadowShader;
        this->bumpShader = bumpShader;
        this->univShader = univShader;
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
    void drawCoordinatesText(StelCore* core);
    //! Draw some text output. This can be filled as needed by development.
    void drawDebug(StelCore* core);

    //! Draw scenery, called by Scenery3dMgr.
    void draw(StelCore* core);
    //! Initializes shadow mapping
    void initShadowMapping();

    //! Gets the current scene's metadata
    SceneInfo getCurrentScene() { return currentScene; }

    bool getDebugEnabled() const { return debugEnabled; }
    void setDebugEnabled(bool debugEnabled) { this->debugEnabled = debugEnabled; }
    bool getShadowsEnabled(void) const { return shadowsEnabled; }
    void setShadowsEnabled(bool shadowsEnabled) { this->shadowsEnabled = shadowsEnabled; }
    bool getBumpsEnabled(void) const { return bumpsEnabled; }
    void setBumpsEnabled(bool bumpsEnabled) { this->bumpsEnabled = bumpsEnabled; }
    bool getTorchEnabled(void) const { return torchEnabled;}
    void setTorchEnabled(bool torchEnabled) { this->torchEnabled = torchEnabled; }
    bool getShadowsFilterEnabled(void) const { return filterShadowsEnabled; }
    void setShadowsFilterEnabled(bool filterShadowsEnabled) { this->filterShadowsEnabled = filterShadowsEnabled; }
    bool getShadowsFilterHQEnabled(void) const { return filterHQ; }
    void setShadowsFilterHQEnabled(bool filterHQ) { this->filterHQ = filterHQ; }
    bool getLocationInfoEnabled(void) const { return textEnabled; }
    void setLocationInfoEnabled(bool locationinfoenabled) { this->textEnabled = locationinfoenabled; }

    uint getCubemapSize() const { return cubemapSize; }
    void setCubemapSize(uint size) { cubemapSize = size; }
    uint getShadowmapSize() const { return shadowmapSize; }
    void setShadowmapSize(uint size) { shadowmapSize = size; }
    float getTorchBrightness() const { return torchBrightness; }
    void setTorchBrightness(float brightness) { torchBrightness = brightness; }

    enum ShadowCaster { None, Sun, Moon, Venus };
    enum Effect { No, BumpMapping, ShadowMapping, All};
    Mat4d mv;
    Mat4d mp;
    Mat4f mv2;
    Vec3d viewUp;
    Vec3d viewDir;
    Vec3d viewPos;
    int drawn;

private:
    SceneInfo currentScene;

    float torchBrightness; // ^L toggle light brightness

    void drawObjModel(StelCore* core);
    void generateShadowMap(StelCore* core);
    void generateCubeMap();
    void generateCubeMap_drawScene();
    void generateCubeMap_drawSceneWithShadows();
    void drawArrays(bool textures=true);
    void drawFromCubeMap(StelCore* core);

    //! @return height at -absolutePosition, which is the current eye point.
    float groundHeight();

    bool hasModels;             // flag to see if there's anything to draw
    bool shadowsEnabled;        // switchable value (^SPACE): Use shadow mapping
    bool bumpsEnabled;          // switchable value (^B): Use bump mapping
    bool textEnabled;           // switchable value (^K): display coordinates on screen. THIS IS NOT FOR DEBUGGING, BUT A PROGRAM FEATURE!
    bool torchEnabled;          // switchable value (^L): adds artificial ambient light
    bool debugEnabled;          // switchable value (^D): display debug graphics and debug texts on screen
    bool lightCamEnabled;       // switchable value: switches camera to light camera
    bool frustEnabled;
    bool filterShadowsEnabled;  // switchable value (^I): Filter shadows
    bool filterHQ;              // switchable value (^U): 64 tap filter shadows
    bool venusOn;

    unsigned int cubemapSize;            // configurable values, typically 512/1024/2048/4096
    unsigned int shadowmapSize;

    Vec3d absolutePosition;     // current eyepoint in model
    float movement_x;           // speed values for moving around the scenery
    float movement_y;
    float movement_z;
    float eye_height;

    StelCore* core;
    OBJ* objModel;
    OBJ* groundModel;
    Heightmap* heightmap;
    OBJ::vertexOrder objVertexOrder; // some OBJ files have left-handed coordinate counting or swapped axes. Allows accounting for those.

    Mat4f projectionMatrix;
    Mat4f lightViewMatrix;
    Mat4f lightProjectionMatrix;
    QOpenGLFramebufferObject* shadowMapFbo;
    QOpenGLFramebufferObject* cubeMap[6]; // front, right, left, back, top, bottom
    StelVertexArray cubePlaneFront, cubePlaneBack,
                cubePlaneLeft, cubePlaneRight,
                cubePlaneTop, cubePlaneBottom;

    QVector<OBJ::StelModel> objModelArrays;


    QString lightMessage; // DEBUG/TEST ONLY. contains on-screen info on ambient/directional light strength and source.
    QString lightMessage2; // DEBUG/TEST ONLY. contains on-screen info on ambient/directional light strength and source.
    QString lightMessage3; // DEBUG/TEST ONLY. contains on-screen info on ambient/directional light strength and source.

    //Combines zRot and rot2grid from scene metadata
    Mat4d zRot2Grid;

    //Currently selected Shader
    QOpenGLShaderProgram* curShader;
    //Shadow mapping shader + per pixel lighting
    QOpenGLShaderProgram* shadowShader;
    //Bump mapping shader
    QOpenGLShaderProgram* bumpShader;
    //Universal shader: shadow + bump mapping
    QOpenGLShaderProgram* univShader;
    //Debug shader
    QOpenGLShaderProgram* debugShader;
    //Depth texture id
    GLuint shadowMapTexture;
    //Shadow Map FBO handle
    GLuint shadowFBO;
    //Currently selected effect
    Effect curEffect;
    //Sends texture data to the shader based on which effect is selected;
    void sendToShader(const OBJ::StelModel* pStelModel, Effect cur, bool& tangEnabled, int& tangLocation);
    //Binds the shader for the selected effect
    void bindShader();
    //Prepare ambient and directional light components from Sun, Moon, Venus.
    Scenery3d::ShadowCaster setupLights(float &ambientBrightness, float &diffuseBrightness, Vec3f &lightsourcePosition);
    //Set independent brightness factors (allow e.g. solar twilight ambient&lunar specular). Call setupLights first!
    void setLights(float ambientBrightness, float diffuseBrightness);
    //Current Model View Matrix
    Mat4f modelView;
    //Current sun position
    Vec3d sunPosition;
    //Sets the scenes' AABB
    void setSceneAABB(AABB* bbox);
    //Renders the Scene's AABB
    void renderSceneAABB(StelPainter &painter);
    //Renders the Frustum
    void renderFrustum(StelPainter &painter);
    //Scene AABB
    AABB sceneBoundingBox;
    //Camera Frustum
    Frustum cFrust;
    //Save the Frustum to be able to move away from it and analyze it
    void saveFrusts();
    //Holds the shadow maps
    GLuint* shadowMapsArray;
    //Holds the shadow transformation matrix per split
    Mat4f* shadowCPM;
    //Number of splits for CSM
    int frustumSplits;
    //Weight for splitting the frustums
    float splitWeight;
    //Array holding the split frustums
    Frustum* frustumArray;
    //Vector holding the convex split bodies for focused shadow mapping
    std::vector<Polyhedron*> focusBodies;
    //Camera values
    float camNear, camFar, camFOV, camAspect;
    //Holds the light direction of the current light
    Vec3f lightDir;
    //Adjust the frustum to the loaded scene bounding box according to Zhang et al.
    void adjustFrustum();
    //Computes the frustum splits
    void computeZDist(float zNear, float zFar);
    //Computes the focus body for given split
    void computePolyhedron(int splitIndex);
    //Computes the crop matrix to focus the light
    void computeCropMatrix(int frustumIndex);
    //Computes the light projection values
    void computeOrthoProjVals();
    //Said values
    float dim, dimNear, dimFar;
    //Analyzes the view samples to find even tighter fitting near and far planes
    void analyzeViewSamples(StelPainter &painter);
    GLuint camDepthFBO;
    GLuint camDepthTex;
    float parallaxScale;
    int vpWidth, vpHeight;
    GLuint camDepthPBO;
    GLuint *pingpongTex;
    int writeTex, readTex;
    GLenum *attachments;

    QOpenGLShaderProgram *minMaxShader;

    QFont debugTextFont;

    //! Loads the model contained in the current scene
    void loadModel();
    void drawFullscreenQuad(int dim);
    //TODO FS: only temporary, will be removed
    void nogluLookAt(double eyeX,  double eyeY,  double eyeZ,  double centerX,  double centerY,  double centerZ,  double upX,  double upY,  double upZ);
};
#endif
