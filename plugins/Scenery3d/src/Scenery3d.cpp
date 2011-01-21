/*
 * TestPlugin
 * 
 * Copyright (C) 2010 Simon Parzer, Gustav Oberwandling, Peter Neubauer
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

#include <QAction>
#include <QString>
#include <QDebug>
#include <QSettings>
#include <stdexcept>
#include <cmath>

#define FBO_TEX_SIZE 2048

Scenery3d::Scenery3d()
    :core(NULL),
    absolutePosition(0.0, 0.0, 0.0),
    movement_x(0.0f), movement_y(0.0f), movement_z(0.0f),
    objModel(NULL)
{
    objModel = new OBJ();
    zRotateMatrix = Mat4d::identity();
    for (int i=0; i<6; i++) {
        cubeMap[i] = NULL;
    }
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
    delete objModel;
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
    objModelArrays = objModel->getStelArrays();

    // Rotate vertices around z axis
    for (unsigned int i=0; i<objModelArrays.size(); i++) {
        OBJ::StelModel& stelModel = objModelArrays[i];
        for (int v=0; v<stelModel.triangleCount*3; v++) {
            zRotateMatrix.transfo(stelModel.vertices[v]);
        }
    }
}

void Scenery3d::handleKeys(QKeyEvent* e)
{
    if (e->type() == QKeyEvent::KeyPress) {
        if (e->key() == Qt::Key_W) {
            movement_z = 1.0f;
            e->accept();
        } else if (e->key() == Qt::Key_X) {
            movement_z = -1.0f;
            e->accept();
        } else if (e->key() == Qt::Key_1) { // GZ: Bitte andere Tasten suchen!
            movement_x = 1.0f;
            e->accept();
        } else if (e->key() == Qt::Key_2) {
            movement_x = -1.0f;
            e->accept();
        } else if (e->key() == Qt::Key_3) {
            movement_y = 1.0f;
            e->accept();
        } else if (e->key() == Qt::Key_4) {
            movement_y = -1.0f;
            e->accept();
        }
    } else if (e->type() == QKeyEvent::KeyRelease) {
        if (e->key() == Qt::Key_W || e->key() == Qt::Key_X || e->key() == Qt::Key_1 || e->key() == Qt::Key_2 || e->key() == Qt::Key_3 || e->key() == Qt::Key_4 ) {
            movement_x = 0.0f;
            movement_y = 0.0f;
            movement_z = 0.0f;
            e->accept();
        }
    }
}

void Scenery3d::update(double deltaTime)
{
    if (core != NULL) {
        Vec3d viewDirection = core->getMovementMgr()->getViewDirectionJ2000();
        // GZ: Use correct coordinate frame!
        Vec3d viewDirectionAltAz=core->getNavigator()->j2000ToAltAz(viewDirection);
        double alt, az;
        StelUtils::rectToSphe(&az, &alt, viewDirectionAltAz);
        Vec3d move(movement_x * deltaTime * 3.0, movement_y * deltaTime * 3.0, movement_z * deltaTime * 3.0);
        absolutePosition += move;
    }
}

void Scenery3d::generateCubeMap_drawScene(StelPainter& painter)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (unsigned int i=0; i<objModelArrays.size(); i++) {
        OBJ::StelModel& stelModel = objModelArrays[i];
        if (stelModel.texture.data()) {
            stelModel.texture.data()->bind();
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glColor3fv(stelModel.color.v);
        painter.setArrays(stelModel.vertices, stelModel.texcoords, __null, stelModel.normals);
        painter.drawFromArray(StelPainter::Triangles, stelModel.triangleCount * 3, 0, false);
    }
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
            cubeMap[i] = new QGLFramebufferObject(FBO_TEX_SIZE, FBO_TEX_SIZE, QGLFramebufferObject::Depth, GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, cubeMap[i]->texture());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
    }

    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
    Vec3d sunPosition = ssystem->getSun()->getAltAzPos(core->getNavigator());
    zRotateMatrix.transfo(sunPosition);
    sunPosition.normalize();

    // We define the brigthness zero when the sun is 8 degrees below the horizon.
    float sinSunAngleRad = sin(qMin(M_PI_2, asin(sunPosition[2])+8.*M_PI/180.));
    float lightBrightness;
    if(sinSunAngleRad < -0.1/1.5 )
            lightBrightness = 0.01;
    else
            lightBrightness = (0.01 + 1.5*(sinSunAngleRad+0.1/1.5));
    const GLfloat LightAmbient[] = {0.33f, 0.33f, 0.33f, 1.0f};
    const GLfloat LightDiffuse[] = {lightBrightness, lightBrightness, lightBrightness, 1.0f};
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
    glViewport(0, 0, FBO_TEX_SIZE, FBO_TEX_SIZE);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMultMatrixd(projMatd);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glRotated(90.0f, -1.0f, 0.0f, 0.0f);

    glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);

    //front
    glPushMatrix();
    glTranslated(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
    cubeMap[0]->bind();
    glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
    generateCubeMap_drawScene(painter);
    cubeMap[0]->release();
    glPopMatrix();

    //right
    glPushMatrix();
    glRotated(90.0f, 0.0f, 0.0f, 1.0f);
    glTranslated(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
    cubeMap[1]->bind();
    glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
    generateCubeMap_drawScene(painter);
    cubeMap[1]->release();
    glPopMatrix();

    //left
    glPushMatrix();
    glRotated(90.0f, 0.0f, 0.0f, -1.0f);
    glTranslated(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
    cubeMap[2]->bind();
    glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
    generateCubeMap_drawScene(painter);
    cubeMap[2]->release();
    glPopMatrix();

    //back
    glPushMatrix();
    glRotated(180.0f, 0.0f, 0.0f, 1.0f);
    glTranslated(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
    cubeMap[3]->bind();
    glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
    generateCubeMap_drawScene(painter);
    cubeMap[3]->release();
    glPopMatrix();

    //top
    glPushMatrix();
    glRotated(90.0f, 1.0f, 0.0f, 0.0f);
    glTranslated(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
    cubeMap[4]->bind();
    glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
    generateCubeMap_drawScene(painter);
    cubeMap[4]->release();
    glPopMatrix();

    //bottom
    glPushMatrix();
    glRotated(90.0f, -1.0f, 0.0f, 0.0f);
    glTranslated(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
    cubeMap[5]->bind();
    glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
    generateCubeMap_drawScene(painter);
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
    glBindTexture(GL_TEXTURE_2D, cubeMap[0]->texture());
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

    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
    Vec3d sunPosition = ssystem->getSun()->getAltAzPos(core->getNavigator());
    zRotateMatrix.transfo(sunPosition);
    sunPosition.normalize();

    // We define the brigthness zero when the sun is 8 degrees below the horizon.
    float sinSunAngleRad = sin(qMin(M_PI_2, asin(sunPosition[2])+8.*M_PI/180.));
    float lightBrightness;
    if(sinSunAngleRad < -0.1/1.5 )
            lightBrightness = 0.01;
    else
            lightBrightness = (0.01 + 1.5*(sinSunAngleRad+0.1/1.5));
    const GLfloat LightAmbient[] = {0.33f, 0.33f, 0.33f, 1.0f};
    const GLfloat LightDiffuse[] = {lightBrightness, lightBrightness, lightBrightness, 1.0f};
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

    glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);

    for (unsigned int i=0; i<objModelArrays.size(); i++) {
        OBJ::StelModel& stelModel = objModelArrays[i];
        if (stelModel.texture.data()) {
            stelModel.texture.data()->bind();
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glColor3fv(stelModel.color.v);
        painter.setArrays(stelModel.vertices, stelModel.texcoords, __null, stelModel.normals);
        painter.drawFromArray(StelPainter::Triangles, stelModel.triangleCount * 3, 0, false);
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
	
void Scenery3d::draw(StelCore* core, bool useCubeMap)
{
    this->core = core;

    if (useCubeMap) {
        generateCubeMap(core);
        drawFromCubeMap(core);
    } else {
        drawObjModel(core);
    }
}
