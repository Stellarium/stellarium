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

using std::vector;

//! Representation of a complete 3D scenery
class Scenery3d
{
public:
    //! Initializes an empty Scenery3d object.
    //! @param cbmSize Size of the cubemap to use for indirect rendering.
    Scenery3d(int cubemapSize=1024, int shadowmapSize=1024);
    virtual ~Scenery3d();

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

    bool getShadowsEnabled(void) { return shadowsEnabled; }
    void setShadowsEnabled(bool shadowsEnabled) { this->shadowsEnabled = shadowsEnabled; }

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


    enum shadowCaster { None, Sun, Moon };

private:
    double eyeLevel;

    void drawCubeTestScene(StelCore* core);
    void drawObjModel(StelCore* core);
    void generateShadowMap(StelCore* core);
    void generateCubeMap(StelCore* core);
    void generateCubeMap_drawScene(StelPainter& painter, float lightBrightness);
    void generateCubeMap_drawSceneWithShadows(StelPainter& painter, float lightBrightness);
    void drawArrays(StelPainter& painter, bool textures=true);
    void drawFromCubeMap(StelCore* core);

    //! @return height at -absolutePosition, which is the current eye point.
    float groundHeight();

    bool shadowsEnabled;
    bool textEnabled;

    StelCore* core;
    int cubemapSize;
    int shadowmapSize;

    Mat4f projectionMatrix;
    Vec3d absolutePosition;
    float movement_x;
    float movement_y;
    float movement_z;

    OBJ* objModel;
    OBJ* groundModel;
    OBJ::vertexOrder objVertexOrder;
    GLuint shadowMapTexture;
    Mat4f lightViewMatrix;
    Mat4f lightProjectionMatrix;
    QGLFramebufferObject* shadowMapFbo;
    QGLFramebufferObject* cubeMap[6]; // front, right, left, back, top, bottom
    StelVertexArray cubePlane, cubePlaneBack,
                cubePlaneLeft, cubePlaneRight,
                cubePlaneTop, cubePlaneBottom;

    vector<OBJ::StelModel> objModelArrays;
    Heightmap* heightmap;

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
    double groundNullHeight; // Used as height value outside the model ground

    // used to apply the rotation from model/grid coordinates to Stellarium coordinates.
    // In the OBJ files, X=Grid-East, Y=Grid-North, Z=height.
    Mat4d zRotateMatrix;
};

#endif
