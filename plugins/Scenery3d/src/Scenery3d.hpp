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

#include "fixx11h.h"
#include "StelGui.hpp"
#include "StelModule.hpp"
#include "StelPainter.hpp"
#include "Landscape.hpp"
#include "OBJ.hpp"
#include "Heightmap.hpp"

#include <QString>
#include <vector>
#include <QGLFramebufferObject>

#include "StelShader.hpp"

using std::vector;

//! Representation of a complete 3D scenery
class Scenery3d
{
public:
    //! Initializes an empty Scenery3d object.
    //! @param cbmSize Size of the cubemap to use for indirect rendering.
    Scenery3d(int cubemapSize=1024, int shadowmapSize=1024);
    virtual ~Scenery3d();

    //! Sets the shaders for the plugin
    void setShaders(StelShader* shadowShader = 0, StelShader* bumpShader = 0, StelShader* univShader = 0)
    {
        this->shadowShader = shadowShader;
        this->bumpShader = bumpShader;
        this->univShader = univShader;
    }

    //! Loads configuration values from a scenery3d.ini file.
    void loadConfig(const QSettings& scenery3dIni, const QString& scenery3dID);
    //! Loads the model given in the scenery3d.ini file.
    //! Make sure to call .loadConfig() before this.
    void loadModel();
	
    //! Walk/Fly Navigation with Ctrl+Cursor and Ctrl+PgUp/Dn keys.
    //! Pressing Ctrl-Alt: 5x, Ctrl-Shift: 10x speedup; Ctrl-Shift-Alt: 50x!
    //! To allow fine control, zoom in.
    //! If you release Ctrl key while pressing cursor key, movement will continue.
    void handleKeys(QKeyEvent* e);

    //! Update method, called by Scenery3dMgr.
    //! Shifts observer position due to movement through the landscape.
    void update(double deltaTime);
    //! Draw observer grid coordinates as text, called by Scenery3dMgr.
    void drawCoordinatesText(StelCore* core);
    //! Draw scenery, called by Scenery3dMgr.
    void draw(StelCore* core);
    //! Initializes shadow mapping
    void initShadowMapping();

    bool getShadowsEnabled(void) { return shadowsEnabled; }
    void setShadowsEnabled(bool shadowsEnabled) { this->shadowsEnabled = shadowsEnabled; }
    bool getBumpsEnabled(void) { return bumpsEnabled; }
    void setBumpsEnabled(bool bumpsEnabled) { this->bumpsEnabled = bumpsEnabled; }
    bool getTorchEnabled(void) { return torchEnabled;}
    void setTorchEnabled(bool torchEnabled) { this->torchEnabled = torchEnabled; }

    //! @return Name of the scenery.
    QString getName() const { return name; }
    //! @return Author of the scenery.
    QString getAuthorName() const { return authorName; }
    //! @return Description of the scenery.
    QString getDescription() const { return description; }
    //! @return Name of the landscape associated with the scenery.
    QString getLandscapeName() const { return landscapeName; }
    //! @return Flag, whether scenery provides location data or only inherits from its linked Landscape.
    bool hasLocation() const { return (location != NULL); }
    //! @return Location data. These are valid only if written in the @file scenery3d.ini, section location.
    //! Else, returns NULL. You can also check with .hasLocation() before calling.
    const StelLocation& getLocation() const {return *location; }
    //! @return Flag, whether scenery provides default/startup looking direction.
    bool hasLookat() const { return (lookAt_fov[2]!=-1000.0f); }
    //! @return default looking direction and field-of-view (altazimuthal, 0=North).
    //! Valid only if written in the @file scenery3d.ini, value: coord/start_az_alt_fov.
    //! Else, returns NULL. You can also check with .hasLookup() before calling.
    const Vec3f& getLookat() const {return lookAt_fov; }


    enum ShadowCaster { None, Sun, Moon, Venus };
    enum Effect { No, BumpMapping, ShadowMapping, All };

private:
    static const float TORCH_BRIGHTNESS=0.5f;
    static const float AMBIENT_BRIGHTNESS_FACTOR=0.05;
    static const float LUNAR_BRIGHTNESS_FACTOR=0.2;
    static const float VENUS_BRIGHTNESS_FACTOR=0.05;

    double eyeLevel;

    void drawObjModel(StelCore* core);
    void generateShadowMap(StelCore* core);
    void generateCubeMap(StelCore* core);
    void generateCubeMap_drawScene(StelPainter& painter, float ambientBrightness, float directionalBrightness);
    void generateCubeMap_drawSceneWithShadows(StelPainter& painter, float ambientBrightness, float directionalBrightness);
    void drawArrays(StelPainter& painter, bool textures=true);
    void drawFromCubeMap(StelCore* core);

    //! @return height at -absolutePosition, which is the current eye point.
    float groundHeight();

    bool shadowsEnabled;    // switchable value: Use shadow mapping
    bool bumpsEnabled;      // switchable value: Use bump mapping
    bool textEnabled;       // switchable value: display coordinates on screen
    bool torchEnabled;      // switchable value: adds artificial ambient light

    int cubemapSize;        // configurable values, typically 512/1024/2048/4096
    int shadowmapSize;

    Vec3d absolutePosition; // current eyepoint in model
    float movement_x;       // speed values for moving around the scenery
    float movement_y;
    float movement_z;

    StelCore* core;
    OBJ* objModel;
    OBJ* groundModel;
    Heightmap* heightmap;
    OBJ::vertexOrder objVertexOrder; // some OBJ files have left-handed coordinate counting or swapped axes. Allows accounting for those.

    Mat4f projectionMatrix;
    Mat4f lightViewMatrix;
    Mat4f lightProjectionMatrix;
    QGLFramebufferObject* shadowMapFbo;
    QGLFramebufferObject* cubeMap[6]; // front, right, left, back, top, bottom
    StelVertexArray cubePlaneFront, cubePlaneBack,
                cubePlaneLeft, cubePlaneRight,
                cubePlaneTop, cubePlaneBottom;

    vector<OBJ::StelModel> objModelArrays;

    QString id;
    QString name;
    QString authorName;
    QString description;
    QString landscapeName;
    QString modelSceneryFile;
    QString modelGroundFile;
    StelLocation* location;
    Vec3f lookAt_fov; // (az_deg, alt_deg, fov_deg)
    Vec3d modelWorldOffset; // required for coordinate display
    QString gridName;
    double gridCentralMeridian;
    double groundNullHeight; // Used as height value outside the model ground, or if ground=NULL
    QString lightMessage; // DEBUG/TEST ONLY. contains on-screen info on ambient/directional light strength and source.
    QString lightMessage2; // DEBUG/TEST ONLY. contains on-screen info on ambient/directional light strength and source.
    QString lightMessage3; // DEBUG/TEST ONLY. contains on-screen info on ambient/directional light strength and source.

    // used to apply the rotation from model/grid coordinates to Stellarium coordinates.
    // In the OBJ files, X=Grid-East, Y=Grid-North, Z=height.
    Mat4d zRotateMatrix;
    // if only a non-georeferenced OBJ can be provided, you can specify a matrix via .ini/[model]/obj_world_trafo.
    // This will be applied to make sure that X=Grid-East, Y=Grid-North, Z=height.
    Mat4d obj2gridMatrix;

    //Currently selected Shader
    StelShader* curShader;
    //Shadow mapping shader + per pixel lighting
    StelShader* shadowShader;
    //Bump mapping shader
    StelShader* bumpShader;
    //Universal shader: shadow + bump mapping
    StelShader* univShader;
    //Depth texture id
    GLuint shadowMapTexture;
    //Shadow Map FBO handle
    GLuint shadowFBO;
    //Currently selected effect
    Effect curEffect;
    //Sends texture data to the shader based on which effect is selected;
    void sendToShader(OBJ::StelModel& stelModel, Effect cur);
    //Binds the shader for the selected effect
    void bindShader();

    void testMethod();
    //Prepare ambient and directional light components from Sun, Moon, Venus.
    Scenery3d::ShadowCaster setupLights(float &ambientBrightness, float &diffuseBrightness, Vec3f &lightsourcePosition);
    //Set independent brightness factors (allow e.g. solar twilight ambient&lunar specular). Call setupLights first!
    void setLights(float ambientBrightness, float diffuseBrightness);

    Mat4f modelView;
    Mat4f mv2;
};

#endif
