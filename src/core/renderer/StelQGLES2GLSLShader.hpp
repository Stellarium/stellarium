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

#ifndef _STELQGLES2GLSLSHADER_HPP_
#define _STELQGLES2GLSLSHADER_HPP_

#include "StelQGLGLSLShader.hpp"

//! QGL based StelGLSLShader implementation, used by the QGL2 renderer backend with ES2.
//! Main difference between this and StelQGLGLSLShader is that ES2 does not support linking
//! multiple shader programs of the same type, which is used heavily by StelQLGLSLShader.
//! Instead, this will just concatenate source files. Named ("optional") shaders will be
//! concatenated first; unnamed shaders will be added last, the assumption being that
//! optional shaders will contain support functions that will be referred to by main()
//!
//! If the optional shaders require a specific ordering, this will obviously fail; however,
//! Stellarium seems to mostly use this functionality to switch projections on and off.
//!
//! This is kind of brutal, and does require (precompiler conditional) changes to most of
//! the shaders to ensure they no longer contain function prototypes.
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
class StelQGLES2GLSLShader : public StelQGLGLSLShader
{
private:
    //! Adds an "enabled" flag and a hash (to allow us to build an ID for the programCache) to a shader source string
    struct OptionalShaderSource
    {
        //! Is the shader enabled?
        bool enabled;

        //! Hash of the name string
        uintptr_t hash;

        //! Optional shader source
        QString source;
    };

public:
    //! Construct a StelQGLES2GLSLShader owned by specified renderer.
    //!
    //! Used by QGLES2 renderer backend.
    //!
    //! @param renderer Renderer that created this shader.
    //! @param internal Is this shader internal to the renderer backend?
    //!                 (Not directly used by the user)
    StelQGLES2GLSLShader(class StelQGL2Renderer* renderer, bool internal);

    virtual bool addVertexShader(const QString& source);

    virtual bool addFragmentShader(const QString& source);

    virtual bool build();

    virtual bool addVertexShader(const QString& name, const QString& source);

    virtual bool hasVertexShader(const QString& name) const
    {
        return namedVertexShaderSources.contains(name);
    }

    virtual void enableVertexShader(const QString& name)
    {
        Q_ASSERT_X(namedVertexShaderSources.contains(name), Q_FUNC_INFO,
                   "Trying to enable a vertex shader with an unknown name");
        Q_ASSERT_X(state == State_Unlocked || state == State_Modified, Q_FUNC_INFO,
                   "Can't enable a vertex shader after the shader has been built");

        OptionalShaderSource& shader(namedVertexShaderSources[name]);
        if(shader.enabled) {return;}
        shader.enabled = true;
        state = State_Modified;
    }

    virtual void disableVertexShader(const QString& name)
    {
        Q_ASSERT_X(namedVertexShaderSources.contains(name), Q_FUNC_INFO,
                   "Trying to disable a vertex shader with an unknown name");
        Q_ASSERT_X(state == State_Unlocked || state == State_Modified, Q_FUNC_INFO,
                   "Can't disable a vertex shader after the shader has been built");

        OptionalShaderSource& shader(namedVertexShaderSources[name]);
        if(!shader.enabled) {return;}
        shader.enabled = false;
        state = State_Modified;
    }

protected:
    //! Vertex shaders that gets concatenated to the end of the enabled named shaders list
    QString defaultVertexShaderSource;

    //! Optional vertex shaders that may be enabled or disabled.
    //! The enabled sources are concatenated to one another, then concatenated to the defaultVertexShaderSource
    QMap<QString, OptionalShaderSource> namedVertexShaderSources;

    // There are no namedFragmentShaders, but they can be added if/when needed.
    //! Source code for a fragment shader
    QString defaultFragmentShaderSource;

    //! FNV-1a hash (http://isthe.com/chongo/tech/comp/fnv/#FNV-1a) to use as an id for the programCache.
    //! See programCache to see what this is used for.
    //! We need this in StelQGLES2GLSLShader because we can't compile individual shaders to
    //! mix-and-match, we need to compile the whole program in one go.
    uintptr_t CalculateStringHash(QString target);

private:

    //! Get the vertex program matching currently enabled shaders from cache.
    //!
    //! If there is no matching program, return NULL.
    //!
    //! @see programCache
    QGLShaderProgram* getProgramFromCache();
};

#endif // _STELQGLES2GLSLSHADER_HPP_
