/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011-12 Simon Parzer, Peter Neubauer, Andrei Borza, Georg Zotti
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



#include <QtGlobal>


#include "Scenery3d.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelOpenGL.hpp"
#include "StelPainter.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelUtils.hpp"

#include "SolarSystem.hpp"

#include "Scenery3dMgr.hpp"
#include "AABB.hpp"

#include <QSettings>
#include <stdexcept>
#include <cmath>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>

#include <limits>
#include <sstream>

#include <iostream>
#include <fstream>

#define GROUND_MODEL 0
#define GET_GLERROR()                                   \
{                                                       \
    GLenum err = glGetError();                          \
    if (err != GL_NO_ERROR) {                           \
    fprintf(stderr, "[line %d] GL Error: %s\n",         \
    __LINE__, gluErrorString(err));                     \
    fflush(stderr);                                     \
    }                                                   \
}

#define MAXSPLITS 4
#define NEARZ 1.0f;
//If FARZ is too small for your model change it up
#define FARZ 15000.0f


static const float AMBIENT_BRIGHTNESS_FACTOR=0.05f;
static const float LUNAR_BRIGHTNESS_FACTOR=0.2f;
static const float VENUS_BRIGHTNESS_FACTOR=0.005f;


Scenery3d::Scenery3d()
    :absolutePosition(0.0, 0.0, 0.0), // 1.E-12, 1.E-12, 1.E-12), // these values signify "set default values"
    movement_x(0.0f), movement_y(0.0f), movement_z(0.0f),core(NULL),
    objModel(NULL), groundModel(NULL), heightmap(NULL), currentScene(),
    cubemapSize(1024),shadowmapSize(1024),torchBrightness(0.5f)
{
	qDebug()<<"Scenery3d constructor...";
    objModel = new OBJ();
    groundModel = new OBJ();
    shadowMapTexture = 0;
    for (int i=0; i<6; i++) {
        cubeMap[i] = NULL;
    }
    shadowMapFbo = NULL;
    shadowFBO = 0;
    int sub = 20;
    double d_sub_v = 2.0 / sub;
    double d_sub_tex = 1.0 / sub;
    for (int y = 0; y < sub; y++) {
        for (int x = 0; x < sub; x++) {
            double x0 = -1.0 + x * d_sub_v;
            double x1 = x0 + d_sub_v;
            double y0 = -1.0 + y * d_sub_v;
            double y1 = y0 + d_sub_v;
            double tx0 = 0.0f + x * d_sub_tex;
            double tx1 = tx0 + d_sub_tex;
            double ty0 = 0.0f + y * d_sub_tex;
            double ty1 = ty0 + d_sub_tex;
            Vec3d v[] = {
                Vec3d(x0, 1.0, y0),
                Vec3d(x1, 1.0, y0),
                Vec3d(x1, 1.0, y1),
                Vec3d(x0, 1.0, y0),
                Vec3d(x1, 1.0, y1),
                Vec3d(x0, 1.0, y1),
            };
            for (int i=0; i<6; i++) {
                v[i].normalize();
                cubePlaneFront.vertex << v[i];
            }
            cubePlaneFront.texCoords << Vec2f(tx0, ty0)
                                << Vec2f(tx1, ty0)
                                << Vec2f(tx1, ty1)
                                << Vec2f(tx0, ty0)
                                << Vec2f(tx1, ty1)
                                << Vec2f(tx0, ty1);
        }
    }
    shadowsEnabled = false;
    bumpsEnabled = false;
    torchEnabled = false;
    textEnabled = false;
    debugEnabled = false;
    curEffect = No;
    curShader = 0;
    lightCamEnabled = false;
    hasModels = false;
    sceneBoundingBox = AABB(Vec3f(0.0f), Vec3f(0.0f));
    frustEnabled = false;
    filterShadowsEnabled = true;
    filterHQ = false;
    venusOn = false;

    //Preset frustumSplits
    frustumSplits = 4;
    //Make sure we dont exceed MAXSPLITS or go below 1
    frustumSplits = qMax(qMin(frustumSplits, MAXSPLITS), 1);
    //Define shadow maps array - holds MAXSPLITS textures
    shadowMapsArray = new GLuint[frustumSplits];
    shadowCPM = new Mat4f[frustumSplits];
    frustumArray = new Frustum[frustumSplits];

    camFOV = 90.0f;
    camAspect = 1.0f;
    camNear = NEARZ;
    camFar = FARZ;
    dim = 1.0f;
    dimNear = 1.0f;
    dimFar = dimNear+1.0f;

    parallaxScale = 0.015f;

    camDepthFBO = 0;
    camDepthTex = 0;

    writeTex = 0;
    readTex = 1;
    attachments = new GLenum[2];
    attachments[0] = GL_COLOR_ATTACHMENT0;
    attachments[1] = GL_COLOR_ATTACHMENT1;
    pingpongTex = new GLuint[2];


    Mat4d matrix;
#define PLANE(_VAR_, _MAT_) matrix=_MAT_; _VAR_=StelVertexArray(cubePlaneFront.vertex,StelVertexArray::Triangles,cubePlaneFront.texCoords);\
                        for(int i=0;i<_VAR_.vertex.size();i++){ matrix.transfo(_VAR_.vertex[i]); }
    PLANE(cubePlaneRight, Mat4d::zrotation(-M_PI_2))
    PLANE(cubePlaneLeft, Mat4d::zrotation(M_PI_2))
    PLANE(cubePlaneBack, Mat4d::zrotation(M_PI))
    PLANE(cubePlaneTop, Mat4d::xrotation(-M_PI_2))
    PLANE(cubePlaneBottom, Mat4d::xrotation(M_PI_2))
#undef PLANE

	debugTextFont.setFamily("Courier");
	debugTextFont.setPixelSize(16);
	qDebug()<<"Scenery3d constructor...done";
}

Scenery3d::~Scenery3d()
{
    if (heightmap) {
        delete heightmap;
        heightmap = NULL;
    }
    for(int i=0; i<frustumSplits; i++)
    {
	    if(shadowMapsArray[i] != 0)
	    {
		    glDeleteTextures(1, &shadowMapsArray[i]);
		    shadowMapsArray[i] = 0;
	    }
    }
    delete shadowMapFbo;
    shadowMapFbo = NULL;

    for (int i=0; i<6; i++) {
        if (cubeMap[i] != NULL) {
            delete cubeMap[i];
            cubeMap[i] = NULL;
        }
    }
    if (groundModel != objModel)
    {
        delete groundModel;
        groundModel = NULL;
    }

    if (objModel)
    {
        delete objModel;
        objModel = NULL;
    }

    for(unsigned int i=0; i<focusBodies.size(); i++)
    {
        delete focusBodies[i];
    }

    focusBodies.clear();
}

void Scenery3d::loadScene(const SceneInfo &scene)
{
	currentScene = scene;

	//setup some state
	zRot2Grid = scene.zRotateMatrix*scene.obj2gridMatrix;

	objVertexOrder=OBJ::XYZ;
	if (currentScene.vertexOrder.compare("XZY") == 0) objVertexOrder=OBJ::XZY;
	else if (currentScene.vertexOrder.compare("YXZ") == 0) objVertexOrder=OBJ::YXZ;
	else if (currentScene.vertexOrder.compare("YZX") == 0) objVertexOrder=OBJ::YZX;
	else if (currentScene.vertexOrder.compare("ZXY") == 0) objVertexOrder=OBJ::ZXY;
	else if (currentScene.vertexOrder.compare("ZYX") == 0) objVertexOrder=OBJ::ZYX;

	loadModel();
}

void Scenery3d::loadModel()
{
        if(objModel != NULL)
        {
	    objModel->clean();
	    qDebug()<<"OBJ memory after clean: "<<objModel->memoryUsage();
        }

	QString modelFile = StelFileMgr::findFile( currentScene.fullPath+ "/" + currentScene.modelScenery);
	if(!objModel->load(modelFile, objVertexOrder, currentScene.sceneryGenerateNormals))
            throw std::runtime_error("Failed to load OBJ file.");

        hasModels = objModel->hasStelModels();
	//transform the vertices of the model to match the grid
	objModel->transform( zRot2Grid );
        //objModel->transform(obj2gridMatrix);

	qDebug()<<"OBJ memory after load: "<<objModel->memoryUsage();


        /* We could re-create zRotateMatrix here if needed: We may have "default" conditions with landscape coordinates
        // inherited from a landscape, or loaded from scenery3d.ini. In any case, at this point they should have been valid.
        // But it turned out that loading/setting the landscape works with a smooth transition, therefore at this point,
        // current location might still be the old location, before the location set in the landscape background takes over.
        // So, computing rot_z and zRotateMatrix absolutely requires a location section in our scenery3d.ini and our own location.
        //if (rot_z==-360.0){ // signal value indicating "recompute zRotateMatrix from new coordinates"
            //double lng =StelApp::getInstance().getCore()->getNavigator()->getCurrentLocation().longitude;
            //double lat =StelApp::getInstance().getCore()->getNavigator()->getCurrentLocation().latitude;
        //} */

	if (currentScene.modelGround.isEmpty())
            groundModel=objModel;
	else if (!currentScene.modelGround.compare("NULL"))
            groundModel=NULL;
        else
        {
            if(groundModel != NULL)
            {
		groundModel->clean();
            }

	    modelFile = StelFileMgr::findFile(currentScene.fullPath + "/" + currentScene.modelGround);
	    if(!groundModel->load(modelFile, objVertexOrder, currentScene.groundGenerateNormals))
                throw std::runtime_error("Failed to load OBJ file.");

	    groundModel->transform( zRot2Grid );
        }

	if (currentScene.hasLocation())
	{ if (currentScene.altitudeFromModel) // previouslay marked meaningless
          {
		currentScene.location->altitude=static_cast<int>(0.5*(objModel->getBoundingBox()->min[2]+objModel->getBoundingBox()->max[2])+currentScene.modelWorldOffset[2]);
          }
        }

	if (currentScene.groundNullHeightFromModel)
        {
	    currentScene.groundNullHeight=((groundModel!=NULL) ? groundModel->getBoundingBox()->min[2] : objModel->getBoundingBox()->min[2]);
            //groundNullHeight = objModel->getBoundingBox()->min[2];
	    qDebug() << "Ground outside model is " << currentScene.groundNullHeight  << "m high (in model coordinates)";
        }
	else qDebug() << "Ground outside model stays " << currentScene.groundNullHeight  << "m high (in model coordinates)";

        if (groundModel)
        {
            heightmap = new Heightmap(*groundModel);
	    heightmap->setNullHeight(currentScene.groundNullHeight);
        }

	if(currentScene.startPositionFromModel)
	{
		absolutePosition.v[0] = -(objModel->getBoundingBox()->max[0]+objModel->getBoundingBox()->min[0])/2.0;
		qDebug() << "Setting Easting  to BBX center: " << objModel->getBoundingBox()->min[0] << ".." << objModel->getBoundingBox()->max[0] << ": " << absolutePosition.v[0];
		absolutePosition.v[1] = -(objModel->getBoundingBox()->max[1]+objModel->getBoundingBox()->min[1])/2.0;
		qDebug() << "Setting Northing to BBX center: " << objModel->getBoundingBox()->min[1] << ".." << objModel->getBoundingBox()->max[1] << ": " << -absolutePosition.v[1];
	}
	else
	{
		absolutePosition[0] = currentScene.relativeStartPosition[0];
		absolutePosition[1] = currentScene.relativeStartPosition[1];
	}
	eye_height = currentScene.eyeLevel;

        OBJ* cur = objModel;
#if GROUND_MODEL
        cur = groundModel;
#endif

        //Set the scene's AABB
        setSceneAABB(cur->getBoundingBox());

        //finally, set core to enable update().
        this->core=StelApp::getInstance().getCore();

	//Add matrix to get into grid space
        cFrust.m = zRot2Grid;

        //Find a good splitweight based on the scene's size
        float maxSize = -std::numeric_limits<float>::max();
        maxSize = std::max(sceneBoundingBox.max.v[0], maxSize);
        maxSize = std::max(sceneBoundingBox.max.v[1], maxSize);

        //qDebug() << "MAXSIZE:" << maxSize;
        if(maxSize < 100.0f)
            splitWeight = 0.5f;
        else if(maxSize < 200.0f)
            splitWeight = 0.60f;
        else if(maxSize < 400.0f)
            splitWeight = 0.70f;
        else
            splitWeight = 0.99f;
}

void Scenery3d::handleKeys(QKeyEvent* e)
{
    if ((e->type() == QKeyEvent::KeyPress) && (e->modifiers() & Qt::ControlModifier))
    {
        // Pressing CTRL+ALT: 5x, CTRL+SHIFT: 10x speedup; CTRL+SHIFT+ALT: 50x!
        float speedup=((e->modifiers() & Qt::ShiftModifier)? 10.0f : 1.0f);
        speedup *= ((e->modifiers() & Qt::AltModifier)? 5.0f : 1.0f);
        switch (e->key())
	{
            case Qt::Key_PageUp:    movement_z = -1.0f * speedup; e->accept(); break;
            case Qt::Key_PageDown:  movement_z =  1.0f * speedup; e->accept(); break;
            case Qt::Key_Up:        movement_y = -1.0f * speedup; e->accept(); break;
            case Qt::Key_Down:      movement_y =  1.0f * speedup; e->accept(); break;
            case Qt::Key_Right:     movement_x =  1.0f * speedup; e->accept(); break;
            case Qt::Key_Left:      movement_x = -1.0f * speedup; e->accept(); break;
	    case Qt::Key_P:         saveFrusts(); e->accept(); break;
        }
    }
    else if ((e->type() == QKeyEvent::KeyRelease) && (e->modifiers() & Qt::ControlModifier))
    {
        if (e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown ||
            e->key() == Qt::Key_Up     || e->key() == Qt::Key_Down     ||
            e->key() == Qt::Key_Left   || e->key() == Qt::Key_Right     )
            {
                movement_x = movement_y = movement_z = 0.0f;
                e->accept();
            }
    }
}

void Scenery3d::saveFrusts()
{
    frustEnabled = !frustEnabled;

    for(int i=0; i<frustumSplits; i++)
    {
        if(frustEnabled) frustumArray[i].saveCorners();
        else frustumArray[i].resetCorners();
    }
}

void Scenery3d::setLights(float ambientBrightness, float diffuseBrightness)
{   // N.B. ambientBrightness for flat shading, diffuse brightness also casts shadows!
    bool red=StelApp::getInstance().getVisionModeNight();
    //lightBrightness *= 0.5f; // GZ: WE WILL SEE...
    //GZ: to achieve brighter surfaces and shadow effect, we use sqrt(diffuseBrightness):
    // NO LONGER - this had been done already...
    //float diffBrightness=std::sqrt(diffuseBrightness);
    const GLfloat LightAmbient[] = {ambientBrightness, (red? 0 : ambientBrightness), (red? 0 : ambientBrightness), 1.0f};
    const GLfloat LightDiffuse[] = {diffuseBrightness, (red? 0 : diffuseBrightness), (red? 0 : diffuseBrightness), 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);
}

void Scenery3d::setSceneAABB(AABB* bbox)
{
    sceneBoundingBox = AABB(bbox->min, bbox->max);
}

void Scenery3d::renderSceneAABB(StelPainter& painter)
{
    //sceneBoundingBox.render(&zRot2Grid);
    //sceneBoundingBox.render();

    Vec3d aabb[8];

    aabb[0] = vecfToDouble(this->sceneBoundingBox.getCorner(AABB::MinMinMin));
    aabb[1] = vecfToDouble(this->sceneBoundingBox.getCorner(AABB::MaxMinMin));
    aabb[2] = vecfToDouble(this->sceneBoundingBox.getCorner(AABB::MaxMinMax));
    aabb[3] = vecfToDouble(this->sceneBoundingBox.getCorner(AABB::MinMinMax));
    aabb[4] = vecfToDouble(this->sceneBoundingBox.getCorner(AABB::MinMaxMin));
    aabb[5] = vecfToDouble(this->sceneBoundingBox.getCorner(AABB::MaxMaxMin));
    aabb[6] = vecfToDouble(this->sceneBoundingBox.getCorner(AABB::MaxMaxMax));
    aabb[7] = vecfToDouble(this->sceneBoundingBox.getCorner(AABB::MinMaxMax));

    unsigned int inds[36] = {
        3, 2, 0,
        2, 1, 0,
        2, 7, 1,
        7, 4, 1,
        7, 6, 4,
        6, 5, 4,
        6, 3, 5,
        3, 0, 5,
        0, 1, 4,
        4, 5, 0,
        3, 2, 7,
        7, 6, 3
    };

    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    debugShader->bind();
    painter.setArrays(aabb, NULL, NULL, NULL);
    //TODO FS fix
    //painter.drawFromArray(StelPainter::Triangles, 36, 0, false, inds);
    //Done. Unbind shader
    glUseProgram(0);
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
}

void Scenery3d::renderFrustum(StelPainter &painter)
{
    debugShader->bind();

    for(int i=0; i<frustumSplits; i++)
    {
        Vec3f ntl = frustumArray[i].drawCorners[Frustum::NTL];
        Vec3f ntr = frustumArray[i].drawCorners[Frustum::NTR];
        Vec3f nbr = frustumArray[i].drawCorners[Frustum::NBR];
        Vec3f nbl = frustumArray[i].drawCorners[Frustum::NBL];
        Vec3f ftr = frustumArray[i].drawCorners[Frustum::FTR];
        Vec3f ftl = frustumArray[i].drawCorners[Frustum::FTL];
        Vec3f fbl = frustumArray[i].drawCorners[Frustum::FBL];
        Vec3f fbr = frustumArray[i].drawCorners[Frustum::FBR];

        glColor3f(1.0f, 0.0f, 0.0f);

        glBegin(GL_LINE_LOOP);
            //near plane
            glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
            glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
            glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
            glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
        glEnd();

        glBegin(GL_LINE_LOOP);
            //far plane
            glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
            glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
            glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
            glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
        glEnd();

        glBegin(GL_LINE_LOOP);
            //bottom plane
            glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
            glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
            glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
            glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
        glEnd();

        glBegin(GL_LINE_LOOP);
            //top plane
            glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
            glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
            glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
            glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
        glEnd();

        glBegin(GL_LINE_LOOP);
            //left plane
            glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
            glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
            glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
            glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
        glEnd();

        glBegin(GL_LINE_LOOP);
            // right plane
            glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
            glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
            glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
            glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
        glEnd();

        Vec3f a,b;
        glBegin(GL_LINES);
            // near
            a = (ntr + ntl + nbr + nbl) * 0.25;
            b = a + frustumArray[i].planes[Frustum::NEARP]->sNormal;
            glVertex3f(a.v[0],a.v[1],a.v[2]);
            glVertex3f(b.v[0],b.v[1],b.v[2]);

            // far
            a = (ftr + ftl + fbr + fbl) * 0.25;
            b = a + frustumArray[i].planes[Frustum::FARP]->sNormal;
            glVertex3f(a.v[0],a.v[1],a.v[2]);
            glVertex3f(b.v[0],b.v[1],b.v[2]);

            // left
            a = (ftl + fbl + nbl + ntl) * 0.25;
            b = a + frustumArray[i].planes[Frustum::LEFT]->sNormal;
            glVertex3f(a.v[0],a.v[1],a.v[2]);
            glVertex3f(b.v[0],b.v[1],b.v[2]);

            // right
            a = (ftr + nbr + fbr + ntr) * 0.25;
            b = a + frustumArray[i].planes[Frustum::RIGHT]->sNormal;
            glVertex3f(a.v[0],a.v[1],a.v[2]);
            glVertex3f(b.v[0],b.v[1],b.v[2]);

            // top
            a = (ftr + ftl + ntr + ntl) * 0.25;
            b = a + frustumArray[i].planes[Frustum::TOP]->sNormal;
            glVertex3f(a.v[0],a.v[1],a.v[2]);
            glVertex3f(b.v[0],b.v[1],b.v[2]);

            // bottom
            a = (fbr + fbl + nbr + nbl) * 0.25;
            b = a + frustumArray[i].planes[Frustum::BOTTOM]->sNormal;
            glVertex3f(a.v[0],a.v[1],a.v[2]);
            glVertex3f(b.v[0],b.v[1],b.v[2]);
        glEnd();
        //Done. Unbind shader
        debugShader->release();
    }
}

void Scenery3d::update(double deltaTime)
{
    if (core != NULL)
    {
        StelMovementMgr *stelMovementMgr = GETSTELMODULE(StelMovementMgr);

        Vec3d viewDirection = core->getMovementMgr()->getViewDirectionJ2000();
        Vec3d viewDirectionAltAz=core->j2000ToAltAz(viewDirection);
        double alt, az;
        StelUtils::rectToSphe(&az, &alt, viewDirectionAltAz);

        Vec3d move(( movement_x * std::cos(az) + movement_y * std::sin(az)),
                   ( movement_x * std::sin(az) - movement_y * std::cos(az)),
                   movement_z);

        move *= deltaTime * 0.01 * qMax(5.0, stelMovementMgr->getCurrentFov());

        //Bring move into world-grid space
        zRot2Grid.transfo(move);

        absolutePosition.v[0] += move.v[0];
        absolutePosition.v[1] += move.v[1];
	eye_height -= move.v[2];
	absolutePosition.v[2] = -groundHeight()-eye_height;

        //View Up in our case always pointing positive up
        viewUp.v[0] = 0;
        viewUp.v[1] = 0;
        viewUp.v[2] = 1;

        //View Direction
        viewDir = core->getMovementMgr()->getViewDirectionJ2000();
        viewDir = core->j2000ToAltAz(viewDir);
        //Bring viewDir into world-grid space
        zRot2Grid.transfo(viewDir);
        //Switch components as they aren't correct anymore
        Vec3d tmp = viewDir;
        viewDir.v[0] = tmp.v[1];
        viewDir.v[1] = -tmp.v[0];

        //View Position is already in world-grid space
        viewPos = -absolutePosition;

        //Calculate the Frustum for the current camera
        cFrust.calcFrustum(viewPos, viewDir, viewUp);
    }
}

float Scenery3d::groundHeight()
{
    if (heightmap == NULL) {
	return currentScene.groundNullHeight;
    } else {
        return heightmap->getHeight(-absolutePosition.v[0],-absolutePosition.v[1]);
    }
}

void Scenery3d::drawArrays(bool textures)
{
	//TODO FS: ditch fixed-function PL and client side state, replace with shader-based OpenGL and buffers!

    drawn = 0;

    const GLfloat zero[] = {0.0f, 0.0f, 0.0f, 1.0f};
    const GLfloat amb[] = {0.025f, 0.025f, 0.025f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb); // tiny overall background light

    bool tangEnabled = false;
    int tangLocation;

    for(int i=0; i<objModel->getNumberOfStelModels(); i++)
    {
        const OBJ::StelModel* pStelModel = &objModel->getStelModel(i);
        const OBJ::Material* pMaterial = pStelModel->pMaterial;

        if(textures)
        {
            sendToShader(pStelModel, curEffect, tangEnabled, tangLocation);
        }

        if(pMaterial->illum == OBJ::TRANSLUCENT)
        {
            glMaterialfv(GL_FRONT, GL_SPECULAR, zero);
            glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
            glEnable(GL_COLOR_MATERIAL);
            glColor4f(pMaterial->diffuse[0], pMaterial->diffuse[1], pMaterial->diffuse[2], pMaterial->alpha);
        }
        else
        {
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, pMaterial->diffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, zero);
            glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
        }

        if(pMaterial->illum == OBJ::DIFFUSE_AND_AMBIENT) // May make funny effects! [Note the reversed logic!]
        {
            glMaterialfv(GL_FRONT, GL_AMBIENT, pMaterial->ambient);
        }

        if(pMaterial->illum == OBJ::SPECULAR)
        {
            glMaterialfv(GL_FRONT, GL_AMBIENT, pMaterial->ambient);
            // GZ: This should enable specular color effects with colored and textured models.
            glMaterialfv(GL_FRONT, GL_SPECULAR, pMaterial->specular);
            glMaterialf(GL_FRONT, GL_SHININESS, pMaterial->shininess);
            glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR); // test how expensive this is.
            glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1); // Useful for Specular effects, change to 0 if too expensive
        }
        else
        {
            glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
            glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 0);
        }        

	int stride = objModel->getVertexSize();
	const OBJ::Vertex* vtxArray = objModel->getVertexArray();

        if(pStelModel->pMaterial->texture)
        {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, stride, &vtxArray->texCoord);
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, &vtxArray->position);
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, stride, &vtxArray->normal);

	glDrawElements(GL_TRIANGLES, pStelModel->triangleCount * 3, GL_UNSIGNED_INT, objModel->getIndexArray() + pStelModel->startIndex);

	if(pStelModel->pMaterial->texture)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

        if(pMaterial->illum == OBJ::TRANSLUCENT)
        {
            glDisable(GL_BLEND);
            glDisable(GL_COLOR_MATERIAL);
        }

        if(tangEnabled)
        {
            glDisableVertexAttribArray(tangLocation);
        }

    }

    //cFrust.drawFrustum();
    //renderFrustum(painter);
    //renderSceneAABB(painter);

//    for(unsigned int i=0; i<focusBodies.size() && textures; i++)
//    {
//        if(focusBodies[i]->getVertCount())
//        {
//            AABB bb;
//            for(unsigned int j=0; j<focusBodies[i]->getVertCount(); j++)
//                bb.expand(focusBodies[i]->getVerts()[j]);

//            bb.render();
//        }
//    }
}

void Scenery3d::sendToShader(const OBJ::StelModel* pStelModel, Effect cur, bool& tangEnabled, int& tangLocation)
{
    tangEnabled = false;

    if(cur != No)
    {
        curShader->setUniformValue("boolDebug", debugEnabled);
	curShader->setUniformValue("fTransparencyThresh", currentScene.transparencyThreshold);
        curShader->setUniformValue("alpha", pStelModel->pMaterial->alpha);
        curShader->setUniformValue("boolFilterHQ", filterHQ);
        curShader->setUniformValue("boolFilterShadows", filterShadowsEnabled);
        curShader->setUniformValue("boolVenus", venusOn);

        int iIllum = pStelModel->pMaterial->illum;
        if(iIllum < 0 || iIllum > 2)
        {
            if(iIllum != 9)
            {
                //Map to default
                iIllum = 0;
                qWarning() << "[Scenery3D] Illumination model was invalid. Forced to Illumination model 0.";
            }
        }

        curShader->setUniformValue("iIllum", iIllum);

        if (pStelModel->pMaterial->texture)
        {
            glActiveTexture(GL_TEXTURE0);
            pStelModel->pMaterial->texture.data()->bind();

            //Send texture to shader
            curShader->setUniformValue("tex", 0);
            //Indicate that we are in texture Mode
            curShader->setUniformValue("onlyColor", false);
        }
        else
        {
            //No texture, send color and indication
            curShader->setUniformValue("vecColor", pStelModel->pMaterial->diffuse[0], pStelModel->pMaterial->diffuse[1], pStelModel->pMaterial->diffuse[2], pStelModel->pMaterial->diffuse[3]);
            curShader->setUniformValue("onlyColor", true);
        }

        //Bump Mapping
        if(cur == BumpMapping || cur == All)
        {
            //Send tangents to shader
            if(objModel->hasTangents())
            {
                tangLocation = curShader->attributeLocation("vecTangent");
                glEnableVertexAttribArray(tangLocation);
                glVertexAttribPointer(tangLocation, 4, GL_FLOAT, 0, objModel->getVertexSize(), objModel->getVertexArray()->tangent);
                tangEnabled = true;
            }

            curShader->setUniformValue("s", parallaxScale);

            if (pStelModel->pMaterial->bump_texture)
            {
                glActiveTexture(GL_TEXTURE1);
                pStelModel->pMaterial->bump_texture.data()-> bind();

                //Send bump map to shader
                curShader->setUniformValue("bmap", 1);
                //Flag for bumped lighting
                curShader->setUniformValue("boolBump", true);
            }
            else
            {
                curShader->setUniformValue("boolBump", false);
            }

            if (pStelModel->pMaterial->height_texture)
            {
                glActiveTexture(GL_TEXTURE2);
                pStelModel->pMaterial->height_texture.data()-> bind();

                //Send bump map to shader
                curShader->setUniformValue("hmap", 2);

                //Flag for parallax mapping
                curShader->setUniformValue("boolHeight", true);
            }
            else
            {
                curShader->setUniformValue("boolHeight", false);
            }
        }
    }
    else // No-shader code, more classical OpenGL pipeline
    {
        //TODO FS: remove
        glActiveTexture(GL_TEXTURE0);
        if (pStelModel->pMaterial->texture)
        {
            glActiveTexture(GL_TEXTURE0);
            pStelModel->pMaterial->texture.data()->bind();
        }
    }
}

void Scenery3d::generateCubeMap_drawScene()
{
    //Bind shader based on selected effect flags
    bindShader();

    drawArrays(true);

    //Unbind
    glUseProgram(0);
}

void Scenery3d::bindShader()
{
    //No Shader, No effect
    curEffect = No;
    curShader = 0;

    if(shadowsEnabled && !bumpsEnabled){ curShader = shadowShader; curEffect = ShadowMapping; }
    else if(!shadowsEnabled && bumpsEnabled){ curShader = bumpShader; curEffect = BumpMapping; }
    else if(shadowsEnabled && bumpsEnabled){ curShader = univShader; curEffect = All; }

    //Bind the selected shader
    if(curShader != 0) curShader->bind();
}

void Scenery3d::generateCubeMap_drawSceneWithShadows()
{    
    //Bind the shader
    bindShader();

    int location;

    //Calculate texture matrix for projection
    //This matrix takes us from eye space to the light's clip space
    //It is postmultiplied by the inverse of the current view matrix when specifying texgen
    static Mat4f biasMatrix(0.5f, 0.0f, 0.0f, 0.0f,
                            0.0f, 0.5f, 0.0f, 0.0f,
                            0.0f, 0.0f, 0.5f, 0.0f,
                            0.5f, 0.5f, 0.5f, 1.0f);	//bias from [-1, 1] to [0, 1]

    //Holds the squared frustum splits necessary for the lookup in the shader
    Vec4f squaredSplits = Vec4f(0.0f);
    for(int i=0; i<frustumSplits; i++)
    {
        squaredSplits.v[i] = frustumArray[i].zFar * frustumArray[i].zFar;

        //Bind current depth map texture
        glActiveTexture(GL_TEXTURE3+i);
        glBindTexture(GL_TEXTURE_2D, shadowMapsArray[i]);

        //Compute texture matrix
        Mat4f texMat = biasMatrix * shadowCPM[i];

        //Send to shader
        QString smapLoc = "smap_"+ QString::number(i);
        curShader->setUniformValue(smapLoc.toLatin1().constData(), 3+i);

        QString texMatLoc = "texmat_"+ QString::number(i);
        curShader->setUniformValue(texMatLoc.toLatin1().constData(), QMatrix4x4(texMat.r));
    }

    //Send squared splits to the shader
    curShader->setUniformValue("vecSplits", squaredSplits.v[0], squaredSplits.v[1], squaredSplits.v[2], squaredSplits.v[3]);

    //Activate normal texturing
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    //Draw
    drawArrays(true);

    //Done. Unbind shader
    glUseProgram(0);
}

void Scenery3d::computeZDist(float zNear, float zFar)
{
    float ratio = zFar/zNear;

    //Compute the z-planes for the subfrusta
    frustumArray[0].zNear = zNear;

    for(int i=1; i<frustumSplits; i++)
    {
        float s_i = i/static_cast<float>(frustumSplits);

        frustumArray[i].zNear = splitWeight*(zNear*powf(ratio, s_i)) + (1.0f-splitWeight)*(zNear + (zFar - zNear)*s_i);
        //Set the previous zFar to the newly computed zNear
        frustumArray[i-1].zFar = frustumArray[i].zNear * 1.005f;
    }

    //Make sure the last zFar is set correctly
    frustumArray[frustumSplits-1].zFar = zFar;
}

void Scenery3d::computePolyhedron(int splitIndex)
{
    //Building a convex body for directional lights according to Wimmer et al. 2006
    Polyhedron &body = *focusBodies[splitIndex];

    //Add the Frustum to begin with
    body.add(frustumArray[splitIndex]);
    //Intersect with the scene AABB
    body.intersect(sceneBoundingBox);
    //Extrude towards light direction
    body.extrude(lightDir, sceneBoundingBox);
}

void Scenery3d::computeOrthoProjVals()
{
    //Focus the light first on the entire scene
    float maxZ = 0.0f;
    float minZ = std::numeric_limits<float>::max();
    dim = 0.0f;

    Vec3f eye = lightDir;
    Vec3f vDir = -eye;
    vDir.normalize();
    Vec3f up = Vec3f(0.0f, 0.0f, 1.0f);
    Vec3f down = -up;
    Vec3f left = vDir^up;
    left.normalize();
    Vec3f right = -left;

    for(unsigned int i=0; i<AABB::CORNERCOUNT; i++)
    {
        Vec3f v = sceneBoundingBox.getCorner(static_cast<AABB::Corner>(i));
        Vec3f toCam = v - eye;

        float dist = toCam.dot(vDir);
        maxZ = std::max(dist, maxZ);
        minZ = std::min(dist, minZ);

        dim = std::max(std::abs(toCam.dot(left)), dim);
        dim = std::max(std::abs(toCam.dot(right)), dim);
        dim = std::max(std::abs(toCam.dot(up)), dim);
        dim = std::max(std::abs(toCam.dot(down)), dim);
    }

    //Make sure planes arent too small
    dimNear = std::max(minZ, 1.0f);
    dimFar = std::max(maxZ, dimNear + 1.0f);
}

void Scenery3d::computeCropMatrix(int frustumIndex)
{
    //Compute the light frustum, only need to do this once because we're just cropping it afterwards
    if(frustumIndex == 0)
    {
        computeOrthoProjVals();
    }

    //Calculating a fitting Projection Matrix for the light
    Mat4f lightProj, lightMVP, c;

    //Setup the Ortho Projection based on found z values
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-dim, dim, -dim, dim, dimNear, dimFar);
    //Save it for later use
    glGetFloatv(GL_PROJECTION_MATRIX, lightProj);
    glPushMatrix();
    glMultMatrixf(lightViewMatrix);
    //Save the light's ModelViewProjection Matrix for later use
    glGetFloatv(GL_PROJECTION_MATRIX, lightMVP);
    glPopMatrix();

    float maxX = -std::numeric_limits<float>::max();
    float maxY = maxX;
    float maxZ = maxX;
    float minX = std::numeric_limits<float>::max();
    float minY = minX;
    float minZ = minX;

    //! Uncomment this and the other marked lines to get no artifacts (but way worse shadow quality
    //! making the second split pretty much useless
    AABB bb;
    for(unsigned int i=0; i<focusBodies[frustumIndex]->getVertCount(); i++)
        bb.expand(focusBodies[frustumIndex]->getVerts()[i]);

    //Project the frustum into light space and find the boundaries
    for(unsigned int i=0; i<focusBodies[frustumIndex]->getVertCount(); i++)
    //for(unsigned int i=0; i<AABB::CORNERCOUNT; i++)
    {
        Vec3f tmp = focusBodies[frustumIndex]->getVerts()[i];
        //Vec3f tmp = bb.getCorner(static_cast<AABB::Corner>(i));
        Vec4f transf = lightMVP*Vec4f(tmp.v[0], tmp.v[1], tmp.v[2], 1.0f);

        transf.v[0] /= transf.v[3];
        transf.v[1] /= transf.v[3];
        transf.v[2] /= transf.v[3];

        if(transf.v[0] > maxX) maxX = transf.v[0];
        if(transf.v[0] < minX) minX = transf.v[0];
        if(transf.v[1] > maxY) maxY = transf.v[1];
        if(transf.v[1] < minY) minY = transf.v[1];
        if(transf.v[2] > maxZ) maxZ = transf.v[2];
        if(transf.v[2] < minZ) minZ = transf.v[2];
    }

    //To avoid artifacts and far plane clipping
    maxZ = 1.0f;

    //Build the crop matrix and apply it to the light projection matrix
    float scaleX = 2.0f/(maxX - minX);
    float scaleY = 2.0f/(maxY - minY);
    float scaleZ = 1.0f/(maxZ - minZ);

    float offsetZ = -minZ*scaleZ;

    //Reducing swimming as specified in Practical cascaded shadow maps by Zhang et al.
    float quantizer = 64.0f;
    scaleX = 1.0f/std::ceil(1.0f/scaleX*quantizer) * quantizer;
    scaleY = 1.0f/std::ceil(1.0f/scaleY*quantizer) * quantizer;

    float offsetX = -0.5f*(maxX + minX)*scaleX;
    float offsetY = -0.5f*(maxY + minY)*scaleY;

    float halfTex = 0.5f*shadowmapSize;
    offsetX = std::ceil(offsetX*halfTex)/halfTex;
    offsetY = std::ceil(offsetY*halfTex)/halfTex;

    //Making the crop matrix
    c = Mat4f(scaleX, 0.0f,   0.0f, offsetX,
              0.0f,   scaleY, 0.0f, offsetY,
              0.0f,   0.0f,   scaleZ, offsetZ,
              0.0f,   0.0f,   0.0f, 1.0f);

    c = c.transpose();

    //Crop the light projection matrix
    glLoadMatrixf(c);
    glMultMatrixf(lightProj);
}

void Scenery3d::adjustFrustum()
{
    //Create the cam frustum because it might have changed from before
    Frustum camFrust;
    camFrust.setCamInternals(camFOV, camAspect, camNear, camFar);
    camFrust.calcFrustum(viewPos, viewDir, viewUp);

    //Compute H = V intersect S according to Zhang et al.
    Polyhedron p;
    p.add(camFrust);
    p.intersect(sceneBoundingBox);
    p.makeUniqueVerts();

    //Find the boundaries
    float maxZ = -std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();

    Vec3f eye = vecdToFloat(viewPos);

    Vec3f vDir = vecdToFloat(viewDir);
    vDir.normalize();

    const std::vector<Vec3f> &verts = p.getVerts();
    for(unsigned int i=0; i<p.getVertCount(); i++)
    {
        //Find the distance to the camera
        Vec3f v = verts[i];
        Vec3f toCam = v - eye;
        float dist = toCam.dot(vDir);

        maxZ = std::max(dist, maxZ);
        minZ = std::min(dist, minZ);
    }

    //Setup the newly found near and far planes but make sure they're not too small
    camNear = std::max(minZ, 1.0f);
    camFar = std::max(maxZ, camNear+1.0f);

    //Clear the previous bodies
    focusBodies.clear();

    //Setup the subfrusta
    for(int i=0; i<frustumSplits; i++)
    {
        frustumArray[i].setCamInternals(camFOV, camAspect, camNear, camFar);
        focusBodies.push_back(new Polyhedron());
        focusBodies[i]->clear();
    }
}

void Scenery3d::analyzeViewSamples(StelPainter &painter)
{
    float f = 2.0 / tan(camFOV * M_PI / 360.0);
    Mat4f camProj = Mat4f(f / camAspect, 0, 0, 0,
                    0, f, 0, 0,
                    0, 0, (camFar + camNear) / (camNear - camFar), 2.0 * camFar * camNear / (camNear - camFar),
                    0, 0, -1, 0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glColorMask(0, 0, 0, 0); // disable color writes (increase performance?)

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMultMatrixf(camProj);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Vec3d l = viewPos + viewDir;
    nogluLookAt(viewPos.v[0], viewPos.v[1], viewPos.v[2],
            l.v[0],l.v[1],l.v[2],
            viewUp.v[0],viewUp.v[1],viewUp.v[2]);

    //Bind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, camDepthFBO);

    //No shader needed for generating the depth map
    glUseProgram(0);

    //Set viewport
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, shadowmapSize, shadowmapSize);

    //Activate texture unit as usual
    glActiveTexture(GL_TEXTURE7);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, camDepthTex, 0);

    //Clear everything
    glClear(GL_DEPTH_BUFFER_BIT);

    //Draw the scene
    drawArrays(false);

    //Switch to the minMax reduction shader
    minMaxShader->bind();

    //Send the depth map as base level to the min/max shader
    curShader->setUniformValue("tex", 7);

    //Calculate dimensions of the next level
    int outDim = shadowmapSize/2;

    //Set the first colorattachment as render target
    glDrawBuffer(attachments[writeTex]);

    //Draw a fullscreen quad to trigger the calculation
    drawFullscreenQuad(outDim);

    //Calculate remaining passes
    unsigned int steps = std::floor(std::log(outDim+0.5)/std::log(2.0));

    //Render remaining passes
    for(unsigned int i=0; i<steps; i++)
    {
        //Set viewport
        glPushAttrib(GL_VIEWPORT_BIT);
        glViewport(0, 0, outDim, outDim);

        //Send the previously generated texture as current level to the shader
        glActiveTexture(GL_TEXTURE8+readTex);;
        curShader->setUniformValue("tex", 8+readTex);

        //Output will be written in writeTex
        glDrawBuffer(attachments[writeTex]);
        outDim /= 2;
        drawFullscreenQuad(outDim);
    }

    //Done
    glUseProgram(0);

    //Reset Viewportbit
    glPopAttrib();

    //Unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    //Switch back to normal texturing
    glActiveTexture(GL_TEXTURE0);

    //Reset
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glColorMask(1, 1, 1, 1);
}

void Scenery3d::drawFullscreenQuad(int dim)
{
    float fDim = static_cast<float>(dim);
    glBegin(GL_QUADS);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(fDim, 0.0f);
        glVertex2f(fDim, fDim);
        glVertex2f(0.0, fDim);
    glEnd();

    //Swap around textures
    if(writeTex == 0)
    {
        writeTex = 1;
        readTex = 0;
    }
    else
    {
        writeTex = 0;
        readTex = 1;
    }
}

void Scenery3d::generateShadowMap(StelCore* core)
{
    //Needed so we can actually adjust the frustum upwards after it was already adjusted downwards.
    //This resets it and they'll be recomputed in adjustFrustum();
    camNear = NEARZ;
    camFar = FARZ;

    //Nothing to draw
    if(!hasModels)
        return;

    //Adjust the frustum to the scene before analyzing the view samples
    adjustFrustum();
    //Further adjust
    //analyzeViewSamples(painter);

    //Determine sun position
    SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
    sunPosition = ssystem->getSun()->getAltAzPosAuto(core);
    //zRotateMatrix.transfo(sunPosition); // GZ: These rotations were commented out - testing 20120122->correct!
    sunPosition.normalize();
    // GZ: at night, a near-full Moon can cast good shadows.
    Vec3d moonPosition = ssystem->getMoon()->getAltAzPosAuto(core);
    //zRotateMatrix.transfo(moonPosition);
    moonPosition.normalize();
    Vec3d venusPosition = ssystem->searchByName("Venus")->getAltAzPosAuto(core);
    //zRotateMatrix.transfo(venusPosition);
    venusPosition.normalize();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    //Backface culling for ESM!
    glCullFace(GL_FRONT);
    glColorMask(0, 0, 0, 0); // disable color writes (increase performance?)

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glMatrixMode(GL_MODELVIEW);
    //glPushMatrix();
    //glLoadIdentity();
    //glRotated(90.0f, -1.0f, 0.0f, 0.0f);

    //front
    glPushMatrix();
    glLoadIdentity();

    //Select view position based on which planet is visible
    if (sunPosition[2]>0)
    {
        nogluLookAt (sunPosition[0], sunPosition[1], sunPosition[2], 0, 0, 0, 0, 0, 1);
        lightDir = Vec3f(sunPosition[0], sunPosition[1], sunPosition[2]);
        lightDir.normalize();
        venusOn = false;
    }
    else if (moonPosition[2]>0)
    {
        nogluLookAt (moonPosition[0], moonPosition[1], moonPosition[2], 0, 0, 0, 0, 0, 1);
        lightDir = Vec3f(moonPosition[0], moonPosition[1], moonPosition[2]);
        lightDir.normalize();
        venusOn = false;
    }
    else
    {
        nogluLookAt(venusPosition[0], venusPosition[1], venusPosition[2], 0, 0, 0, 0, 0, 1);
        lightDir = Vec3f(venusPosition[0], venusPosition[1], venusPosition[2]);
        lightDir.normalize();
        venusOn = true;
    }

    /* eyeX,eyeY,eyeZ,centerX,centerY,centerZ,upX,upY,upZ)*/
    glGetFloatv(GL_MODELVIEW_MATRIX, lightViewMatrix); // save light view for further render passes

    //Bind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

    //No shader needed for generating the depth map
    glUseProgram(0);

    //Fix selfshadowing
    //glEnable(GL_POLYGON_OFFSET_FILL);
    //glPolygonOffset(1.1f,4.0f);
    //! This is now done in shader via a bias. If that's ever a problem uncomment this part and the disabling farther down and change
    //! glCullFace(GL_FRONT) to GL_BACK (farther up from here)

    //Set viewport
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, shadowmapSize, shadowmapSize);

    //Compute and set z-distances for each split
    computeZDist(camNear, camFar);

    //For each split
    for(int i=0; i<frustumSplits; i++)
    {
        //Calculate the Frustum for this split
        frustumArray[i].calcFrustum(viewPos, viewDir, viewUp);

        //Find the convex body that encompasses all shadow receivers and casters for this split
        focusBodies[i]->clear();
        computePolyhedron(i);

        //qDebug() << i << ".split vert count:" << focusBodies[i]->getVertCount();

        if(focusBodies[i]->getVertCount())
        {
            //Calculate the crop matrix so that the light's frustum is tightly fit to the current split's PSR+PSC polyhedron
            //This alters the ProjectionMatrix of the light
            computeCropMatrix(i);

            //Activate texture unit as usual
            glActiveTexture(GL_TEXTURE3+i);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapsArray[i], 0);

            //Clear everything
            glClear(GL_DEPTH_BUFFER_BIT);

            //Draw the scene
	    drawArrays(false);

            //Store the new projection * lightView for later
            glMatrixMode(GL_PROJECTION);
            glMultMatrixf(lightViewMatrix);
            glGetFloatv(GL_PROJECTION_MATRIX, shadowCPM[i]);
        }
    }

    //Reset Viewportbit
    glPopAttrib();

    //Move polygons back to normal position
    //glDisable(GL_POLYGON_OFFSET_FILL);

    //Unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //Switch back to normal texturing
    glActiveTexture(GL_TEXTURE0);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    //glPopMatrix();

    //Reset
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    glColorMask(1, 1, 1, 1);
}

Scenery3d::ShadowCaster  Scenery3d::setupLights(float &ambientBrightness, float &directionalBrightness, Vec3f &lightsourcePosition)
{
    SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
    Vec3d sunPosition = ssystem->getSun()->getAltAzPosAuto(core);
    //zRotateMatrix.transfo(sunPosition); //: GZ: VERIFIED THE NECESSITY OF THIS. STOP: MAYBE ONLY FOR NON-ROTATED NORMALS.(20120219)
    sunPosition.normalize();
    Vec3d moonPosition = ssystem->getMoon()->getAltAzPosAuto(core);
    float moonPhaseAngle=ssystem->getMoon()->getPhase(core->getObserverHeliocentricEclipticPos());
    //zRotateMatrix.transfo(moonPosition);
    moonPosition.normalize();
    PlanetP venus=ssystem->searchByEnglishName("Venus");
    Vec3d venusPosition = venus->getAltAzPosAuto(core);
    float venusPhaseAngle=venus->getPhase(core->getObserverHeliocentricEclipticPos());
    //zRotateMatrix.transfo(venusPosition);
    venusPosition.normalize();

    // The light model here: ambient light consists of solar twilight and day ambient,
    // plus lunar ambient, plus a base constant AMBIENT_BRIGHTNESS_FACTOR[0.1?],
    // plus an artificial "torch" that can be toggled via Ctrl-L[ight].
    // We define the ambient solar brightness zero when the sun is 18 degrees below the horizon, and lift the sun by 18 deg.
    // ambient brightness component of the sun is then  MIN(0.3, sin(sun)+0.3)
    // With the sun above the horizon, we raise only the directional component.
    // ambient brightness component of the moon is sqrt(sin(alt_moon)*(cos(moon.phase_angle)+1)/2)*LUNAR_BRIGHTNESS_FACTOR[0.2?]
    // Directional brightness factor: sqrt(sin(alt_sun)) if sin(alt_sun)>0 --> NO: MIN(0.7, sin(sun)+0.1), i.e. sun 6 degrees higher.
    //                                sqrt(sin(alt_moon)*(cos(moon.phase_angle)+1)/2)*LUNAR_BRIGHTNESS_FACTOR if sin(alt_moon)>0
    //                                sqrt(sin(alt_venus)*(cos(venus.phase_angle)+1)/2)*VENUS_BRIGHTNESS_FACTOR[0.15?]
    // Note the sqrt(sin(alt))-terms: they are to increase brightness sooner than with the Lambert law.
    //float sinSunAngleRad = sin(qMin(M_PI_2, asin(sunPosition[2])+8.*M_PI/180.));
    //float sinMoonAngleRad = moonPosition[2];

    float sinSunAngle  = sunPosition[2];
    float sinMoonAngle = moonPosition[2];
    float sinVenusAngle = venusPosition[2];
    ambientBrightness=AMBIENT_BRIGHTNESS_FACTOR+(torchEnabled? torchBrightness : 0);
    directionalBrightness=0.0f;
    ShadowCaster shadowcaster = None;
    // DEBUG AIDS: Helper strings to be displayed
    QString sunAmbientString;
    QString moonAmbientString;
    QString backgroundAmbientString=QString("%1").arg(ambientBrightness, 6, 'f', 4);
    QString directionalSourceString;

    if(sinSunAngle > -0.3f) // sun above -18 deg?
    {
        ambientBrightness += qMin(0.3, sinSunAngle+0.3);
        sunAmbientString=QString("%1").arg(qMin(0.3, sinSunAngle+0.3), 6, 'f', 4);
    }
    else
        sunAmbientString=QString("0.0");

    if (sinMoonAngle>0.0f)
    {
        ambientBrightness += sqrt(sinMoonAngle * ((std::cos(moonPhaseAngle)+1)/2)) * LUNAR_BRIGHTNESS_FACTOR;
        moonAmbientString=QString("%1").arg(sqrt(sinMoonAngle * ((std::cos(moonPhaseAngle)+1)/2)) * LUNAR_BRIGHTNESS_FACTOR);
    }
    else
        moonAmbientString=QString("0.0");
    // Now find shadow caster, if any:
    if (sinSunAngle>0.0f)
    {
        directionalBrightness=qMin(0.7, sqrt(sinSunAngle+0.1)); // limit to 0.7 in order to keep total below 1.
        lightsourcePosition.set(sunPosition.v[0], sunPosition.v[1], sunPosition.v[2]);
        if (shadowsEnabled) shadowcaster = Sun;
        directionalSourceString="Sun";
    }
 /*   else if (sinSunAngle> -0.3f) // sun above -18: create shadowless directional pseudo-light from solar azimuth
    {
        directionalBrightness=qMin(0.7, sinSunAngle+0.3); // limit to 0.7 in order to keep total below 1.
        lightsourcePosition.set(sunPosition.v[0], sunPosition.v[1], sinSunAngle+0.3);
        directionalSourceString="(Sun, below hor.)";
    }*/
    else if (sinMoonAngle>0.0f)
    {
        directionalBrightness= sqrt(sinMoonAngle) * ((std::cos(moonPhaseAngle)+1)/2) * LUNAR_BRIGHTNESS_FACTOR;
        directionalBrightness -= (ambientBrightness-0.05)/2.0f;
        directionalBrightness = qMax(0.0f, directionalBrightness);
        if (directionalBrightness > 0)
        {
            lightsourcePosition.set(moonPosition.v[0], moonPosition.v[1], moonPosition.v[2]);
            if (shadowsEnabled) shadowcaster = Moon;
            directionalSourceString="Moon";
        } else directionalSourceString="Moon";
        //Alternately, construct a term around lunar brightness, like
        // directionalBrightness=(mag/-10)
    }
    else if (sinVenusAngle>0.0f)
    {
        directionalBrightness=sqrt(sinVenusAngle)*((std::cos(venusPhaseAngle)+1)/2) * VENUS_BRIGHTNESS_FACTOR;
        directionalBrightness -= (ambientBrightness-0.05)/2.0f;
        directionalBrightness = qMax(0.0f, directionalBrightness);
        if (directionalBrightness > 0)
        {
            lightsourcePosition.set(venusPosition.v[0], venusPosition.v[1], venusPosition.v[2]);
            if (shadowsEnabled) shadowcaster = Venus;
            directionalSourceString="Venus";
        } else directionalSourceString="(Venus, flooded by ambient)";
        //Alternately, construct a term around Venus brightness, like
        // directionalBrightness=(mag/-100)
    }
    else
    {   //GZ: this should not matter here, just to make OpenGL happy.
        lightsourcePosition.set(sunPosition.v[0], sunPosition.v[1], sunPosition.v[2]);
        directionalSourceString="(Sun, below horiz.)";
    }

    // correct light mixture. Directional is good to increase for sunrise/sunset shadow casting.
    if (shadowcaster)
    {
        ambientBrightness-=(torchEnabled? torchBrightness*0.8 : 0);
        directionalBrightness+=(torchEnabled? torchBrightness*0.8 : 0);
    }

    // DEBUG: Prepare output message
    QString shadowCasterName;
    switch (shadowcaster) {
        case None:  shadowCasterName="None";  break;
        case Sun:   shadowCasterName="Sun";   break;
        case Moon:  shadowCasterName="Moon";  break;
        case Venus: shadowCasterName="Venus"; break;
        default: shadowCasterName="Error!!!";
    }
    lightMessage=QString("Ambient: %1 Directional: %2. Shadows cast by: %3 from %4/%5/%6")
                 .arg(ambientBrightness, 6, 'f', 4).arg(directionalBrightness, 6, 'f', 4)
                 .arg(shadowCasterName).arg(lightsourcePosition.v[0], 6, 'f', 4)
                 .arg(lightsourcePosition.v[1], 6, 'f', 4).arg(lightsourcePosition.v[2], 6, 'f', 4);
    lightMessage2=QString("Contributions: Ambient     Sun: %1, Moon: %2, Background+^L: %3").arg(sunAmbientString).arg(moonAmbientString).arg(backgroundAmbientString);
    lightMessage3=QString("               Directional %1 by: %2 ").arg(directionalBrightness, 6, 'f', 4).arg(directionalSourceString);

    return shadowcaster;
}

void Scenery3d::generateCubeMap()
{
    if(!hasModels)
        return;

    for (int i=0; i<6; i++) {
        if (cubeMap[i] == NULL) {
            cubeMap[i] = new QOpenGLFramebufferObject(cubemapSize, cubemapSize, QOpenGLFramebufferObject::Depth);
            if (cubeMap[i]->attachment() != QOpenGLFramebufferObject::Depth){
                qWarning()<< "Scenery3d: Framebuffer failed to aquire depth buffer. Try smaller cubemap_size!";
            }
            glBindTexture(GL_TEXTURE_2D, cubeMap[i]->texture());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
    }

    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glShadeModel(GL_SMOOTH);


    float ambientBrightness, directionalBrightness;
    Vec3f lightsourcePosition;
    ShadowCaster shadows=setupLights(ambientBrightness, directionalBrightness, lightsourcePosition);
    // GZ: These signs were suspiciously -/-/+ before.
    const GLfloat LightPosition[]= {lightsourcePosition.v[0], lightsourcePosition.v[1], lightsourcePosition.v[2], 0.0f} ;// signs determined by experiment

    float fov = 90.0f;
    float aspect = 1.0f;
    float zNear = 1.0f;
    float zFar = 10000.0f;
    float f = 2.0 / tan(fov * M_PI / 360.0);
    Mat4f camProj = Mat4f(f / aspect, 0, 0, 0,
                    0, f, 0, 0,
                    0, 0, (zFar + zNear) / (zNear - zFar), 2.0 * zFar * zNear / (zNear - zFar),
                    0, 0, -1, 0);

    cFrust.setCamInternals(fov, aspect, 1.0, 10000.0);

    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, cubemapSize, cubemapSize);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMultMatrixf(camProj);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glRotated(90.0f, -1.0f, 0.0f, 0.0f);

    #define DRAW_SCENE  glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);\
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);\
                        setLights(ambientBrightness, directionalBrightness);\
			if(shadows){generateCubeMap_drawSceneWithShadows();}\
			else{generateCubeMap_drawScene();}\

    //front
    glPushMatrix();
    glTranslated(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
    cubeMap[0]->bind();
    DRAW_SCENE
    cubeMap[0]->release();
    glPopMatrix();

    //right
    glPushMatrix();
    glRotated(90.0f, 0.0f, 0.0f, 1.0f);
    glTranslated(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
    cubeMap[1]->bind();
    DRAW_SCENE
    cubeMap[1]->release();
    glPopMatrix();

    //left
    glPushMatrix();
    glRotated(90.0f, 0.0f, 0.0f, -1.0f);
    glTranslated(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
    cubeMap[2]->bind();
    DRAW_SCENE
    cubeMap[2]->release();
    glPopMatrix();

    //back
    glPushMatrix();
    glRotated(180.0f, 0.0f, 0.0f, 1.0f);
    glTranslated(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
    cubeMap[3]->bind();
    DRAW_SCENE
    cubeMap[3]->release();
    glPopMatrix();

    //top
    glPushMatrix();
    glRotated(90.0f, 1.0f, 0.0f, 0.0f);
    glTranslated(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
    cubeMap[4]->bind();
    DRAW_SCENE
    cubeMap[4]->release();
    glPopMatrix();

    //bottom
    glPushMatrix();
    glRotated(90.0f, -1.0f, 0.0f, 0.0f);
    glTranslated(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
    cubeMap[5]->bind();
    DRAW_SCENE
    cubeMap[5]->release();
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();

    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

void Scenery3d::drawFromCubeMap(StelCore* core)
{
    if(!hasModels)
	return;

    const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
    StelPainter painter(prj);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //depth test and culling is necessary for correct display,
    //because the cube faces can be projected in quite "weird" ways
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    glClear(GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);

    //painter's color is multiplied with the texture
    painter.setColor(1.0f,1.0f,1.0f,1.0f);

    //front
    glBindTexture(GL_TEXTURE_2D, cubeMap[0]->texture());
    painter.drawSphericalTriangles(cubePlaneFront, true, false, NULL, false);

    //right
    glBindTexture(GL_TEXTURE_2D, cubeMap[1]->texture());
    painter.drawSphericalTriangles(cubePlaneRight, true, false, NULL, false);

    //left
    glBindTexture(GL_TEXTURE_2D, cubeMap[2]->texture());
    painter.drawSphericalTriangles(cubePlaneLeft, true, false, NULL, false);

    //back
    glBindTexture(GL_TEXTURE_2D, cubeMap[3]->texture());
    painter.drawSphericalTriangles(cubePlaneBack, true, false, NULL, false);

    //top
    glBindTexture(GL_TEXTURE_2D, cubeMap[4]->texture());
    painter.drawSphericalTriangles(cubePlaneTop, true, false, NULL, false);

    //bottom
    glBindTexture(GL_TEXTURE_2D, cubeMap[5]->texture());
    painter.drawSphericalTriangles(cubePlaneBottom, true, false, NULL, false);

    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    //glDisable(GL_BLEND);
}

void Scenery3d::drawObjModel(StelCore* core) // for Perspective Projection only!
{
    if(!hasModels)
        return;

    const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);

    //TODO FS remove matrix stack, replace with manual MV and P matrices

    //glEnable(GL_MULTISAMPLE); // enabling multisampling aka Anti-Aliasing
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glShadeModel(GL_SMOOTH);

    float ambientBrightness, directionalBrightness; // was: lightBrightness;
    Vec3f lightsourcePosition;
    ShadowCaster shadows=setupLights(ambientBrightness, directionalBrightness, lightsourcePosition);

    // GZ: These were -/-/+ before!
    const GLfloat LightPosition[]= {lightsourcePosition.v[0], lightsourcePosition.v[1], lightsourcePosition.v[2], 0.0f} ;// signs determined by experiment

    glClear(GL_DEPTH_BUFFER_BIT);

    float fov = prj->getFov();
    float aspect = (float)prj->getViewportWidth() / (float)prj->getViewportHeight();
    float zNear = 1.0f;
    float zFar = 10000.0f;
    float f = 2.0 / tan(fov * M_PI / 360.0);
    Mat4d projMatd(f / aspect, 0, 0, 0,
                   0, f, 0, 0,
                   0, 0, (zFar + zNear) / (zNear - zFar), 2.0 * zFar * zNear / (zNear - zFar),
                   0, 0, -1, 0);

    StelProjector::StelProjectorParams projectorParams = core->getCurrentStelProjectorParams();
    cFrust.setCamInternals(projectorParams.fov, aspect, projectorParams.zNear, 30);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMultMatrixd(projMatd);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    //glMultMatrixd(prj->getModelViewMatrix());
    glMultMatrixd(prj->getModelViewTransform()->getApproximateLinearTransfo());
    glTranslated(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);

    glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);

    setLights(ambientBrightness, directionalBrightness);

    //testMethod();
    //GZ: The following calls apparently require the full cubemap. TODO: Verify and simplify
    if (shadows) {
	generateCubeMap_drawSceneWithShadows();
    } else {
	generateCubeMap_drawScene();
    }

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    //glDisable(GL_BLEND);
}

void Scenery3d::drawCoordinatesText(StelCore* core)
{
    if(!hasModels)
        return;

    const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
    StelPainter painter(prj);
    painter.setFont(debugTextFont);
    painter.setColor(1.0f,0.0f,1.0f);
    float screen_x = prj->getViewportWidth()  - 240.0f;
    float screen_y = prj->getViewportHeight() -  60.0f;
    QString str;

    // model_pos is the observer position (camera eye position) in model-grid coordinates
    Vec3d model_pos=currentScene.zRotateMatrix*Vec3d(-absolutePosition.v[0], absolutePosition.v[1], -absolutePosition.v[2]);
    model_pos[1] *= -1.0;

    // world_pos is the observer position (camera eye position) in grid coordinates, e.g. Gauss-Krueger or UTM.
    Vec3d world_pos= model_pos + currentScene.modelWorldOffset;
    // problem: long grid names!
    painter.drawText(prj->getViewportWidth()-10-qMax(240, painter.getFontMetrics().boundingRect(currentScene.gridName).width()),
		     screen_y, currentScene.gridName);
    screen_y -= 17.0f;
    str = QString("East:   %1m").arg(world_pos[0], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = QString("North:  %1m").arg(world_pos[1], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = QString("Height: %1m").arg(world_pos[2]-eye_height, 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = QString("Eye:    %1m").arg(eye_height, 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);

    /*// DEBUG AIDS:
    screen_y -= 15.0f;
    str = QString("model_X:%1m").arg(model_pos[0], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
    str = QString("model_Y:%1m").arg(model_pos[1], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
    str = QString("model_Z:%1m").arg(model_pos[2], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
    str = QString("abs_X:  %1m").arg(absolutePosition.v[0], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
    str = QString("abs_Y:  %1m").arg(absolutePosition.v[1], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
    str = QString("abs_Z:  %1m").arg(absolutePosition.v[2], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
    str = QString("groundNullHeight: %1m").arg(groundNullHeight, 7, 'f', 2);
    painter.drawText(screen_x, screen_y, str);
    //*/
}

void Scenery3d::drawDebug(StelCore* core)
{
    if(!hasModels)
        return;

    const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
    StelPainter painter(prj);
    painter.setFont(debugTextFont);
    painter.setColor(1,0,1,1);
    // For now, these messages print light mixture values.
    painter.drawText(20, 160, lightMessage);
    painter.drawText(20, 145, lightMessage2);
    painter.drawText(20, 130, lightMessage3);
    // PRINT OTHER MESSAGES HERE:

    float screen_x = prj->getViewportWidth()  - 500.0f;
    float screen_y = prj->getViewportHeight() - 300.0f;

    //Show some debug aids
    if(debugEnabled)
    {
	float debugTextureSize = 128.0f;
	float screen_x = prj->getViewportWidth() - debugTextureSize - 30;
	float screen_y = prj->getViewportHeight() - debugTextureSize - 30;

//	std::string cap = "Camera depth";
//	painter.drawText(screen_x-150, screen_y+130, QString(cap.c_str()));

//	glBindTexture(GL_TEXTURE_2D, camDepthTex);
//	painter.drawSprite2dMode(screen_x, screen_y, debugTextureSize);

	if(shadowsEnabled)
	{
		for(int i=0; i<frustumSplits; i++)
		{
			std::string cap = "SM "+toString(i);
			painter.drawText(screen_x+70, screen_y+130, QString(cap.c_str()));

			glBindTexture(GL_TEXTURE_2D, shadowMapsArray[i]);
			painter.drawSprite2dMode(screen_x, screen_y, debugTextureSize);

			int tmp = screen_y - debugTextureSize-30;
			painter.drawText(screen_x-100, tmp, QString("zNear: %1").arg(frustumArray[i].zNear, 7, 'f', 2));
			painter.drawText(screen_x-100, tmp-15.0f, QString("zFar: %1").arg(frustumArray[i].zFar, 7, 'f', 2));

			screen_x -= 280;
		}
	}

	painter.drawText(screen_x+250.0f, screen_y-200.0f, QString("Splitweight: %1").arg(splitWeight, 3, 'f', 2));
    }

    screen_y -= 100.f;
    QString str = QString("Drawn: %1").arg(drawn);
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = "View Pos";
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = QString("%1 %2 %3").arg(viewPos.v[0], 7, 'f', 2).arg(viewPos.v[1], 7, 'f', 2).arg(viewPos.v[2], 7, 'f', 2);
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = "View Dir";
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = QString("%1 %2 %3").arg(viewDir.v[0], 7, 'f', 2).arg(viewDir.v[1], 7, 'f', 2).arg(viewDir.v[2], 7, 'f', 2);
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = "View Up";
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = QString("%1 %2 %3").arg(viewUp.v[0], 7, 'f', 2).arg(viewUp.v[1], 7, 'f', 2).arg(viewUp.v[2], 7, 'f', 2);
    painter.drawText(screen_x, screen_y, str);

    screen_y -= 30.0f;
    str = QString("Current shader: %1").arg(static_cast<int>(curEffect));
    painter.drawText(screen_x, screen_y, str);

    screen_y -= 30.0f;
    str = QString("Venus: %1").arg(static_cast<int>(venusOn));
    painter.drawText(screen_x, screen_y, str);
}

void Scenery3d::initShadowMapping()
{
    //Query how many texture units we have at disposal
    GLint maxTex, maxComb;
    glGetIntegerv(GL_MAX_TEXTURE_COORDS, &maxTex);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxComb);

    qDebug() << "Available texture units:" << std::max(maxTex, maxComb);


    //Generate FBO - has to be QGLFramebufferObject for some reason.. Generating a normal one lead to bizzare texture results
    //We use handle() to get the id and work as if we created a normal FBO. This is because QGLFramebufferObject doesn't support attaching a texture to the FBO
    QOpenGLFramebufferObject *qglFBO=new QOpenGLFramebufferObject(shadowmapSize, shadowmapSize, QOpenGLFramebufferObject::Depth);
    shadowFBO = qglFBO->handle();
    //if (shadowFBO->attachment() != QGLFramebufferObject::Depth){
    //    qWarning()<< "Scenery3d: Framebuffer failed to aquire depth buffer. Try smaller shadowmap_size!";
    //}

    if (qglFBO->isValid())
        qWarning() << "[Scenery3D] initShadowMapping() qglFBO valid\n";
    else
        qWarning() << "[Scenery3D] initShadowMapping() qglFBO invalid\n";


    //Bind the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    for(int i=0; i<frustumSplits; i++)
    {
        //Generate the depth maps
        glGenTextures(1, &shadowMapsArray[i]);
        //Activate the texture unit - we want sahdows + textures so this is crucial with the current Stellarium pipeline - we start at unit 3
        glActiveTexture(GL_TEXTURE3+i);

        //Bind the depth map and setup parameters
        glBindTexture(GL_TEXTURE_2D, shadowMapsArray[i]);

#if QT_VERSION >= 0x040800
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowmapSize, shadowmapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, shadowmapSize, shadowmapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
#endif

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float ones[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, ones);
        //Attach the depthmap to the Buffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapsArray[i], 0);
    }

    glDrawBuffer(GL_NONE); // essential for depth-only FBOs!!!
    glReadBuffer(GL_NONE); // essential for depth-only FBOs!!!

#if QT_VERSION >= 0x040800
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
#else
    if(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE)
#endif
        qWarning() << "[Scenery3D] GL_FRAMEBUFFER_COMPLETE failed, can't use FBO";

    //Done. Unbind and switch to normal texture unit 0
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0);



    //Generate FBO - has to be QGLFramebufferObject for some reason.. Generating a normal one lead to bizzare texture results
    //We use handle() to get the id and work as if we created a normal FBO. This is because QGLFramebufferObject doesn't support attaching a texture to the FBO
    QOpenGLFramebufferObject *qglCamFBO=new QOpenGLFramebufferObject(shadowmapSize, shadowmapSize, QOpenGLFramebufferObject::Depth);
    camDepthFBO = qglCamFBO->handle();

    if (qglFBO->isValid())
        qWarning() << "[Scenery3D] initShadowMapping() qglCamFBO valid\n";
    else
        qWarning() << "[Scenery3D] initShadowMapping() qglCamFBO invalid\n";


    //Bind the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, camDepthFBO);

    //Generate the depth maps and color maps for min/max
    glGenTextures(1, &camDepthTex);
    glGenTextures(2, pingpongTex);

    //Activate the texture unit
    glActiveTexture(GL_TEXTURE7);
    //Bind the depth map and setup parameters
    glBindTexture(GL_TEXTURE_2D, camDepthTex);

#if QT_VERSION >= 0x040800
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowmapSize, shadowmapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, shadowmapSize, shadowmapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
#endif

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float ones[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, ones);
    glGenerateMipmap(GL_TEXTURE_2D);

    //Setup Color attachment 0 and 1
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, pingpongTex[writeTex]);
    //turn off filtering and wrap modes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    //define texture with floating point format
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, shadowmapSize, shadowmapSize, 0, GL_RGBA, GL_FLOAT,0);

    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_2D, pingpongTex[readTex]);
    //turn off filtering and wrap modes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    //define texture with floating point format
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, shadowmapSize, shadowmapSize, 0, GL_RGBA, GL_FLOAT,0);

    //Attach to the Buffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, camDepthTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachments[writeTex], GL_TEXTURE_2D, pingpongTex[writeTex], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachments[readTex], GL_TEXTURE_2D, pingpongTex[readTex], 0);

    glDrawBuffer(GL_NONE); // essential for depth-only FBOs!!!
    glReadBuffer(GL_NONE); // essential for depth-only FBOs!!!

#if QT_VERSION >= 0x040800
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
#else
    if(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE)
#endif
        qWarning() << "[Scenery3D] GL_FRAMEBUFFER_COMPLETE failed, can't use FBO";

    //Create PBO for readback when analyzing the depth buffer
    glGenBuffers(1, &camDepthPBO);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, camDepthPBO);
    glBufferData(GL_PIXEL_PACK_BUFFER, shadowmapSize*shadowmapSize*sizeof(GLfloat), NULL, GL_STREAM_READ);
    //Done. Unbind
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    //Done. Unbind and switch to normal texture unit 0
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
}

void Scenery3d::draw(StelCore* core)
{
    if (shadowsEnabled)
    {
	generateShadowMap(core);
    }

    if (core->getCurrentProjectionType() == StelCore::ProjectionPerspective)
    {
        drawObjModel(core);
    }
    else
    {
	generateCubeMap();
        drawFromCubeMap(core);
    }
    if (textEnabled) drawCoordinatesText(core);
    if (debugEnabled)
    {
	drawDebug(core);
    }
}

// Replacement for gluLookAt. See http://www.opengl.org/sdk/docs/man2/xhtml/gluLookAt.xml
void Scenery3d::nogluLookAt(double eyeX,  double eyeY,  double eyeZ,  double centerX,  double centerY,  double centerZ,  double upX,  double upY,  double upZ)
{
    Vec3d f(centerX-eyeX, centerY-eyeY, centerZ-eyeZ);
    Vec3d up(upX, upY, upZ);
    f.normalize();
    up.normalize();
    Vec3d s=f ^ up;   // f x up cross product
    Vec3d snorm=s;
    snorm.normalize();
    Vec3d u=snorm ^ f;
    Mat4f M(s[0], s[1], s[2], 0.0f, u[0], u[1], u[2], 0.0f, -f[0], -f[1], -f[2], 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    glMultMatrixf(M.r);
    glTranslated(-eyeX, -eyeY, -eyeZ);
}
