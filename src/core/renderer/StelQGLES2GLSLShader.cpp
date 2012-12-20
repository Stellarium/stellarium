/*
 * Stellarium
 * Copyright (C) 2012 Ferdinand Majerech
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

#include "StelQGLES2GLSLShader.hpp"
#include "StelQGL2Renderer.hpp"

#include <stdint.h>

StelQGLES2GLSLShader::StelQGLES2GLSLShader(StelQGL2Renderer* renderer, bool internal)
    : StelQGLGLSLShader(renderer, internal)
{
    renderer->getStatistics()[SHADERS_CREATED] += 1.0;
    if(!addVertexShader("DefaultProjector",
        "vec4 project(in vec4 v){return v;}\n"))
    {
        qWarning() << "Failed to add default projection vertex shader: " << log();
    }
}

uintptr_t StelQGLES2GLSLShader::CalculateStringHash(QString target)
{
    //magic values care of http://isthe.com/chongo/tech/comp/fnv/#FNV-param
    //I assume I implemented this correctly...
    quint32 hash = 2166136261u;
    for(int i = 0; i < target.length(); ++i)
    {
        hash ^= target.at(i).toAscii();
        hash *= 16777619;
    }

    //using uintptr_t only because the super class already uses it for programCache
    return static_cast<uintptr_t>(hash);
}

bool StelQGLES2GLSLShader::addVertexShader(const QString& source)
{
    Q_ASSERT_X(state == State_Unlocked || state == State_Modified, Q_FUNC_INFO,
               "Can't add a vertex shader to a built shader program");

    defaultVertexShaderSource = source;
    programCache.clear(); //programCache is invalid now, because we've change a main source

    state = State_Modified;

    return true;
}

bool StelQGLES2GLSLShader::addVertexShader(const QString& name, const QString& source)
{
    Q_ASSERT_X(!namedVertexShaders.contains(name), Q_FUNC_INFO,
               "Trying to add a vertex shader with the same name twice");
    Q_ASSERT_X(state == State_Unlocked || state == State_Modified, Q_FUNC_INFO,
               "Can't add a vertex shader to a built shader program");

    OptionalShaderSource value;
    value.enabled = true;
    value.source = source;
    value.hash = CalculateStringHash(name);
    namedVertexShaderSources.insert(name, value);

    state = State_Modified;

    return true;
}

bool StelQGLES2GLSLShader::addFragmentShader(const QString& source)
{
    Q_ASSERT_X(state == State_Unlocked || state == State_Modified, Q_FUNC_INFO,
               "Can't add a fragment shader to a built shader program");

    defaultFragmentShaderSource = source;
    programCache.clear(); //programCache is invalid now, because we've change a main source

    state = State_Modified;

    return true;
}

QGLShaderProgram* StelQGLES2GLSLShader::getProgramFromCache()
{
    // Add up pointers to used shaders to get the ID.
    uintptr_t id = 0;
    foreach(OptionalShaderSource shader, namedVertexShaderSources)
    {
        id += shader.enabled ? shader.hash : 0;
    }

    // If no such program in cache, return NULL
    return programCache.value(id, NULL);
}

// See programCache doc comments to see how caching works here.
bool StelQGLES2GLSLShader::build()
{
    // Unlocked - i.e. not Modified
    if(state == State_Unlocked && program != NULL)
    {
        state = State_Built;
        return true;
    }

    QGLShaderProgram* cached = getProgramFromCache();
    // No matching program in cache, need to link a new program.
    if(cached == NULL)
    {
        uintptr_t id = 0;
        QGLShaderProgram* newProgram = new QGLShaderProgram(renderer->getGLContext());

        //Concatenate the vertex shader sources together (calculating the id as we do)
        QString shaderSource;
        foreach(OptionalShaderSource shader, namedVertexShaderSources)
        {
            if(shader.enabled)
            {
                shaderSource.append(shader.source);
                id += shader.hash;
            }
        }

        shaderSource.append(defaultVertexShaderSource);

        //Compile shaders
        QGLShader* vertexShader = new QGLShader(QGLShader::Vertex, renderer->getGLContext());
        QGLShader* fragmentShader = new QGLShader(QGLShader::Fragment, renderer->getGLContext());

        //Compile vertex shader
        aggregatedLog += vertexShader->log();

        if(!vertexShader->compileSourceCode(shaderSource)) {goto FAILED;}

        //Compile fragment shader
        aggregatedLog += fragmentShader->log();

        //ES 2 requires explicit precision statements
        shaderSource = "precision highp float;\n"
                        "precision highp int;\n";
        shaderSource += defaultFragmentShaderSource;

        if(!fragmentShader->compileSourceCode(shaderSource)) {goto FAILED;}

        //Add vertex and fragment shader to program
        if(!newProgram->addShader(vertexShader)) {goto FAILED;}
        if(!newProgram->addShader(fragmentShader)) {goto FAILED;}

        //Link program
        if(!newProgram->link()) {goto FAILED;}

        //DONE
        aggregatedLog += "Built successfully";
        // Add the program to the cache, and set the current program to it.
        programCache.insert(id, newProgram);
        program = newProgram;
        state = State_Built;
        return true;

FAILED:
        aggregatedLog += newProgram->log();
        delete newProgram;
        return false;
    }

    program = cached;
    state = State_Built;
    return true;
}
