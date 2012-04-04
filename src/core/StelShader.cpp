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

static unsigned int loadShader(const char* path, unsigned int type);
static unsigned int createShader(const char *source, unsigned int type, const char *fileName = 0);
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

bool StelShader::load(const char *vertexFile, const char *pixelFile)
{
        if (!(vertexShader = loadShader(vertexFile, GL_VERTEX_SHADER)))
        {
                return false;
        }
        if (!(pixelShader = loadShader(pixelFile, GL_FRAGMENT_SHADER)))
        {
                return false;
        }
        if (!(program = createProgram(vertexShader, pixelShader)))
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

static unsigned int loadShader(const char* path, unsigned int type)
{
        FILE* fp;
        char* content;
        int size;
        unsigned int shader;

        if (!path)
        {
                return 0;
        }

        if (!(fp = fopen(path, "r")))
        {
                qWarning() << "Could not open shader " << path << ": " << strerror(errno);
                return 0;
        }

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        rewind(fp);

        if (!(content = (char*) malloc(size + 1)))
        {
                qWarning() << "failed to allocate memory";
                return 0;
        }

        fread(content, 1, size, fp);
        content[size] = 0;
        fclose(fp);

        shader = createShader(content, type, path);
        free(content);
        return shader;
}

static unsigned int createShader(const char *source, unsigned int type, const char *fileName) {
        unsigned int shader;
        int status, logLength;
        char infoLog[512];

        if (!fileName)
        {
                fileName = "unknown";
        }

        if (!source) {
                return 0;
        }

        shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, 0);

        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        glGetShaderInfoLog(shader, sizeof infoLog, &logLength, infoLog);

        if (status == GL_FALSE)
        {
                qWarning() << "Error while compiling " << fileName << ": " << infoLog;
                glDeleteShader(shader);
                return 0;
        }
        else if (logLength)
        {
                qWarning() << fileName << ": " << infoLog;
        }
        return shader;
}

static unsigned int createProgram(unsigned int vertexShader, unsigned int pixelShader)
{
        unsigned int program;
        int status, logLength;
        char infoLog[512];

        program = glCreateProgram();

        glAttachShader(program, vertexShader);
        glAttachShader(program, pixelShader);

        glLinkProgram(program);

        glGetProgramiv(program, GL_LINK_STATUS, &status);
        glGetProgramInfoLog(program, sizeof infoLog, &logLength, infoLog);

        if (status == GL_FALSE)
        {
                qWarning() << "Error while linking shader program:\n" << infoLog << "\n";
                glDeleteProgram(program);
                return 0;
        }
        else if (logLength)
        {
                qWarning() << "\n" << infoLog << "\n";
        }

        return program;
}
