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
#include "GLFuncs.hpp"
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
    fprintf(stderr, "[line %d] GL Error: %d\n",         \
    __LINE__, err);                     \
    fflush(stderr);                                     \
    }                                                   \
}

#define MAXSPLITS 4
#define NEARZ 1.0f;
//If FARZ is too small for your model change it up
#define FARZ 15000.0f

//macro for easier uniform setting
#define SET_UNIFORM(shd,uni,val) shd->setUniformValue(shaderManager.uniformLocation(shd,uni),val)

static const float AMBIENT_BRIGHTNESS_FACTOR=0.05f;
static const float LUNAR_BRIGHTNESS_FACTOR=0.2f;
static const float VENUS_BRIGHTNESS_FACTOR=0.005f;

//this is the place where this is initialized
GLExtFuncs glExtFuncs;

Scenery3d::Scenery3d(Scenery3dMgr* parent)
    : parent(parent), currentScene(),torchBrightness(0.5f),cubemapSize(1024),shadowmapSize(1024),
      absolutePosition(0.0, 0.0, 0.0), movement(0.0f,0.0f,0.0f),core(NULL),heightmap(NULL)
{
	qDebug()<<"Scenery3d constructor...";

	cubeMapTex = 0;
	cubeFBO = 0;
	cubeRB = 0;
	cubeMapDepth = 0;

	supportsGSCubemapping = false;
	useGSCubemapping = false;
	reinitCubemapping = true;

	//create the front cubemap face vertices
	//created a 20x20 subdivision to give a good approximation of non-linear projections

	//TODO optimize this by using indexed geometry and TRIANGLE_FAN!
	// This currently uses 20x20x6=2400 vertices per face, 14400 in total.
	// This may be a bit much for CPU transformation in each frame.

	const int sub = 20;
	const float d_sub_v = 2.0 / sub;
	for (int y = 0; y < sub; y++) {
		for (int x = 0; x < sub; x++) {
			float x0 = -1.0 + x * d_sub_v;
			float x1 = x0 + d_sub_v;
			float y0 = -1.0 + y * d_sub_v;
			float y1 = y0 + d_sub_v;

			Vec3f v[] = {
				Vec3f(x0, 1.0, y0),
				Vec3f(x1, 1.0, y0),
				Vec3f(x1, 1.0, y1),
				Vec3f(x0, 1.0, y0),
				Vec3f(x1, 1.0, y1),
				Vec3f(x0, 1.0, y1),
			};
			for (int i=0; i<6; i++) {
				v[i].normalize();
				cubePlaneFront<< v[i];
			}
		}
	}

	int vtxCount = cubePlaneFront.size();

	//only positions are required for cubemapping
	QVector<Vec3f> cubeVtx;
	cubeVtx.reserve(vtxCount * 6);
	//init with copies of front face
	cubeVtx<<cubePlaneFront;  //E face y=1
	cubeVtx<<cubePlaneFront;  //S face x=1
	cubeVtx<<cubePlaneFront;  //N face x=-1
	cubeVtx<<cubePlaneFront;  //W face y=-1
	cubeVtx<<cubePlaneFront;  //down face z=-1
	cubeVtx<<cubePlaneFront;  //up face z=1


	//create the other cube faces by rotating the front face
#define PLANE(_STARTIDX_, _MAT_) for(int i=_STARTIDX_;i<_STARTIDX_ + vtxCount;i++){ _MAT_.transfo(cubeVtx[i]); }

	PLANE(1 * vtxCount, Mat4f::zrotation(-M_PI_2));
	PLANE(2 * vtxCount, Mat4f::zrotation(M_PI_2));
	PLANE(3 * vtxCount, Mat4f::zrotation(M_PI));
	PLANE(4 * vtxCount, Mat4f::xrotation(-M_PI_2));
	PLANE(5 * vtxCount, Mat4f::xrotation(M_PI_2));
#undef PLANE

	cubeVertices = cubeVtx;


	shaderParameters.pixelLighting = false;
	shaderParameters.shadows = false;
	shaderParameters.bump = false;
	shaderParameters.shadowFilter = true;
	shaderParameters.shadowFilterHQ = false;
	shaderParameters.geometryShader = false;

    torchEnabled = false;
    textEnabled = false;
    debugEnabled = false;
    curEffect = No;
    lightCamEnabled = false;
    hasModels = false;
    sceneBoundingBox = AABB(Vec3f(0.0f), Vec3f(0.0f));
    frustEnabled = false;

    venusOn = false;

    //Preset frustumSplits
    frustumSplits = 4;
    //Make sure we dont exceed MAXSPLITS or go below 1
    frustumSplits = qMax(qMin(frustumSplits, MAXSPLITS), 1);


    camFOV = 90.0f;
    camAspect = 1.0f;
    camNear = NEARZ;
    camFar = FARZ;

    parallaxScale = 0.015f;

	debugTextFont.setFamily("Courier");
	debugTextFont.setPixelSize(16);


	//initialize cube rotations... found by trial and error :)
	QMatrix4x4 stackBase;

	//all angles were found using some experimenting :)
	//this is the EAST face (y=1)
	stackBase.rotate(90.0f,-1.0f,0.0f,0.0f);

	//south (x=1) ok
	cubeRotation[0] = stackBase;
	cubeRotation[0].rotate(-90.0f,0.0f,1.0f,0.0f);
	cubeRotation[0].rotate(90.0f,0.0f,0.0f,1.0f);
	//NORTH (x=-1) ok
	cubeRotation[1] = stackBase;
	cubeRotation[1].rotate(90.0f,0.0f,1.0f,0.0f);
	cubeRotation[1].rotate(-90.0f,0.0f,0.0f,1.0f);
	//EAST (y=1) ok
	cubeRotation[2] = stackBase;
	//west (y=-1) ok
	cubeRotation[3] = stackBase;
	cubeRotation[3].rotate(180.0f,-1.0f,0.0f,0.0f);
	//top (z=1) ok
	cubeRotation[4] = stackBase;
	cubeRotation[4].rotate(-90.0f,1.0f,0.0f,0.0f);
	//bottom (z=-1)
	cubeRotation[5] = stackBase;
	cubeRotation[5].rotate(90.0f,1.0f,0.0f,0.0f);
	cubeRotation[5].rotate(180.0f,0.0f,0.0f,1.0f);


	qDebug()<<"Scenery3d constructor...done";
}

Scenery3d::~Scenery3d()
{
    if (heightmap) {
        delete heightmap;
        heightmap = NULL;
    }

	deleteShadowmapping();
	deleteCubemapping();
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
	//TODO FS: introduce background loading of models!
	//Everything that OBJ does could probably be moved to a background thread, except upload of OpenGL objects which should happen in main thread.

	objModel.clean();
	qDebug()<<"OBJ memory after clean: "<<objModel.memoryUsage();
	groundModel.clean();

	QString modelFile = StelFileMgr::findFile( currentScene.fullPath+ "/" + currentScene.modelScenery);
	if(!objModel.load(modelFile, objVertexOrder, currentScene.sceneryGenerateNormals))
            throw std::runtime_error("Failed to load OBJ file.");

	hasModels = objModel.hasStelModels();
	//transform the vertices of the model to match the grid
	objModel.transform( zRot2Grid );
        //objModel->transform(obj2gridMatrix);

	//upload data to GL
	objModel.uploadTexturesGL();
	objModel.uploadBuffersGL();

	qDebug()<<"OBJ memory after load: "<<objModel.memoryUsage();


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
	else if(currentScene.modelGround != "NULL")
        {
	    modelFile = StelFileMgr::findFile(currentScene.fullPath + "/" + currentScene.modelGround);
	    if(!groundModel.load(modelFile, objVertexOrder, currentScene.groundGenerateNormals))
                throw std::runtime_error("Failed to load OBJ file.");

	    groundModel.transform( zRot2Grid );

	    //the ground model needs no opengl uploads, so we skip them
        }

	if (currentScene.hasLocation())
	{ if (currentScene.altitudeFromModel) // previouslay marked meaningless
          {
		currentScene.location->altitude=static_cast<int>(0.5*(objModel.getBoundingBox().min[2]+objModel.getBoundingBox().max[2])+currentScene.modelWorldOffset[2]);
          }
        }

	if (currentScene.groundNullHeightFromModel)
        {
	    currentScene.groundNullHeight=((groundModel.isLoaded()) ? groundModel.getBoundingBox().min[2] : objModel.getBoundingBox().min[2]);
            //groundNullHeight = objModel->getBoundingBox()->min[2];
	    qDebug() << "Ground outside model is " << currentScene.groundNullHeight  << "m high (in model coordinates)";
        }
	else qDebug() << "Ground outside model stays " << currentScene.groundNullHeight  << "m high (in model coordinates)";

	//delete old heightmap
	if(heightmap)
	{
		delete heightmap;
		heightmap = NULL;
	}

	if (groundModel.isLoaded())
        {	
	    heightmap = new Heightmap(&groundModel);
	    heightmap->setNullHeight(currentScene.groundNullHeight);
        }

	if(currentScene.startPositionFromModel)
	{
		absolutePosition.v[0] = -(objModel.getBoundingBox().max[0]+objModel.getBoundingBox().min[0])/2.0;
		qDebug() << "Setting Easting  to BBX center: " << objModel.getBoundingBox().min[0] << ".." << objModel.getBoundingBox().max[0] << ": " << absolutePosition.v[0];
		absolutePosition.v[1] = -(objModel.getBoundingBox().max[1]+objModel.getBoundingBox().min[1])/2.0;
		qDebug() << "Setting Northing to BBX center: " << objModel.getBoundingBox().min[1] << ".." << objModel.getBoundingBox().max[1] << ": " << -absolutePosition.v[1];
	}
	else
	{
		absolutePosition[0] = currentScene.relativeStartPosition[0];
		absolutePosition[1] = currentScene.relativeStartPosition[1];
	}
	eye_height = currentScene.eyeLevel;

	OBJ* cur = &objModel;
#if GROUND_MODEL
	cur = &groundModel;
#endif

        //Set the scene's AABB
	setSceneAABB(cur->getBoundingBox());

        //finally, set core to enable update().
        this->core=StelApp::getInstance().getCore();

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
	//TODO FS maybe move this to Mgr, so that input is separate from rendering and scene management?

    if ((e->type() == QKeyEvent::KeyPress) && (e->modifiers() & Qt::ControlModifier))
    {
        // Pressing CTRL+ALT: 5x, CTRL+SHIFT: 10x speedup; CTRL+SHIFT+ALT: 50x!
        float speedup=((e->modifiers() & Qt::ShiftModifier)? 10.0f : 1.0f);
        speedup *= ((e->modifiers() & Qt::AltModifier)? 5.0f : 1.0f);
        switch (e->key())
	{
	    case Qt::Key_PageUp:    movement[2] = -1.0f * speedup; e->accept(); break;
	    case Qt::Key_PageDown:  movement[2] =  1.0f * speedup; e->accept(); break;
	    case Qt::Key_Up:        movement[1] = -1.0f * speedup; e->accept(); break;
	    case Qt::Key_Down:      movement[1] =  1.0f * speedup; e->accept(); break;
	    case Qt::Key_Right:     movement[0] =  1.0f * speedup; e->accept(); break;
	    case Qt::Key_Left:      movement[0] = -1.0f * speedup; e->accept(); break;
	    case Qt::Key_P:         saveFrusts(); e->accept(); break;
        }
    }
    else if ((e->type() == QKeyEvent::KeyRelease) && (e->modifiers() & Qt::ControlModifier))
    {
        if (e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown ||
            e->key() == Qt::Key_Up     || e->key() == Qt::Key_Down     ||
            e->key() == Qt::Key_Left   || e->key() == Qt::Key_Right     )
            {
		movement[0] = movement[1] = movement[2] = 0.0f;
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

void Scenery3d::setSceneAABB(const AABB& bbox)
{
    sceneBoundingBox = bbox;
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

    /* commented out for compiler warning
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
    };*/

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

	Vec3d move(( movement[0] * std::cos(az) + movement[1] * std::sin(az)),
		   ( movement[0] * std::sin(az) - movement[1] * std::cos(az)),
		   movement[2]);

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

void Scenery3d::setupPassUniforms(QOpenGLShaderProgram *shader)
{
	//send projection matrix
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MAT_PROJECTION, projectionMatrix);

	//send light strength
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_LIGHT_AMBIENT,lightInfo.ambient);
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_LIGHT_DIFFUSE,lightInfo.directional);

	//set alpha test threshold (this is scene-global for now)
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_FLOAT_ALPHA_THRESH,currentScene.transparencyThreshold);

	//-- Shadowing setup -- this was previously in generateCubeMap_drawSceneWithShadows
	//first check if shader supports shadows
	GLint loc = shaderManager.uniformLocation(shader,ShaderMgr::UNIFORM_VEC_SQUAREDSPLITS);

	//ALWAYS update the shader matrices, even if "no" shadow is cast
	//this fixes weird time-dependent crashes (this was fun to debug)
	if(shaderParameters.shadows && loc >= 0)
	{
		//Calculate texture matrix for projection
		//This matrix takes us from eye space to the light's clip space
		//It is postmultiplied by the inverse of the current view matrix when specifying texgen
		static const QMatrix4x4 biasMatrix(0.5f, 0.0f, 0.0f, 0.5f,
					     0.0f, 0.5f, 0.0f, 0.5f,
					     0.0f, 0.0f, 0.5f, 0.5f,
					     0.0f, 0.0f, 0.0f, 1.0f);	//bias from [-1, 1] to [0, 1]

		//Holds the squared frustum splits necessary for the lookup in the shader
		Vec4f squaredSplits = Vec4f(0.0f);
		for(int i=0; i<frustumSplits; i++)
		{
		    squaredSplits.v[i] = frustumArray.at(i).zFar * frustumArray.at(i).zFar;

		    //Bind current depth map texture
		    glActiveTexture(GL_TEXTURE3+i);
		    glBindTexture(GL_TEXTURE_2D, shadowMapsArray.at(i));

		    //Compute texture matrix
		    QMatrix4x4 texMat = biasMatrix * shadowCPM.at(i);

		    //Send to shader
		    SET_UNIFORM(shader,static_cast<ShaderMgr::UNIFORM>(ShaderMgr::UNIFORM_TEX_SHADOW0+i), 3+i);
		    SET_UNIFORM(shader,static_cast<ShaderMgr::UNIFORM>(ShaderMgr::UNIFORM_MAT_SHADOW0+i),texMat);
		}

		//Send squared splits to the shader
		shader->setUniformValue(loc, squaredSplits.v[0], squaredSplits.v[1], squaredSplits.v[2], squaredSplits.v[3]);
	}
}

void Scenery3d::setupFrameUniforms(QOpenGLShaderProgram *shader)
{
	//-- Transform setup --
	//check if shader wants a MVP or separate matrices
	GLint loc = shaderManager.uniformLocation(shader,ShaderMgr::UNIFORM_MAT_MVP);
	if(loc>=0)
	{
		shader->setUniformValue(loc,projectionMatrix * modelViewMatrix);
	}
	else
	{
		//note that we DO NOT set the projection matrix here, done in setupPassUniforms already

		//this macro saves a bit of writing
		SET_UNIFORM(shader,ShaderMgr::UNIFORM_MAT_MODELVIEW, modelViewMatrix);
	}

	//-- Lighting setup --
	//check if we require a normal matrix, this is assumed to be required for all "shading" shaders
	loc = shaderManager.uniformLocation(shader,ShaderMgr::UNIFORM_MAT_NORMAL);
	if(loc>=0)
	{
		QMatrix3x3 normalMatrix = modelViewMatrix.normalMatrix();
		shader->setUniformValue(loc,normalMatrix);

		//assume light direction is only required when normal matrix is also used (would not make much sense alone)
		SET_UNIFORM(shader,ShaderMgr::UNIFORM_LIGHT_DIRECTION, lightInfo.lightDirectionWorld); //set light dir in world space

		//check if the shader wants view space info
		loc = shaderManager.uniformLocation(shader,ShaderMgr::UNIFORM_LIGHT_DIRECTION_VIEW);
		if(loc>=0)
			shader->setUniformValue(loc,(normalMatrix * lightInfo.lightDirectionWorld));

	}
}

void Scenery3d::setupMaterialUniforms(QOpenGLShaderProgram* shader, const OBJ::Material &mat)
{
	//send off material parameters without further changes for now
	glUniform3fv(shaderManager.uniformLocation(shader,ShaderMgr::UNIFORM_MTL_AMBIENT),1,mat.ambient);
	glUniform3fv(shaderManager.uniformLocation(shader,ShaderMgr::UNIFORM_MTL_DIFFUSE),1,mat.diffuse);
	glUniform3fv(shaderManager.uniformLocation(shader,ShaderMgr::UNIFORM_MTL_SPECULAR),1,mat.specular);
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MTL_SHININESS,mat.shininess);
	//force alpha to 1 here for non-translucent mats (fixes incorrect blending in cubemap)
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MTL_ALPHA, mat.illum == OBJ::TRANSLUCENT? mat.alpha : 1.0f);

	if(mat.texture)
	{
		mat.texture->bind(0); //this already sets glActiveTexture(0)
		SET_UNIFORM(shader,ShaderMgr::UNIFORM_TEX_DIFFUSE,0);
	}
	if(shaderParameters.bump && mat.bump_texture)
	{
		mat.bump_texture->bind(1);
		SET_UNIFORM(shader,ShaderMgr::UNIFORM_TEX_BUMP,1);
	}
	if(shaderParameters.bump && mat.height_texture)
	{
		mat.bump_texture->bind(2);
		SET_UNIFORM(shader,ShaderMgr::UNIFORM_TEX_HEIGHT,2);
	}
}

void Scenery3d::drawArrays(bool shading, bool blendAlphaAdditive)
{
	drawn = 0;

	QOpenGLShaderProgram* curShader = NULL;
	QSet<QOpenGLShaderProgram*> initialized;

	//override some shader Params
	GlobalShaderParameters pm = shaderParameters;
	if(venusOn)
		pm.shadowFilter = false;

	//bind VAO
	objModel.bindGL();

	//TODO optimize: clump models with same material together when first loading to minimize state changes
	const OBJ::Material* lastMaterial = NULL;
	bool blendEnabled = false;
	for(int i=0; i<objModel.getNumberOfStelModels(); i++)
	{

		const OBJ::StelModel* pStelModel = &objModel.getStelModel(i);
		const OBJ::Material* pMaterial = pStelModel->pMaterial;
		Q_ASSERT(pMaterial);

		if(shading && lastMaterial!=pMaterial)
		{
			//get a shader from shadermgr that fits the current state + material combo
			QOpenGLShaderProgram* newShader = shaderManager.getShader(pm,pMaterial);
			if(!newShader)
			{
				//shader invalid, can't draw
				parent->showMessage("Scenery3d shader error, can't draw. Check debug output for details.");
				break;
			}
			if(newShader!=curShader)
			{
				curShader = newShader;
				curShader->bind();
				if(!initialized.contains(curShader))
				{
					//needs init
					setupPassUniforms(curShader);
					setupFrameUniforms(curShader);

					//rebind vao
					//objModel.bindGL();
					initialized.insert(curShader);
				}
			}

			setupMaterialUniforms(curShader,*pMaterial);
			lastMaterial = pMaterial;

			if(pMaterial->illum == OBJ::TRANSLUCENT )
			{
				//TODO provide Z-sorting for transparent objects (center of bounding box should be fine)
				if(!blendEnabled)
				{
					glEnable(GL_BLEND);
					if(blendAlphaAdditive)
						glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					else //traditional direct blending
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					blendEnabled = true;
				}
			}
			else
			{
				if(blendEnabled)
				{
					glDisable(GL_BLEND);
					blendEnabled=false;
				}
			}
		}

		glDrawElements(GL_TRIANGLES, pStelModel->triangleCount * 3, GL_UNSIGNED_INT, reinterpret_cast<const void*>(pStelModel->startIndex * sizeof(unsigned int)));
	}

	if(shading&&curShader)
		curShader->release();

	if(blendEnabled)
		glDisable(GL_BLEND);

	//release VAO
	objModel.unbindGL();
}

void Scenery3d::sendToShader(const OBJ::StelModel* pStelModel, Effect cur, bool& tangEnabled, int& tangLocation)
{
    tangEnabled = false;
    //TODO move this to setupXXXUniforms

    //dummy for now
    QOpenGLShaderProgram* curShader=NULL;
    if(cur != No)
    {
        curShader->setUniformValue("boolDebug", debugEnabled);
	curShader->setUniformValue("fTransparencyThresh", currentScene.transparencyThreshold);

        curShader->setUniformValue("boolVenus", venusOn);

        int iIllum = pStelModel->pMaterial->illum;
        if(iIllum < 0 || iIllum > 2)
        {
	    if(iIllum != 9 && iIllum != 4)
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
	    if(objModel.hasTangents())
            {
                tangLocation = curShader->attributeLocation("vecTangent");
                glEnableVertexAttribArray(tangLocation);
		glVertexAttribPointer(tangLocation, 4, GL_FLOAT, 0, objModel.getVertexSize(), objModel.getVertexArray()->tangent);
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

void Scenery3d::computeFrustumSplits(float zNear, float zFar)
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

void Scenery3d::computePolyhedron(Polyhedron& body,const Frustum& frustum,const Vec3f& shadowDir)
{
    //Building a convex body for directional lights according to Wimmer et al. 2006


    //Add the Frustum to begin with
    body.add(frustum);
    //Intersect with the scene AABB
    body.intersect(sceneBoundingBox);
    //Extrude towards light direction
    body.extrude(shadowDir, sceneBoundingBox);
}

void Scenery3d::computeOrthoProjVals(const Vec3f shadowDir,float& orthoExtent,float& orthoNear,float& orthoFar)
{
    //Focus the light first on the entire scene
    float maxZ = 0.0f;
    float minZ = std::numeric_limits<float>::max();
    orthoExtent = 0.0f;

    Vec3f eye = shadowDir;
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

	orthoExtent = std::max(std::abs(toCam.dot(left)), orthoExtent);
	orthoExtent = std::max(std::abs(toCam.dot(right)), orthoExtent);
	orthoExtent = std::max(std::abs(toCam.dot(up)), orthoExtent);
	orthoExtent = std::max(std::abs(toCam.dot(down)), orthoExtent);
    }

    //Make sure planes arent too small
    orthoNear = std::max(minZ, 1.0f);
    orthoFar = std::max(maxZ, orthoNear + 1.0f);
}

void Scenery3d::computeCropMatrix(int frustumIndex,const Vec3f& shadowDir)
{
    //Compute the light frustum, only need to do this once because we're just cropping it afterwards
	static float orthoExtent,orthoFar,orthoNear;
    if(frustumIndex == 0)
    {
	computeOrthoProjVals(shadowDir,orthoExtent,orthoNear,orthoFar);
    }

    //Calculating a fitting Projection Matrix for the light
    QMatrix4x4 lightProj;
    //Mat4f lightProj, lightMVP, c;

    //Setup the Ortho Projection based on found z values
    lightProj.ortho(-orthoExtent,orthoExtent,-orthoExtent,orthoExtent,orthoNear,orthoFar);

    QMatrix4x4 lightMVP = lightProj*modelViewMatrix;

    float maxX = -std::numeric_limits<float>::max();
    float maxY = maxX;
    float maxZ = maxX;
    float minX = std::numeric_limits<float>::max();
    float minY = minX;
    float minZ = minX;

    //! Uncomment this and the other marked lines to get no artifacts (but way worse shadow quality
    //! making the second split pretty much useless
    AABB bb;
    for(int i=0; i<focusBodies.at(frustumIndex).getVertCount(); i++)
	bb.expand(focusBodies.at(frustumIndex).getVerts().at(i));

    //Project the frustum into light space and find the boundaries
    for(int i=0; i<focusBodies.at(frustumIndex).getVertCount(); i++)
    {
	const Vec3f tmp = focusBodies.at(frustumIndex).getVerts().at(i);
	QVector4D transf4 = lightMVP*QVector4D(tmp.v[0], tmp.v[1], tmp.v[2], 1.0f);
	QVector3D transf = transf4.toVector3DAffine();

	if(transf.x() > maxX) maxX = transf.x();
	if(transf.x() < minX) minX = transf.x();
	if(transf.y() > maxY) maxY = transf.y();
	if(transf.y() < minY) minY = transf.y();
	if(transf.z() > maxZ) maxZ = transf.z();
	if(transf.z() < minZ) minZ = transf.z();
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
    QMatrix4x4 crop(scaleX, 0.0f,   0.0f, offsetX,
		    0.0f,   scaleY, 0.0f, offsetY,
		    0.0f,   0.0f,   scaleZ, offsetZ,
		    0.0f,   0.0f,   0.0f, 1.0f);

    //Crop the light projection matrix
    projectionMatrix = crop * lightProj;
    //store final matrix for retrieval later
    shadowCPM[frustumIndex] = projectionMatrix * modelViewMatrix;
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

    const QVector<Vec3f> &verts = p.getVerts();
    for(int i=0; i<p.getVertCount(); i++)
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

    //Setup the subfrusta
    for(int i=0; i<frustumSplits; i++)
    {
        frustumArray[i].setCamInternals(camFOV, camAspect, camNear, camFar);
    }
}

bool Scenery3d::generateShadowMap()
{
	//test if shadow mapping has been initialized
	if(shadowFBOs.size()==0)
	{
		if(!initShadowmapping())
			return false; //can't use shadowmaps
	}

	//Needed so we can actually adjust the frustum upwards after it was already adjusted downwards.
	//This resets it and they'll be recomputed in adjustFrustum();
	camNear = NEARZ;
	camFar = FARZ;

	//Adjust the frustum to the scene before analyzing the view samples
	adjustFrustum();

	//Determine sun position
	SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	Vec3d sunPosition = ssystem->getSun()->getAltAzPosAuto(core);
	//zRotateMatrix.transfo(sunPosition); // GZ: These rotations were commented out - testing 20120122->correct!
	sunPosition.normalize();
	// GZ: at night, a near-full Moon can cast good shadows.
	Vec3d moonPosition = ssystem->getMoon()->getAltAzPosAuto(core);
	//zRotateMatrix.transfo(moonPosition);
	moonPosition.normalize();
	Vec3d venusPosition = ssystem->searchByName("Venus")->getAltAzPosAuto(core);
	//zRotateMatrix.transfo(venusPosition);
	venusPosition.normalize();

	//find the direction the shadow is cast (= light direction)
	Vec3f shadowDirV3f;

	//Select view position based on which planet is visible
	if (sunPosition[2]>0)
	{
		shadowDirV3f = Vec3f(sunPosition.v[0],sunPosition.v[1],sunPosition.v[2]);
		lightInfo.shadowCaster = Sun;
		venusOn = false;
	}
	else if (moonPosition[2]>0)
	{
		shadowDirV3f = Vec3f(moonPosition.v[0],moonPosition.v[1],moonPosition.v[2]);
		lightInfo.shadowCaster = Moon;
		venusOn = false;
	}
	else
	{
		//TODO fix case where not even Venus is visible, led to problems for me today
		shadowDirV3f = Vec3f(venusPosition.v[0],venusPosition.v[1],venusPosition.v[2]);
		lightInfo.shadowCaster = Venus;
		venusOn = true;
	}

	QVector3D shadowDir(shadowDirV3f.v[0],shadowDirV3f.v[1],shadowDirV3f.v[2]);
	static const QVector3D vZero = QVector3D();
	static const QVector3D vZeroZeroOne = QVector3D(0,0,1);

	//calculate lights modelview matrix
	modelViewMatrix.setToIdentity();
	modelViewMatrix.lookAt(shadowDir,vZero,vZeroZeroOne);

	//Compute and set z-distances for each split
	computeFrustumSplits(camNear, camFar);

	//perform actual rendering
	return renderShadowMaps(shadowDirV3f);
}

bool Scenery3d::renderShadowMaps(const Vec3f& shadowDir)
{
	//bind transform shader
	QOpenGLShaderProgram* tfshd = shaderManager.getTransformShader();
	if(!tfshd)
	{
		parent->showMessage("Shadow transformation shader can not be used, can not use shadows. Check log for details.");
		return false;
	}
	tfshd->bind();

	//Fix selfshadowing
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.05f,2.0f);
	//! This is now done in shader via a bias. If that's ever a problem uncomment this part and the disabling farther down and change
	//! glCullFace(GL_FRONT) to GL_BACK (farther up from here)

	//GL state
	//enable depth + front face culling
	glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	//frontface culling for ESM!
	glCullFace(GL_FRONT);

	//Set viewport to shadowmap
	glViewport(0, 0, shadowmapSize, shadowmapSize);

	//For each split
	for(int i=0; i<frustumSplits; i++)
	{
		//Calculate the Frustum for this split
		frustumArray[i].calcFrustum(viewPos, viewDir, viewUp);

		//Find the convex body that encompasses all shadow receivers and casters for this split
		focusBodies[i].clear();
		computePolyhedron(focusBodies[i],frustumArray[i],shadowDir);

		//qDebug() << i << ".split vert count:" << focusBodies[i]->getVertCount();

		if(focusBodies[i].getVertCount())
		{
			//Calculate the crop matrix so that the light's frustum is tightly fit to the current split's PSR+PSC polyhedron
			//This alters the ProjectionMatrix of the light
			//the final light matrix is stored in shadowCPM
			computeCropMatrix(i,shadowDir);

			//really only mvp required, so only call this
			setupFrameUniforms(tfshd);

			glBindFramebuffer(GL_FRAMEBUFFER,shadowFBOs.at(i));

			//Clear everything
			glClear(GL_DEPTH_BUFFER_BIT);

			//Draw the scene
			drawArrays(false);
		}
	}


	//Unbind
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//reset viewport (see StelPainter::setProjector)
	const Vec4i& vp = altAzProjector->getViewport();
	glViewport(vp[0], vp[1], vp[2], vp[3]);

	//Move polygons back to normal position
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.0f,0.0f);

	//Reset
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);

	tfshd->release();

	return true;
}

void Scenery3d::calculateLighting()
{
	//calculate which light source we need + intensity
	float ambientBrightness, directionalBrightness; // was: lightBrightness;
	Vec3f lightsourcePosition; //should be normalized already
	lightInfo.lightSource = calculateLightSource(ambientBrightness, directionalBrightness, lightsourcePosition);
	lightInfo.lightDirectionWorld = QVector3D(lightsourcePosition.v[0],lightsourcePosition.v[1],lightsourcePosition.v[2]);

	//if the night vision mode is on, use red-tinted lighting
	bool red=StelApp::getInstance().getVisionModeNight();

	//for now, lighting is only white
	lightInfo.ambient = QVector3D(ambientBrightness,ambientBrightness, ambientBrightness);
	lightInfo.directional = QVector3D(directionalBrightness,directionalBrightness,directionalBrightness);

	if(red)
	{
		lightInfo.ambient     = QVector3D(ambientBrightness,0,0);
		lightInfo.directional = QVector3D(directionalBrightness,0,0);
	}

}

Scenery3d::ShadowCaster  Scenery3d::calculateLightSource(float &ambientBrightness, float &directionalBrightness, Vec3f &lightsourcePosition)
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

    //GZ: this should not matter here, just to make OpenGL happy.
    lightsourcePosition.set(sunPosition.v[0], sunPosition.v[1], sunPosition.v[2]);
    directionalSourceString="(Sun, below horiz.)";

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
	if (shaderParameters.shadows) shadowcaster = Sun;
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
	    if (shaderParameters.shadows) shadowcaster = Moon;
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
	    if (shaderParameters.shadows) shadowcaster = Venus;
            directionalSourceString="Venus";
        } else directionalSourceString="(Venus, flooded by ambient)";
        //Alternately, construct a term around Venus brightness, like
        // directionalBrightness=(mag/-100)
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
	//setup projection matrix - this is a 90-degree quadratic perspective
	float zNear = 1.0f;
	float zFar = 10000.0f;
	projectionMatrix.setToIdentity();
	projectionMatrix.perspective(90.0f,1.0f,zNear,zFar);

	//set opengl viewport to the size of cubemap
	glViewport(0, 0, cubemapSize, cubemapSize);

	//set GL state - we want depth test + culling
	glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);

	glBindFramebuffer(GL_FRAMEBUFFER,cubeFBO);

	for(int i=0;i<6;++i)
	{
		modelViewMatrix = cubeRotation[i];
		modelViewMatrix.translate(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,cubeMapTex,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);\
		drawArrays(true,true);
	}

	//cubemap fbo must be released
	glBindFramebuffer(GL_FRAMEBUFFER,0);

	//reset GL state
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	//reset viewport (see StelPainter::setProjector)
	const Vec4i& vp = altAzProjector->getViewport();
	glViewport(vp[0], vp[1], vp[2], vp[3]);
}

void Scenery3d::drawFromCubeMap()
{
	//cube map drawing uses the StelPainter for correct warping (i.e. transforms happen on CPU side)
    StelPainter painter(altAzProjector);

    glEnable(GL_BLEND);
    //note that GL_ONE is required here for correct blending (see drawArrays)
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    //depth test and culling is necessary for correct display,
    //because the cube faces can be projected in quite "weird" ways
    glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    glClear(GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);

    //painter's color is multiplied with the texture
    painter.setColor(1.0f,1.0f,1.0f,1.0f);
    painter.enableCubeMap(true);
    glBindTexture(GL_TEXTURE_CUBE_MAP,cubeMapTex);

    //we have to simulate the behavoir of drawStelVertexArray ourselves, because we use float vertices
    //check if discontinuties exist
    //if(altAzProjector->hasDiscontinuity())
    //{
	    //TODO fix similar to arr.removeDiscontinuousTriangles
    //}

    painter.setArrays(cubeVertices.constData());

    //TODO the original vertex positions are static data, they should be stored in a VBO instead of uploaded each frame.
    // It would probably be best to reimplement this ourselves.
    painter.drawFromArray(StelPainter::Triangles,cubeVertices.count());

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void Scenery3d::drawDirect() // for Perspective Projection only!
{
    //calculate standard perspective projection matrix, use QMatrix4x4 for that
    float fov = altAzProjector->getFov();
    float aspect = (float)altAzProjector->getViewportWidth() / (float)altAzProjector->getViewportHeight();
    float zNear = 1.0f;
    float zFar = 10000.0f;
    float f = 2.0 / tan(fov * M_PI / 360.0);
    projectionMatrix = QMatrix4x4 (
		    f / aspect, 0, 0, 0,
		    0, f, 0, 0,
		    0, 0, (zFar + zNear) / (zNear - zFar), -1,
		    0, 0, 2.0 * zFar * zNear / (zNear - zFar), 0  );

    //calc modelview transform
    modelViewMatrix = convertToQMatrix( altAzProjector->getModelViewTransform()->getApproximateLinearTransfo() );
    modelViewMatrix.optimize(); //may make inversion faster?
    modelViewMatrix.translate(absolutePosition.v[0],absolutePosition.v[1],absolutePosition.v[2]);

    //depth test needs enabling, clear depth buffer, color buffer already contains background so it stays
    glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);

    //enable backface culling for increased performance
    glEnable(GL_CULL_FACE);

    //only 1 call needed here
    drawArrays(true);

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

}

void Scenery3d::drawWithCubeMap()
{
	generateCubeMap();
	drawFromCubeMap();
}

void Scenery3d::drawCoordinatesText()
{
    StelPainter painter(altAzProjector);
    painter.setFont(debugTextFont);
    painter.setColor(1.0f,0.0f,1.0f);
    float screen_x = altAzProjector->getViewportWidth()  - 240.0f;
    float screen_y = altAzProjector->getViewportHeight() -  60.0f;
    QString str;

    // model_pos is the observer position (camera eye position) in model-grid coordinates
    Vec3d model_pos=currentScene.zRotateMatrix*Vec3d(-absolutePosition.v[0], absolutePosition.v[1], -absolutePosition.v[2]);
    model_pos[1] *= -1.0;

    // world_pos is the observer position (camera eye position) in grid coordinates, e.g. Gauss-Krueger or UTM.
    Vec3d world_pos= model_pos + currentScene.modelWorldOffset;
    // problem: long grid names!
    painter.drawText(altAzProjector->getViewportWidth()-10-qMax(240, painter.getFontMetrics().boundingRect(currentScene.gridName).width()),
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

void Scenery3d::drawDebug()
{
    StelPainter painter(altAzProjector);
    painter.setFont(debugTextFont);
    painter.setColor(1,0,1,1);
    // For now, these messages print light mixture values.
    painter.drawText(20, 160, lightMessage);
    painter.drawText(20, 145, lightMessage2);
    painter.drawText(20, 130, lightMessage3);
    // PRINT OTHER MESSAGES HERE:

    float screen_x = altAzProjector->getViewportWidth()  - 500.0f;
    float screen_y = altAzProjector->getViewportHeight() - 300.0f;

    //Show some debug aids
    if(debugEnabled)
    {
	float debugTextureSize = 128.0f;
	float screen_x = altAzProjector->getViewportWidth() - debugTextureSize - 30;
	float screen_y = altAzProjector->getViewportHeight() - debugTextureSize - 30;

//	std::string cap = "Camera depth";
//	painter.drawText(screen_x-150, screen_y+130, QString(cap.c_str()));

//	glBindTexture(GL_TEXTURE_2D, camDepthTex);
//	painter.drawSprite2dMode(screen_x, screen_y, debugTextureSize);

	if(shaderParameters.shadows)
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

void Scenery3d::init()
{
	//init extensions
	glExtFuncs.init(QOpenGLContext::currentContext());

	OBJ::setupGL();

	//enable seamless cubemapping if HW supports it
	if(QOpenGLContext::currentContext()->hasExtension("GL_ARB_seamless_cube_map"))
	{
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		qDebug()<<"[Scenery3d] Seamless cubemap filtering enabled";
	}

	//check if GS cubemapping is possible
	if(QOpenGLContext::currentContext()->hasExtension("GL_ARB_geometry_shader4"))
	{
		this->supportsGSCubemapping = true;
		qDebug()<<"[Scenery3d] Geometry shader supported";
	}

	//shadow map init happens on first usage of shadows
}

void Scenery3d::deleteCubemapping()
{
	//delete cube map
	if(cubeFBO)
	{
		glDeleteFramebuffers(1,&cubeFBO);
		cubeFBO = 0;
	}

	if(cubeRB)
	{
		glDeleteRenderbuffers(1,&cubeRB);
		cubeRB = 0;
	}

	if(cubeMapTex)
	{
		glDeleteTextures(1,&cubeMapTex);
		cubeMapTex = 0;
	}

	if(cubeMapDepth)
	{
		glDeleteTextures(1,&cubeMapDepth);
		cubeMapDepth = 0;
	}
}

void Scenery3d::initCubemapping()
{
	qDebug()<<"[Scenery3d] Initializing cubemap...";
	deleteCubemapping();

	//TODO FS: Intel does not seem to like the "real" cubemap approach very	much. While it works without problems,
	// the performance is worse than with the 6 textured planes (17fps in rev. 4999 --> 7 fps in rev. 5000 for
	// Testscene in cubemap mode). On AMD, performance seems about the same, maybe a bit better than the old approach.
	// This could also be caused by the additional data that must be submitted to cubemap shader (the original vertex positions)
	// or the combined cube vertices maybe cause a GPU stall while transforming on CPU?
	// Will investigate possible options.

	//gen cube tex
	glGenTextures(1,&cubeMapTex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTex);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	bool isEs = QOpenGLContext::currentContext()->isOpenGLES();
	//create faces
	for (int i=0;i<6;++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,isEs ? GL_RGBA : GL_RGBA8,
			     cubemapSize,cubemapSize,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
	}


	//create fbo
	glGenFramebuffers(1,&cubeFBO);
	glBindFramebuffer(GL_FRAMEBUFFER,cubeFBO);

	if(supportsGSCubemapping && useGSCubemapping)
	{
		//use a cube map for depth, and attach all cube faces at once
		glExtFuncs.glFramebufferTextureARB(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,cubeMapTex,0);

		glGenTextures(1,&cubeMapDepth);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapDepth);
		//this all has probably not much effect on depth processing because we don't intend to sample
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		//create faces
		for (int i=0;i<6;++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,isEs ? GL_DEPTH_COMPONENT : GL_DEPTH_COMPONENT24,
				     cubemapSize,cubemapSize,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_BYTE,NULL);
		}

		//attach depth cubemap
		glExtFuncs.glFramebufferTextureARB(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, cubeMapDepth, 0);
	}
	else
	{
		//use a single depth renderbuffer & bind one cube face at a time

		//gen renderbuffer for single-face depth
		glGenRenderbuffers(1,&cubeRB);
		glBindRenderbuffer(GL_RENDERBUFFER,cubeRB);
		GLenum format = isEs ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT24;
		glRenderbufferStorage(GL_RENDERBUFFER, format,cubemapSize,cubemapSize);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, cubeRB);
		//temp binding
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_CUBE_MAP_POSITIVE_X,cubeMapTex,0);
	}

	//check validity
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		qWarning() << "[Scenery3D] glCheckFramebufferStatus failed, probably can't use cube map";
	}

	//unbind FB
	glBindFramebuffer(GL_FRAMEBUFFER,0);

	qDebug()<<"[Scenery3d] Initializing cubemap...done!";
}

void Scenery3d::deleteShadowmapping()
{
	if(shadowFBOs.size()>0) //kinda hack that finds out if shadowmap related objects have been created
	{
		//we can delete them all at once then
		glDeleteFramebuffers(shadowFBOs.size(),shadowFBOs.constData());
		glDeleteTextures(shadowMapsArray.size(),shadowMapsArray.constData());

		shadowFBOs.clear();
		shadowMapsArray.clear();
		shadowCPM.clear();
		frustumArray.clear();
		focusBodies.clear();

		qDebug()<<"[Scenery3d] Shadowmapping objects cleaned up";
	}
}

bool Scenery3d::initShadowmapping()
{
	deleteShadowmapping();

	bool valid = false;

	if(shadowmapSize>0)
	{
		//Define shadow maps array - holds MAXSPLITS textures
		shadowFBOs.resize(frustumSplits);
		shadowMapsArray.resize(frustumSplits);
		shadowCPM.resize(frustumSplits);
		frustumArray.resize(frustumSplits);
		focusBodies.resize(frustumSplits);

		//Query how many texture units we have at disposal
		GLint maxTex, maxComb;
		glGetIntegerv(GL_MAX_TEXTURE_COORDS, &maxTex);
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxComb);

		qDebug() << "Available texture units:" << std::max(maxTex, maxComb);

		//For shadowmapping, we use create 1 SM FBO for each frustum split - this seems to be the optimal solution on modern GPUs,
		//see http://www.reddit.com/r/opengl/comments/1rsnhy/most_efficient_fbo_usage_in_multipass_pipeline/
		//The point seems to be that switching attachments may cause re-validation of the FB.

		//Generate the FBO ourselves. We do this because Qt does not support depth-only FBOs to save some memory.
		glGenFramebuffers(frustumSplits,shadowFBOs.data());
		glGenTextures(frustumSplits,shadowMapsArray.data());

		for(int i=0; i<frustumSplits; i++)
		{
			//Bind the FBO
			glBindFramebuffer(GL_FRAMEBUFFER, shadowFBOs.at(i));

			//Activate the texture unit - we want sahdows + textures so this is crucial with the current Stellarium pipeline - we start at unit 3
			glActiveTexture(GL_TEXTURE3+i);

			//Bind the depth map and setup parameters
			glBindTexture(GL_TEXTURE_2D, shadowMapsArray.at(i));

			//initialize depth map, OpenGL ES 2 does require the OES_depth_texture extension, check for it maybe?
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowmapSize, shadowmapSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			//we use hardware-accelerated depth compare mode
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

			const float ones[] = {1.0f, 1.0f, 1.0f, 1.0f};
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, ones);
			//Attach the depthmap to the Buffer
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapsArray[i], 0);
			glDrawBuffer(GL_NONE); // essential for depth-only FBOs!!!
			glReadBuffer(GL_NONE);

			if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				qWarning() << "[Scenery3D] glCheckFramebufferStatus failed, can't use FBO";
				break;
			}
			else if (i==frustumSplits-1)
			{
				valid = true;
			}
		}

		//Done. Unbind and switch to normal texture unit 0
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glActiveTexture(GL_TEXTURE0);

		qDebug()<<"[Scenery3D] shadowmapping initialized";
	}
	else
	{
		qWarning()<<"[Scenery3D] shadowmapping not supported or disabled";
	}

	if(!valid)
	{
		parent->showMessage("Shadow mapping can not be used on your hardware, check logs for details");
	}
	return valid;
}

void Scenery3d::draw(StelCore* core)
{
	//cant draw if no models
	if(!hasModels)
		return;

	if(reinitCubemapping)
	{
		//init cubemaps
		initCubemapping();
		reinitCubemapping = false;
	}

	//update projector from core
	altAzProjector = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);

	//turn off blending, because it seems to be enabled somewhere we do not have access
	glDisable(GL_BLEND);

	//recalculate lighting info
	calculateLighting();

	if (shaderParameters.shadows)
	{
		if(!generateShadowMap())
			return;
	}
	else
	{
		//remove the shadow mapping stuff if not in use, this is only done once
		deleteShadowmapping();
	}

	if (core->getCurrentProjectionType() == StelCore::ProjectionPerspective)
	{
		//when Stellarium uses perspective projection we can use the fast direct method
		drawDirect();
	}
	else
	{
		//we have to use a workaround using cubemapping
		drawWithCubeMap();
	}
	if (textEnabled) drawCoordinatesText();
	if (debugEnabled)
	{
		drawDebug();
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
    Mat4f M(s[0], u[0], -f[0], 0.0f, s[1], u[1], -f[1], 0.0f, s[2], u[2], -f[2], 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    glMultMatrixf(M.r);
    glTranslated(-eyeX, -eyeY, -eyeZ);
}
