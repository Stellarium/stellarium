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



#include "FlightPainter.hpp"
#include <QOpenGLShader>
#include "StelCore.hpp"
#include "StelApp.hpp"


QOpenGLShaderProgram *FlightPainter::shaderProg = NULL;
int FlightPainter::projLoc;
int FlightPainter::sizeLoc;
int FlightPainter::vertexArrayLoc;
int FlightPainter::alphaLoc;
int FlightPainter::colArrayLoc;
int FlightPainter::colModeLoc;


FlightPainter::FlightPainter()
{
}

void FlightPainter::drawPath(const float *path, const QVector<Vec4d> &cols, double alpha, Flight::PathColour pathMode, int size, const StelProjectorP projector)
{
    const Mat4f& m = projector->getProjectionMatrix();
    const QMatrix4x4 qMat(m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15]);
    shaderProg->bind();
    shaderProg->setAttributeArray(vertexArrayLoc, (const GLfloat *)path, 2);
    shaderProg->enableAttributeArray(vertexArrayLoc);
    shaderProg->setAttributeArray(colArrayLoc, (const GLfloat *)cols.constData(), 4);
    shaderProg->enableAttributeArray(colArrayLoc);
    shaderProg->setUniformValue(alphaLoc, (GLfloat)alpha);
    shaderProg->setUniformValue(sizeLoc, (GLfloat)size);
    shaderProg->setUniformValue(projLoc, qMat);
    if (pathMode == Flight::EncodeHeight) {
        shaderProg->setUniformValue(colModeLoc, 1);
    } else if (pathMode == Flight::EncodeVelocity) {
        shaderProg->setUniformValue(colModeLoc, 2);
    }

    glDrawArrays(GL_LINE_STRIP, 0, size);

    shaderProg->disableAttributeArray(vertexArrayLoc);
    shaderProg->disableAttributeArray(colArrayLoc);

    shaderProg->release();
}

void FlightPainter::initShaders()
{
    QOpenGLShader vShader(QOpenGLShader::Vertex);
    const char *vShaderSrc =
            "#version 130\n"
            "in mediump vec3 vertex;\n"
            "in mediump vec4 color;\n"
            "uniform mediump float alpha;\n"
            "uniform int mode;\n"
            "uniform mediump float size;\n"
            "uniform mediump mat4 proj;\n"
            "out mediump vec4 fragcolor;\n"
            "void main(void)\n"
            "{\n"
            "	gl_Position = proj * vec4(vertex, 1.);\n"
            " 	switch (mode) {\n"
            "		case 1: fragcolor = vec4(color.x, 0, 0, alpha); break;\n"
            " 		case 2: fragcolor = vec4(color.z, color.y, color.w, alpha); break;\n"
            "		default: float c = .2 + .8 * float(gl_VertexID) / size; fragcolor = vec4(c, c, c, alpha); break;\n"
            "	}\n"
            "}\n";

    vShader.compileSourceCode(vShaderSrc);
    if (!vShader.log().isEmpty()) {
        qWarning() << "Warnings while compiling vertex shader: " << vShader.log();
    }

    QOpenGLShader fShader(QOpenGLShader::Fragment);
    const char *fShaderSrc =
            "varying mediump vec4 fragcolor;\n"
            "void main(void)\n"
            "{\n"
                "gl_FragColor = fragcolor;\n"
            "}\n";

    fShader.compileSourceCode(fShaderSrc);
    if (!fShader.log().isEmpty()) {
        qWarning() << "Warnings while compiling vertex shader: " << fShader.log();
    }

    shaderProg = new QOpenGLShaderProgram(QOpenGLContext::currentContext());
    shaderProg->addShader(&vShader);
    shaderProg->addShader(&fShader);
    shaderProg->link();
    if (!shaderProg->log().isEmpty()) {
        qWarning() << "Warnings while linking FlightPainter shader: " << shaderProg->log();
    }

    projLoc = shaderProg->attributeLocation("proj");
    vertexArrayLoc = shaderProg->attributeLocation("vertex");
    colArrayLoc = shaderProg->attributeLocation("color");
    alphaLoc = shaderProg->uniformLocation("alpha");
    colModeLoc = shaderProg->uniformLocation("mode");
    sizeLoc = shaderProg->uniformLocation("size");
}

void FlightPainter::deinitShaders()
{
    delete shaderProg;
}
