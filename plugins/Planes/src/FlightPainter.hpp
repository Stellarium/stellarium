/*
 * Copyright (C) 2013 Felix Zeltner
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */



#ifndef FLIGHTPAINTER_HPP
#define FLIGHTPAINTER_HPP

#include <QOpenGlFunctions>
#include <QtCore>
#include "Flight.hpp"

class FlightPainter : protected QOpenGLFunctions
{
public:
    FlightPainter();

    void drawPath(const float *path, const QVector<Vec4d> &cols, double alpha, Flight::PathColour pathMode, int size, const StelProjectorP projector);

    static void initShaders();
    static void deinitShaders();

private:

    static QOpenGLShaderProgram *shaderProg;
    static int projLoc;
    static int sizeLoc;
    static int vertexArrayLoc;
    static int alphaLoc;
    static int colArrayLoc;
    static int colModeLoc;
};

#endif // FLIGHTPAINTER_HPP
