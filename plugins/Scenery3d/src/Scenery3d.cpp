/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011 Simon Parzer, Peter Neubauer
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

#include <GLee.h>
#include "Scenery3d.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "StelFileMgr.hpp"
#include "Scenery3dMgr.hpp"
#include "StelNavigator.hpp"
#include "StelMovementMgr.hpp"
#include "StelUtils.hpp"
#include "SolarSystem.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"

#include <QAction>
#include <QString>
#include <QDebug>
#include <QSettings>
#include <stdexcept>
#include <cmath>

#define SHADOW_DEBUG 0

float Scenery3d::EYE_LEVEL = 1.0;
//const float Scenery3d::MOVE_SPEED = 4.0;
const float Scenery3d::MAX_SLOPE = 1.1;
const int Scenery3d::SHADOWMAP_SIZE = 4096;

Scenery3d::Scenery3d(int cbmSize)
    :core(NULL),
    absolutePosition(0.0, 0.0, 0.0),
    movement_x(0.0f), movement_y(0.0f), movement_z(0.0f),
    objModel(NULL)
{
    cubemapSize=cbmSize;
    objModel = new OBJ();
    groundModel = new OBJ();
    heightmap = NULL;
    zRotateMatrix = Mat4d::identity();
    shadowMapTexture = 0;
    for (int i=0; i<6; i++) {
        cubeMap[i] = NULL;
    }
    shadowMapFbo = NULL;
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
                cubePlane.vertex << v[i];
            }
            cubePlane.texCoords << Vec2f(tx0, ty0)
                                << Vec2f(tx1, ty0)
                                << Vec2f(tx1, ty1)
                                << Vec2f(tx0, ty0)
                                << Vec2f(tx1, ty1)
                                << Vec2f(tx0, ty1);
        }
    }
    shadowsEnabled = false;
    Mat4d matrix;
#define PLANE(var, mat) matrix=mat; var=StelVertexArray(cubePlane.vertex,StelVertexArray::Triangles,cubePlane.texCoords);\
                        for(int i=0;i<var.vertex.size();i++){ matrix.transfo(var.vertex[i]); }
    PLANE(cubePlaneRight, Mat4d::zrotation(-M_PI_2))
    PLANE(cubePlaneLeft, Mat4d::zrotation(M_PI_2))
    PLANE(cubePlaneBack, Mat4d::zrotation(M_PI))
    PLANE(cubePlaneTop, Mat4d::xrotation(-M_PI_2))
    PLANE(cubePlaneBottom, Mat4d::xrotation(M_PI_2))
#undef PLANE
}

Scenery3d::~Scenery3d()
{
    if (heightmap != NULL) {
        delete heightmap;
        heightmap = NULL;
    }
    delete objModel;
    delete groundModel;
	 if (shadowMapTexture != 0)
	 {
		 glDeleteTextures(1, &shadowMapTexture);
		 shadowMapTexture = 0;
		 delete shadowMapFbo;
	 }
    for (int i=0; i<6; i++) {
        if (cubeMap[i] != NULL) {
            delete cubeMap[i];
            cubeMap[i] = NULL;
        }
    }
}

void Scenery3d::loadConfig(const QSettings& scenery3dIni, const QString& scenery3dID)
{
    id = scenery3dID;
    name = scenery3dIni.value("model/name").toString();
    authorName = scenery3dIni.value("model/author").toString();
    description = scenery3dIni.value("model/description").toString();
    landscapeName = scenery3dIni.value("model/landscape").toString();
    modelSceneryFile = scenery3dIni.value("model/scenery").toString();
    modelGroundFile = scenery3dIni.value("model/ground").toString();

    orig_x = scenery3dIni.value("coord/orig_x").toReal();
    orig_y = scenery3dIni.value("coord/orig_y").toReal();
    orig_z = scenery3dIni.value("coord/orig_z").toReal();
    rot_z = scenery3dIni.value("coord/rot_z").toReal();

    zRotateMatrix = Mat4d::rotation(Vec3d(0.0, 0.0, 1.0), (90.0 + rot_z) * M_PI / 180.0);
}

void Scenery3d::loadModel()
{
	QString modelFile = StelFileMgr::findFile(Scenery3dMgr::MODULE_PATH + id + "/" + modelSceneryFile);
	qDebug() << "Trying to load OBJ model: " << modelFile;
	objModel->load(modelFile.toAscii());
        objModel->transform(zRotateMatrix);
	objModelArrays = objModel->getStelArrays();

	modelFile = StelFileMgr::findFile(Scenery3dMgr::MODULE_PATH + id + "/" + modelGroundFile);
	qDebug() << "Trying to load ground OBJ model: " << modelFile;
	groundModel->load(modelFile.toAscii());
        groundModel->transform(zRotateMatrix);


	// Rotate vertices around z axis
        /*for (unsigned int i=0; i<objModelArrays.size(); i++)
	{
		OBJ::StelModel& stelModel = objModelArrays[i];
		for (int v=0; v<stelModel.triangleCount*3; v++)
		{
			zRotateMatrix.transfo(stelModel.vertices[v]);
		}
        }*/

	heightmap = new Heightmap(*groundModel);

	absolutePosition[2] = minObserverHeight();
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
            case Qt::Key_Space: shadowsEnabled = !shadowsEnabled; e->accept(); break;
            case Qt::Key_PageUp:    movement_z = -1.0f * speedup; e->accept(); break;
            case Qt::Key_PageDown:  movement_z =  1.0f * speedup; e->accept(); break;
            case Qt::Key_Up:        movement_x = -1.0f * speedup; e->accept(); break;
            case Qt::Key_Down:      movement_x =  1.0f * speedup; e->accept(); break;
            case Qt::Key_Right:     movement_y = -1.0f * speedup; e->accept(); break;
            case Qt::Key_Left:      movement_y =  1.0f * speedup; e->accept(); break;
        }
    }
    else if ((e->type() == QKeyEvent::KeyRelease) && (e->modifiers() & Qt::ControlModifier))
    {
        if (e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown ||
            e->key() == Qt::Key_Up     || e->key() == Qt::Key_Down     ||
            e->key() == Qt::Key_Left   || e->key() == Qt::Key_Right     )
            {
                movement_x = 0.0f;
                movement_y = 0.0f;
                movement_z = 0.0f;
                e->accept();
            }
    }
}

void Scenery3d::update(double deltaTime)
{
    if (core != NULL)
    {
        StelMovementMgr *stelMovementMgr = GETSTELMODULE(StelMovementMgr);

        Vec3d viewDirection = core->getMovementMgr()->getViewDirectionJ2000();
        // GZ: Use correct coordinate frame!
        Vec3d viewDirectionAltAz=core->getNavigator()->j2000ToAltAz(viewDirection);
        double alt, az;
        StelUtils::rectToSphe(&az, &alt, viewDirectionAltAz);

        // GZ: Those are not needed, unless code below is activated.
        //Vec3d prevPosition = absolutePosition;
        // float prevHeight = prevPosition[2];

        // PN: (this was the old code)
        //Vec3d move(movement_x * deltaTime * MOVE_SPEED,
        //           movement_y * deltaTime * MOVE_SPEED,
        //           movement_z * deltaTime * MOVE_SPEED);

        // GZ:
        Vec3d move(( movement_x * std::cos(az) + movement_y * std::sin(az)),
                   ( movement_x * std::sin(az) - movement_y * std::cos(az)),
                   movement_z);
        move *= deltaTime * 0.01 * qMax(5.0, stelMovementMgr->getCurrentFov());

        absolutePosition.v[0] += move.v[0];
        absolutePosition.v[1] += move.v[1];
        EYE_LEVEL -= move.v[2];
        // GZ: This is enough.
        absolutePosition.v[2] = minObserverHeight();

        /*
        // PN: This is to prevent climbing too steep.
        // GZ: I commented this out for now, but leave as optional code, maybe reactivate later.
        float nextHeight = minObserverHeight();
        if ((prevHeight - nextHeight) > (deltaTime * MAX_SLOPE))
            {
                absolutePosition = prevPosition; // prevent climbing too high
            }
        else
            {
                absolutePosition.v[2] = minObserverHeight();
            }
        */
    }
}

float Scenery3d::minObserverHeight()
{
	if (heightmap == NULL)
	{
                return -EYE_LEVEL;
	}
	else
	{
		float x = -absolutePosition.v[0];
		float y = -absolutePosition.v[1];
		return -(heightmap->getHeight(x,y) + EYE_LEVEL);
	}
}

void Scenery3d::drawArrays(StelPainter& painter, bool textures)
{
    for (unsigned int i=0; i<objModelArrays.size(); i++) {
          OBJ::StelModel& stelModel = objModelArrays[i];
          if (stelModel.texture.data()) {
                if (textures) stelModel.texture.data()->bind();
          } else {
                if (textures) glBindTexture(GL_TEXTURE_2D, 0);
          }
          glColor3fv(stelModel.color.v);
          painter.setArrays(stelModel.vertices, stelModel.texcoords, __null, stelModel.normals);
          painter.drawFromArray(StelPainter::Triangles, stelModel.triangleCount * 3, 0, false);
    }
}

void Scenery3d::generateCubeMap_drawScene(StelPainter& painter, float lightBrightness)
{
    const GLfloat LightAmbient[] = {0.33f, 0.33f, 0.33f, 1.0f};
    const GLfloat LightDiffuse[] = {lightBrightness, lightBrightness, lightBrightness, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);
    drawArrays(painter);
}

void Scenery3d::generateCubeMap_drawSceneWithShadows(StelPainter& painter, float lightBrightness)
{
    const GLfloat LightAmbient[] = {0.33f, 0.33f, 0.33f, 1.0f};
    const GLfloat LightDiffuse[] = {lightBrightness, lightBrightness, lightBrightness, 1.0f};
    const GLfloat LightAmbientShadow[] = {0.02f, 0.02f, 0.02f, 1.0f};
    const GLfloat LightDiffuseShadow[] = {0.02f, 0.02f, 0.02f, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbientShadow);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuseShadow);
    drawArrays(painter);


    //Calculate texture matrix for projection
    //This matrix takes us from eye space to the light's clip space
    //It is postmultiplied by the inverse of the current view matrix when specifying texgen
    static Mat4f biasMatrix(0.5f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0.5f, 0.0f, 0.0f,
                         0.0f, 0.0f, 0.5f, 0.0f,
                         0.5f, 0.5f, 0.5f, 1.0f);	//bias from [-1, 1] to [0, 1]
    Mat4f textureMatrix = biasMatrix * lightProjectionMatrix * lightViewMatrix;

    Vec4f matrixRow[4];
    for (int i = 0; i < 4; i++)
    {
        matrixRow[i].set(textureMatrix[i+0], textureMatrix[i+4], textureMatrix[i+8], textureMatrix[i+12]);
    }

    //Set up texture coordinate generation.
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_S, GL_EYE_PLANE, matrixRow[0]);
    glEnable(GL_TEXTURE_GEN_S);

    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_T, GL_EYE_PLANE, matrixRow[1]);
    glEnable(GL_TEXTURE_GEN_T);

    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_R, GL_EYE_PLANE, matrixRow[2]);
    glEnable(GL_TEXTURE_GEN_R);

    glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_Q, GL_EYE_PLANE, matrixRow[3]);
    glEnable(GL_TEXTURE_GEN_Q);

    //Bind & enable shadow map texture
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);

    //Enable shadow comparison
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
    //Shadow comparison should be true (ie not in shadow) if r<=texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    //Shadow comparison should generate an INTENSITY result
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

    //Set alpha test to discard false comparisons
    glAlphaFunc(GL_GEQUAL, 0.99f);
    glEnable(GL_ALPHA_TEST);
    glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);
    drawArrays(painter, false); // draw without textures...

    // reset shadowmap texture parameters
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);
    glDisable(GL_TEXTURE_GEN_Q);

    glDisable(GL_ALPHA_TEST);
}

void Scenery3d::generateShadowMap(StelCore* core)
{
	if (objModelArrays.empty()) {
		return;
	}

	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, core->getCurrentProjectionType());
	StelPainter painter(prj);

	if (shadowMapTexture == 0) {
            shadowMapFbo = new QGLFramebufferObject(SHADOWMAP_SIZE, SHADOWMAP_SIZE, QGLFramebufferObject::Depth, GL_TEXTURE_2D);
            //Create the shadow map texture
            glGenTextures(1, &shadowMapTexture);
            glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOWMAP_SIZE, SHADOWMAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	}

        // Determine sun position
        SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
        Vec3d sunPosition = ssystem->getSun()->getAltAzPos(core->getNavigator());
        //zRotateMatrix.transfo(sunPosition);
        sunPosition.normalize();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT); // cull front faces
        glColorMask(0, 0, 0, 0); // disable color writes (increase performance?)

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, SHADOWMAP_SIZE, SHADOWMAP_SIZE);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	//glMultMatrixd(projMatd);
	const GLdouble orthoLeft = -50;
	const GLdouble orthoRight = 50;
	const GLdouble orthoBottom = -50;
	const GLdouble orthoTop = 50;
	glOrtho(orthoLeft, orthoRight, orthoBottom, orthoTop, -100, 100);
        glGetFloatv(GL_PROJECTION_MATRIX, lightProjectionMatrix); // save light projection for further render passes

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glRotated(90.0f, -1.0f, 0.0f, 0.0f);

	//front
	glPushMatrix();
	glLoadIdentity();
	gluLookAt (sunPosition[0], sunPosition[1], sunPosition[2], 0, 0, 0, 0, 0, 1);
	/* eyeX,eyeY,eyeZ,centerX,centerY,centerZ,upX,upY,upZ)*/
        glGetFloatv(GL_MODELVIEW_MATRIX, lightViewMatrix); // save light view for further render passes

	shadowMapFbo->bind();
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.1f, 4.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawArrays(painter);
	glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, SHADOWMAP_SIZE, SHADOWMAP_SIZE);
        glDisable(GL_POLYGON_OFFSET_FILL);
	shadowMapFbo->release();
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();

        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);
        glCullFace(GL_BACK);
        glDisable(GL_CULL_FACE);
        glColorMask(1, 1, 1, 1);
}

void Scenery3d::generateCubeMap(StelCore* core)
{
    if (objModelArrays.empty()) {
            return;
    }

    const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, core->getCurrentProjectionType());
    StelPainter painter(prj);

    for (int i=0; i<6; i++) {
        if (cubeMap[i] == NULL) {
            cubeMap[i] = new QGLFramebufferObject(cubemapSize, cubemapSize, QGLFramebufferObject::Depth, GL_TEXTURE_2D);
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

    SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
    Vec3d sunPosition = ssystem->getSun()->getAltAzPos(core->getNavigator());
    zRotateMatrix.transfo(sunPosition);
    sunPosition.normalize();

    // We define the brigthness zero when the sun is 8 degrees below the horizon.
    float sinSunAngleRad = sin(qMin(M_PI_2, asin(sunPosition[2])+8.*M_PI/180.));
    float lightBrightness;
    bool shadows = shadowsEnabled;
    if(sinSunAngleRad < -0.1/1.5 )
    {
        lightBrightness = 0.01;
        shadows = false;
    }
    else
    {
        lightBrightness = (0.01 + 1.5*(sinSunAngleRad+0.1/1.5));
    }

    const GLfloat LightPosition[] = {-sunPosition.v[0], -sunPosition.v[1], sunPosition.v[2], 0.0f}; // signs determined by experiment

    float fov = 90.0f;
    float aspect = 1.0f;
    float zNear = 1.0f;
    float zFar = 10000.0f;
    float f = 2.0 / tan(fov * M_PI / 360.0);
    Mat4d projMatd(f / aspect, 0, 0, 0,
                    0, f, 0, 0,
                    0, 0, (zFar + zNear) / (zNear - zFar), 2.0 * zFar * zNear / (zNear - zFar),
                    0, 0, -1, 0);

    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, cubemapSize, cubemapSize);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMultMatrixd(projMatd);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glRotated(90.0f, -1.0f, 0.0f, 0.0f);

    #define DRAW_SCENE  glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);\
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);\
                        if(shadows){generateCubeMap_drawSceneWithShadows(painter, lightBrightness);}\
                        else{generateCubeMap_drawScene(painter, lightBrightness);}

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
}

void Scenery3d::drawFromCubeMap(StelCore* core)
{
    if (objModelArrays.empty()) {
        return;
    }

    const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, core->getCurrentProjectionType());
    StelPainter painter(prj);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    glClear(GL_DEPTH_BUFFER_BIT);

    //front
#if SHADOW_DEBUG
        glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
#else
        glBindTexture(GL_TEXTURE_2D, cubeMap[0]->texture());
#endif
    painter.drawSphericalTriangles(cubePlane, true, __null, false);

    //right
    glBindTexture(GL_TEXTURE_2D, cubeMap[1]->texture());
    painter.drawSphericalTriangles(cubePlaneRight, true, __null, false);

    //left
    glBindTexture(GL_TEXTURE_2D, cubeMap[2]->texture());
    painter.drawSphericalTriangles(cubePlaneLeft, true, __null, false);

    //back
    glBindTexture(GL_TEXTURE_2D, cubeMap[3]->texture());
    painter.drawSphericalTriangles(cubePlaneBack, true, __null, false);

    //top
    glBindTexture(GL_TEXTURE_2D, cubeMap[4]->texture());
    painter.drawSphericalTriangles(cubePlaneTop, true, __null, false);

    //bottom
    glBindTexture(GL_TEXTURE_2D, cubeMap[5]->texture());
    painter.drawSphericalTriangles(cubePlaneBottom, true, __null, false);

    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    //glDisable(GL_BLEND);
}

void Scenery3d::drawObjModel(StelCore* core)
{
    if (objModelArrays.empty()) {
        return;
    }

    const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, core->getCurrentProjectionType());
    StelPainter painter(prj);

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

    SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
    Vec3d sunPosition = ssystem->getSun()->getAltAzPos(core->getNavigator());
    zRotateMatrix.transfo(sunPosition);
    sunPosition.normalize();

    // We define the brigthness zero when the sun is 8 degrees below the horizon.
    float sinSunAngleRad = sin(qMin(M_PI_2, asin(sunPosition[2])+8.*M_PI/180.));
    float lightBrightness;
    bool shadows = shadowsEnabled;
    if(sinSunAngleRad < -0.1/1.5 ) {
        lightBrightness = 0.01;
        shadows = false;
    }
    else
    {
        lightBrightness = (0.01 + 1.5*(sinSunAngleRad+0.1/1.5));
    }
    const GLfloat LightPosition[] = {-sunPosition.v[0], -sunPosition.v[1], sunPosition.v[2], 0.0f}; // signs determined by experiment

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

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMultMatrixd(projMatd);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMultMatrixd(prj->getModelViewMatrix());
    glTranslated(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);

    glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);

    if (shadows) {
        generateCubeMap_drawSceneWithShadows(painter, lightBrightness);
    } else {
        generateCubeMap_drawScene(painter, lightBrightness);
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
    if (objModelArrays.empty()) {
        return;
    }
    const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, core->getCurrentProjectionType());
    StelPainter painter(prj);
    const QFont font("sans-serif", 12);
    painter.setFont(font);
    float x = prj->getViewportWidth() - 250.0f;
    float y = prj->getViewportHeight() - 30.0f;
    QString s;
    s = QString("X %1m").arg(orig_x + absolutePosition.v[0]);
    painter.drawText(x, y, s);
    y -= 15.0f;
    s = QString("Y %1m").arg(orig_y + absolutePosition.v[1]);
    painter.drawText(x, y, s);
    y -= 15.0f;
    s = QString("Z %1m").arg(orig_z + absolutePosition.v[2]);
    painter.drawText(x, y, s);
}
	
void Scenery3d::draw(StelCore* core)
{
    this->core = core;

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
        generateCubeMap(core);
        drawFromCubeMap(core);
    }
}
