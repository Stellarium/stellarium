/*
 * Stellarium
 * Copyright (C) 2011 Eleni Maria Stea
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

#include <QDebug>
#include <QString>

#include <QDir>
#include <QFile>

#include <GLee.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "StelShader.hpp"
#include "StelFileMgr.hpp"

static unsigned int loadShader(std::string path, unsigned int type);
static unsigned int createProgram(unsigned int vertexShader, unsigned int pixelShader);

StelShader::StelShader()
{
        vertexShader = pixelShader = program = 0;
}

StelShader::~StelShader()
{
        if (program)
        {
                if (vertexShader)
                {
                        glDeleteShader(vertexShader);
                }
                if (pixelShader)
                {
                        glDeleteShader(pixelShader);
                }
                glDeleteProgram(program);
        }
}

int StelShader::uniformLocation(const char *name) const
{
        if (!(use()))
        {
                return -1;
        }
        return glGetUniformLocation(program, name);
}

int StelShader::attributeLocation(const char *name) const
{
        if (!use())
        {
                return -1;
        }
        return glGetAttribLocation(program, name);
}

void StelShader::setUniform(int location, int value)
{
        if (!use() || location == -1)
        {
                return;
        }
        glUniform1i(location, value);
}

void StelShader::setUniform(int location, int x, int y)
{
        if(!use() || location == -1)
        {
                return;
        }
        glUniform2i(location, x, y);
}

void StelShader::setUniform(int location, int x, int y, int z)
{
        if(!use() || location == -1)
        {
                return;
        }
        glUniform3i(location, x, y, z);
}

void StelShader::setUniform(int location, int x, int y, int z, int w)
{
        if(!use() || location == -1)
        {
                return;
        }
        glUniform4i(location, x, y, z, w);
}

void StelShader::setUniform(int location, float value)
{
        if (!use() || location == -1)
        {
                return;
        }
        glUniform1f(location, value);
}

void StelShader::setUniform(int location, float x, float y)
{
       if(!use() || location == -1)
       {
               return;
       }
       glUniform2f(location, x, y);
}

void StelShader::setUniform(int location, float x, float y, float z)
{
        if (!use() || location == -1)
        {
                return;
        }
        glUniform3f(location, x, y, z);
}

void StelShader::setUniform(int location, float x, float y, float z, float w)
{
        if(!use() || location == -1)
        {
                return;
        }
        glUniform4f(location, x, y, z, w);
}

void StelShader::setUniform (int location, float mat[])
{
        if(!use() || location == -1)
        {
            return;
        }

        glUniformMatrix4fv(location, 1, GL_FALSE, mat);
}

bool StelShader::load(std::string vertexShaderPath, std::string fragmentShaderPath)
{
    if(!(vertexShader = loadShader(vertexShaderPath, GL_VERTEX_SHADER)))
    {
        return false;
    }

    if(!(pixelShader = loadShader(fragmentShaderPath, GL_FRAGMENT_SHADER)))
    {
        return false;
    }
    if(!(program = createProgram(vertexShader, pixelShader)))
    {
        return false;
    }
    return true;
}

bool StelShader::use() const
{
        if(program)
        {
                glUseProgram(program);
                return true;
        }
        return false;
}

bool useShader(const StelShader *shader)
{
        if(shader)
        {
                return shader->use();
        }
        glUseProgram(0);
        return true;
}

static unsigned int loadShader(std::string path, unsigned int type)
{
    int succeeded;

    std::ifstream shader_file(path.c_str(), std::ifstream::in);
    std::string shader_source = std::string(std::istreambuf_iterator<char>(shader_file), std::istreambuf_iterator<char>());

    // create handle
    unsigned int shader = glCreateShader(type);

    const char *shader_c_str = shader_source.c_str();
    glShaderSource(shader, 1, &shader_c_str, NULL);

    // compile shader
    glCompileShader(shader);

    // do error-checking
    glGetShaderiv(shader, GL_COMPILE_STATUS, &succeeded);

    if (!succeeded || !glIsShader(shader) )
    {
            // there was an error

            // get log-length
            int log_length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

            // get info-log
            std::string info_log(log_length, ' ');
            glGetShaderInfoLog(shader, log_length, NULL, &info_log[0]);

            // print info-log
            if(type == GL_VERTEX_SHADER)
            {
                qWarning() << "Vertex-shader compile error:\n\n";
            }
            else
            {
                qWarning() << "Fragment-shader compile error:\n\n";
            }

            qWarning() << info_log.c_str() << "\n";
    } else
    {
        if(type == GL_VERTEX_SHADER)
        {
            qWarning() << "Vertex-Shader loaded and compiled successfully.\n";
        }
        else
        {
            qWarning() << "Fragment-Shader loaded and compiled successfully.\n";
        }
    }

    return shader;
}

static unsigned int createProgram(unsigned int vertexShader, unsigned int pixelShader)
{
    int succeeded;

    // -- link shader-program --
    int shader_program = glCreateProgram();

    // attach shaders
    glAttachShader(shader_program, vertexShader);
    glAttachShader(shader_program, pixelShader);

    // link
    glLinkProgram(shader_program);

    // check for errors
    glGetProgramiv(shader_program, GL_LINK_STATUS, &succeeded);

    if (!succeeded || !glIsProgram(shader_program))
    {
            // there was an error

            // get log-length
            int log_length = 0;
            glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &log_length);

            // get info-log
            std::string info_log(log_length, ' ');
            glGetProgramInfoLog(shader_program, log_length, NULL, &info_log[0]);

            // print info-log
            qWarning() << "Shader-program link error:\n\n" << info_log.c_str() << "\n";
    } else
    {
            qWarning() << "Shader-program linked successfully.\n\n";
    }

    return shader_program;
}
