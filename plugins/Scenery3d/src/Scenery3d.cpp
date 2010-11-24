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
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelNavigator.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"

#include <GLee.h>

#include <QAction>
#include <QString>
#include <QDebug>
#include <stdexcept>
#include <cmath>

////////////////////////////////////////////////////////////////////////////////
// 
StelModule* Scenery3dStelPluginInterface::getStelModule() const
{
	return new Scenery3d();
}

StelPluginInfo Scenery3dStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	//Q_INIT_RESOURCE(TestPlugin);
	
	StelPluginInfo info;
	info.id = "Scenery3d";
	info.displayedName = "Scenery3d";
	info.authors = "Simon Parzer, Peter Neubauer";
	info.contact = "/dev/null";
	info.description = "Test scene renderer.";
	return info;
}

Q_EXPORT_PLUGIN2(Scenery3d, Scenery3dStelPluginInterface)


////////////////////////////////////////////////////////////////////////////////
// Constructor and destructor
Scenery3d::Scenery3d()
{
    setObjectName("Scenery3d");
}

Scenery3d::~Scenery3d()
{

}


////////////////////////////////////////////////////////////////////////////////
// Methods inherited from the StelModule class
// init(), update(), draw(), setStelStyle(), getCallOrder()

void Scenery3d::init()
{
    qDebug() << "Scenery3d::init()";
    font.setPixelSize(30);
    rotation = 0.0f;
}

void Scenery3d::deinit()
{
    qDebug() << "Scenery3d::deinit()";
}

void Scenery3d::update(double deltaTime)
{
    rotation += 8.0f * deltaTime;
}

void Scenery3d::draw(StelCore* core)
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

    const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, StelCore::ProjectionEqualArea);
    StelPainter painter(prj);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glClear(GL_DEPTH_BUFFER_BIT);

    Vec3d* vertice = new Vec3d[14 * 100];
    Vec3f* colors = new Vec3f[14 * 100];

    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            double xPos = -0.9 + x * 0.2;
            double yPos = -0.9 + y * 0.2;
            for (int i = 0; i < 14; i++) {
                int idx = (x*10 + y) * 14 + i;
                Vec3d cv = cube_vertice[i];
                vertice[idx][0] = cv[0] / 40.0 + xPos;
                vertice[idx][1] = cv[1] / 40.0 + yPos;
                vertice[idx][2] = cv[2] / 40.0;
                colors[idx] = cube_colors[i];
            }
        }
    }

    painter.setArrays(vertice, NULL, colors);
    for (int i = 0; i < 100; i++) {
        painter.drawFromArray(StelPainter::TriangleStrip, 14, i * 14);
    }

    delete[] vertice;
    delete[] colors;

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

/*void TestPlugin::draw(StelCore* core)
{
    const StelProjectorP prj = core->getProjection2d();
    StelPainter painter(prj);

    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glClear(GL_DEPTH_BUFFER_BIT);

    double aspect = (double)prj->getViewportWidth() / (double)prj->getViewportHeight();
    double f = 2.0 / tanf(45.0 * M_PI / 360.0);
    double zNear = 1.0;
    double zFar = 10000.0;
    Mat4f projectionMatrix( // should work like gluPerspective
            f / aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, (zFar + zNear) / (zNear - zFar), (2.0 * zFar * zNear) / (zNear - zFar),
            0, 0, -1.0, 0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMultMatrixf(projectionMatrix);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(1.5f, 0.0f, -7.0f);
    glRotatef(rotation, 1.0f, 1.0f, 1.0f);

    glBindTexture(GL_TEXTURE_2D, 0); // no texture
    glBegin(GL_QUADS);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f( 1.0f, 1.0f,-1.0f);
    glVertex3f(-1.0f, 1.0f,-1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glVertex3f( 1.0f, 1.0f, 1.0f);

    glColor3f(1.0f, 0.5f, 0.0f);
    glVertex3f( 1.0f,-1.0f, 1.0f);
    glVertex3f(-1.0f,-1.0f, 1.0f);
    glVertex3f(-1.0f,-1.0f,-1.0f);
    glVertex3f( 1.0f,-1.0f,-1.0f);

    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f( 1.0f, 1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glVertex3f(-1.0f,-1.0f, 1.0f);
    glVertex3f( 1.0f,-1.0f, 1.0f);

    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex3f( 1.0f,-1.0f,-1.0f);
    glVertex3f(-1.0f,-1.0f,-1.0f);
    glVertex3f(-1.0f, 1.0f,-1.0f);
    glVertex3f( 1.0f, 1.0f,-1.0f);

    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f,-1.0f);
    glVertex3f(-1.0f,-1.0f,-1.0f);
    glVertex3f(-1.0f,-1.0f, 1.0f);

    glColor3f(1.0f, 0.0f, 1.0f);
    glVertex3f( 1.0f, 1.0f,-1.0f);
    glVertex3f( 1.0f, 1.0f, 1.0f);
    glVertex3f( 1.0f,-1.0f, 1.0f);
    glVertex3f( 1.0f,-1.0f,-1.0f);
    glEnd();

    //const StelNavigator* nav = core->getNavigator();
    //Mat4d mat = nav->getAltAzModelViewMat();
    //painter.setProjector(core->getProjection(mat));
    //static const Vec3d vertexArray[] = {
        //Vec3d(0.25f, 0.0f, 0.1f),
        //Vec3d(0.25f, 0.25f, 0.1f),
        //Vec3d(0.0f, 0.25f, 0.1f),
    //};

    //painter.setArrays(vertexArray, NULL);
    //painter.drawFromArray(StelPainter::Triangles, 3);
    
    //Vec3f pos;
    //Vec3f xy;

    //pos.set(-1.f, 0.f, 0.f);
    //if (prj->project(pos, xy))
    //{
        //painter.drawText(xy[0], xy[1], "TestPlugin", 0., 0., 0.);
        //qDebug() << " X " << xy[0] << " Y " << xy[1];
    //}
    painter.drawRect2d(10, 10, 100, 100);

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}*/

//! Key events. stellarium-collada uses WASD for movement.
void Scenery3d::handleKeys (class QKeyEvent *e)
{
}
	
double Scenery3d::getCallOrder(StelModuleActionName actionName) const
{
    if (actionName == StelModule::ActionHandleKeys)
    {
        return -1.;
    }
    else if (actionName == StelModule::ActionDraw)
    {
        return 500.; // after landscape, etc, but before stel. GUI
    }
    return 0.;
}

bool Scenery3d::configureGui(bool show)
{
	return true;
}

