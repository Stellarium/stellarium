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

#ifndef STELSHADER_HPP
#define STELSHADER_HPP

//#include "VecMath.hpp"

//! @class StelShader
//! stores basic info about a glsl shader program

class StelShader {
public:
        StelShader();
        ~StelShader();

//! Sets the location of a uniform variable
        int uniformLocation (const char *name) const;

//! Sets the location of a vertex attribute
        int attributeLocation (const char *name) const;

//! Passes a uniform to the shader program
        void setUniform (int location, int value);
//! Passes a uniform to the shader program
        void setUniform (int location, int x, int y);
//! Passes a uniform to the shader program
        void setUniform (int location, int x, int y, int z);
//! Passes a uniform to the shader program
        void setUniform (int location, int x, int y, int z, int w);

//! Passes a uniform to the shader program
        void setUniform (int location, float value);
//! Passes a uniform to the shader program
        void setUniform (int location, float x, float y);
//! Passes a uniform to the shader program
        void setUniform (int location, float x, float y, float z);
//! Passes a uniform to the shader program
        void setUniform (int location, float x, float y, float z, float w);

//! Loads a vertex and a pixel Shader from the vertex and pixel shader files respectively.
//! @param const char* vertexFile, the path of the vertex shader
//! @param const char* pixelFile, the path of the fragment shader
//! @return true when both shader files are successfully loaded
        bool load(const char* vertexFile, const char* pixelFile);

//! Use current shader
//! @return true when the current shader program is successfully used
        bool use() const;

        friend bool useShader (const StelShader *shader);

private:
        unsigned int vertexShader;
        unsigned int pixelShader;
        unsigned int program;
};

bool useShader(const StelShader *shader);
#endif // STELSHADER_HPP
