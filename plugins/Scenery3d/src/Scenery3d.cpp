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

#include <GLee.h>

#include <QAction>
#include <QString>
#include <QDebug>
#include <QSettings>
#include <stdexcept>
#include <cmath>


Scenery3d::Scenery3d()
    :rotation(0.0f)
{

}

Scenery3d::~Scenery3d()
{

}

void Scenery3d::load(const QSettings& scenery3dIni, const QString& scenery3dID)
{
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

    const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, core->getCurrentProjectionType());
    StelPainter painter(prj);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glClear(GL_DEPTH_BUFFER_BIT);

    Vec3d* vertice = new Vec3d[14 * 100];
    Vec3f* colors = new Vec3f[14 * 100];
    QVector<Vec3d> vVertice;

    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            double xPos = 5.0 + x * 3.0;
            double yPos = 5.0 + y * 3.0;
            for (int i = 0; i < 14; i++) {
                int idx = (x*10 + y) * 14 + i;
                Vec3d cv = cube_vertice[i];
                vertice[idx][0] = cv[0] + xPos;
                vertice[idx][1] = cv[1] + yPos;
                vertice[idx][2] = cv[2];
                vVertice << Vec3d(cv[0] + xPos, cv[1] + yPos, cv[2]);
                colors[idx] = cube_colors[i];
            }
        }
    }

    /*painter.setArrays(vertice, NULL, colors);
    for (int i = 0; i < 100; i++) {
        painter.drawFromArray(StelPainter::TriangleStrip, 14, i * 14);
    }*/
    painter.setColor(1.f, 1.f, 1.f);
    QVector<Vec3d> pv;
    for (int i = 0; i < 100; i++) {
        pv.clear();
        for (int j = 0; j < 14; j++) {
            pv << vVertice[i*14 + j];
        }
        StelVertexArray va(pv, StelVertexArray::TriangleStrip);
        painter.drawSphericalTriangles(va, false, NULL, true, 1000.);
    }

    delete[] vertice;
    delete[] colors;

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}
	
