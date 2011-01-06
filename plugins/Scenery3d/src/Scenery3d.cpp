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


Scenery3d::Scenery3d()
    :core(NULL),
    absolutePosition(0.0, 0.0, 0.0),
    rotation(0.0f), movement_z(0.0f),
    objModel(NULL)
{
    objModel = new OBJ();
    zRotateMatrix = Mat4d::identity();
}

Scenery3d::~Scenery3d()
{
    delete objModel;
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
    for (int i=0; i<objModelArrays.size(); i++) {
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
    rotation += 8.0f * deltaTime;
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

void Scenery3d::drawObjModel(StelCore* core)
{
    if (objModelArrays.empty()) {
        return;
    }

    const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, core->getCurrentProjectionType());
    StelPainter painter(prj);

    //glEnable(GL_BLEND);
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
    //const GLfloat LightPosition[] = {0.0f, 1.0f, 0.0f, 0.0f};
    //const GLfloat LightPosition[] = {0.0f, 0.0f, 0.0f, 1.0f};

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

    //glRotated(this->rotation, 0.0, 1.0, 0.0);

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

void Scenery3d::drawCubeTestScene(StelCore* core)
{
    const Vec3d cube_vertice[] = {
        Vec3d( 1.0, 1.0, 1.0),
        Vec3d(-1.0, 1.0, 1.0),
        Vec3d( 1.0,-1.0, 1.0),
        Vec3d(-1.0,-1.0, 1.0),

        Vec3d(-1.0,-1.0,-1.0),

        Vec3d(-1.0, 1.0, 1.0),
        Vec3d(-1.0, 1.0,-1.0),

        Vec3d( 1.0, 1.0, 1.0),
        Vec3d( 1.0, 1.0,-1.0),

        Vec3d( 1.0,-1.0, 1.0),
        Vec3d( 1.0,-1.0,-1.0),

        Vec3d(-1.0,-1.0,-1.0),

        Vec3d( 1.0, 1.0,-1.0),
        Vec3d(-1.0, 1.0,-1.0),
    };

    const Vec3f cube_colors[] = {
        Vec3f(0.0f, 1.0f, 0.0f),
        Vec3f(0.0f, 1.0f, 0.0f),
        Vec3f(0.0f, 1.0f, 0.0f),
        Vec3f(0.0f, 1.0f, 0.0f),

        Vec3f(1.0f, 0.5f, 0.0f),

        Vec3f(1.0f, 0.0f, 0.0f),
        Vec3f(1.0f, 0.0f, 0.0f),

        Vec3f(1.0f, 1.0f, 0.0f),
        Vec3f(1.0f, 1.0f, 0.0f),

        Vec3f(0.0f, 0.0f, 1.0f),
        Vec3f(0.0f, 0.0f, 1.0f),

        Vec3f(1.0f, 0.5f, 0.0f),

        Vec3f(1.0f, 0.0f, 1.0f),
        Vec3f(1.0f, 0.0f, 1.0f),
    };

    Vec3d cube_vertice_triangles[36];
    Vec3f cube_colors_triangles[36];

    for (int i=2; i<14; i++) {
        int idx = i-2;
        int tri = idx*3;
        if ((i%2) == 0) {
            cube_vertice_triangles[tri] = cube_vertice[idx];
            cube_vertice_triangles[tri+1] = cube_vertice[idx+1];
            cube_vertice_triangles[tri+2] = cube_vertice[idx+2];
        } else {
            cube_vertice_triangles[tri] = cube_vertice[idx+1];
            cube_vertice_triangles[tri+1] = cube_vertice[idx];
            cube_vertice_triangles[tri+2] = cube_vertice[idx+2];
        }
        cube_colors_triangles[tri] = cube_colors_triangles[tri+1] =
                cube_colors_triangles[tri+2] = cube_colors[i];
    }

    const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, core->getCurrentProjectionType());
    StelPainter painter(prj);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glClear(GL_DEPTH_BUFFER_BIT);

    Vec3d* vertice = new Vec3d[36 * 100];
    Vec3f* colors = new Vec3f[36 * 100];

    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            double xPos = 3.0 + x * 3.0;
            double yPos = 3.0 + y * 3.0;
            for (int i = 0; i < 36; i++) {
                int idx = (x*10 + y) * 36 + i;
                Vec3d cv = cube_vertice_triangles[i];
                vertice[idx][0] = cv[0] + xPos;
                vertice[idx][1] = cv[1] + yPos;
                vertice[idx][2] = cv[2];
                colors[idx] = cube_colors_triangles[i];
            }
        }
    }

    //Vec3d nullv(0, 0, 0);
    //float fov = prj->getFov();
    float fov = prj->getFov();
    float aspect = (float)prj->getViewportWidth() / (float)prj->getViewportHeight();
    float zNear = 1.0f;
    float zFar = 10000.0f;
    float f = 2.0 / tan(fov * M_PI / 360.0);
    Mat4d projMatd(f / aspect, 0, 0, 0,
                   0, f, 0, 0,
                   0, 0, (zFar + zNear) / (zNear - zFar), 2.0 * zFar * zNear / (zNear - zFar),
                   0, 0, -1, 0);
    /*Mat4d mvpMat = projMatd * prj->getModelViewMatrix();
    const int num_triangles = 100*36 / 3;
    for (int i=0; i<num_triangles; i++) {
        int tri = i*3;
        bool valid = true;
        valid &= prj->projectInPlace(vertice[tri]);
        valid &= prj->projectInPlace(vertice[tri+1]);
        valid &= prj->projectInPlace(vertice[tri+2]);
        if (!valid) {
            vertice[tri] = vertice[tri+1] = vertice[tri+2] = nullv;
        }
        vertice[tri].transfo4d(mvpMat);
        prj->viewportTransform(vertice[tri]);
        vertice[tri+1].transfo4d(mvpMat);
        prj->viewportTransform(vertice[tri+1]);
        vertice[tri+2].transfo4d(mvpMat);
        prj->viewportTransform(vertice[tri+2]);
        //qDebug() << vertice[tri].toString();
    }*/


    /*Mat4f glViewMatrix;
    Mat4f glProjectionMatrix;
    glGetFloatv(GL_MODELVIEW_MATRIX, glViewMatrix.r);
    glGetFloatv(GL_PROJECTION_MATRIX, glProjectionMatrix.r);
    qDebug() << "view";
    glViewMatrix.print();
    qDebug() << "projection";
    glProjectionMatrix.print();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();*/

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMultMatrixd(projMatd);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMultMatrixd(prj->getModelViewMatrix());
    glTranslated(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);

    painter.setArrays(vertice, NULL, colors);
    for (int i = 0; i < 100; i++) {
        painter.drawFromArray(StelPainter::Triangles, 36, i * 36, false);
    }

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    /*painter.setColor(1.f, 1.f, 1.f);
    QVector<Vec3d> pv;
    for (int i = 0; i < 100; i++) {
        pv.clear();
        for (int j = 0; j < 14; j++) {
            pv << vVertice[i*14 + j];
        }
        StelVertexArray va(pv, StelVertexArray::TriangleStrip);
        painter.drawSphericalTriangles(va, false, NULL, true, 1000.);
    }*/

    delete[] vertice;
    delete[] colors;

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}
	
void Scenery3d::draw(StelCore* core)
{
    this->core = core;
    // for debug purposes
    //drawCubeTestScene(core);
    drawObjModel(core);
}
