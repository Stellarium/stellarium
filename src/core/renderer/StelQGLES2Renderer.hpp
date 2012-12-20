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

#ifndef _STELQGLES2RENDERER_HPP_
#define _STELQGLES2RENDERER_HPP_

#include "StelQGL2Renderer.hpp"
#include "StelQGLES2GLSLShader.hpp"

//! Renderer backend using OpenGL ES 2.0
//! similar to (and thus derivative of) the OpenGL 2 renderer
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
class StelQGLES2Renderer : public StelQGL2Renderer
{
public:
    //! Construct a StelQGL2Renderer.
    //!
    //! @param parent       Parent widget for the renderer's GL widget.
    //! @param pvrSupported Are .pvr (PVRTC - PowerVR hardware) textures supported on this platform?
    //!                     This should be true for mobile platforms with PowerVR GPUs.
    StelQGLES2Renderer(QGraphicsView* parent, bool pvrSupported)
        : StelQGL2Renderer(parent, pvrSupported)
    {
    }

    //! Overridden to use StelQGLES2GLSLShader
    virtual StelGLSLShader* createGLSLShader()
    {
        return new StelQGLES2GLSLShader(this, false);
    }

    virtual bool init()
    {
        Q_ASSERT_X(!initialized, Q_FUNC_INFO,
                   "StelQGL2Renderer is already initialized");

        // Using  this instead of makeGLContextCurrent() to avoid invariant
        // as we're not in valid state (getGLContext() isn't public - it doesn't call invariant)
        getGLContext()->makeCurrent();

        // GL2 drawing doesn't work with GL1 paint engine .
        //
        // Not checking for GL2 as there could also be e.g. X11, or GL3 in future, etc;
        // where GL2 might work fine.
        if(qtPaintEngineType() == QPaintEngine::OpenGL)
        {
            qWarning() << "StelQGLES2Renderer::init : Failed because Qt paint engine is not OpenGL2 \n"
                          "If paint engine is OpenGL3 or higher, this code needs to be updated";
            return false;
        }

        if(!QGLFormat::openGLVersionFlags().testFlag(QGLFormat::OpenGL_ES_Version_2_0))
        {
            qWarning() << "StelQGLES2Renderer::init : StelQGLES2Renderer needs OpenGL ES 2.0";
            return false;
        }

        // Check for features required for the GL2 backend.
        if(!gl.hasOpenGLFeature(QGLFunctions::Shaders))
        {
            qWarning() << "StelQGLES2Renderer::init : Required feature not supported: shaders";
            return false;
        }
        if(!gl.hasOpenGLFeature(QGLFunctions::Buffers))
        {
            qWarning() << "StelQGLES2Renderer::init : Required feature not supported: VBOs/IBOs";
            return false;
        }
        // We don't want to check for multitexturing everywhere.
        // Also, GL2 requires multitexturing so this shouldn't even happen.
        // (GL1 works fine without multitexturing)
        if(!gl.hasOpenGLFeature(QGLFunctions::Multitexture))
        {
            qWarning() << "StelQGLES2Renderer::init : Required feature not supported: Multitexturing";
            return false;
        }

        if(!StelQGLRenderer::init())
        {
            qWarning() << "StelQGLES2Renderer::init : StelQGLRenderer init failed";
            return false;
        }

        // Each shader here handles a specific combination of vertex attribute
        // interpretations. E.g. vertex-color-texcoord .

        // Notes about uniform/attribute names.
        //
        // Projection matrix is always specified (and must be declared in the shader),
        // as "uniform mat4 projectionMatrix"
        //
        // For shaders that have no vertex color attribute,
        // "uniform vec4 globalColor" must be declared.
        //
        // Furthermore, each vertex attribute interpretation has its name that
        // must be used. These are specified in attributeGLSLName function in
        // StelGLUtilityFunctions.cpp . These are:
        //
        // - "vertex" for vertex position
        // - "texCoord" for texture coordinate
        // - "normal" for vertex normal
        // - "color" for vertex color
        //
        // GLSLShader can also require unprojected position attribute,
        // "unprojectedVertex", but this is only specified if the shader requires it.
        //

        // All vertices have the same color.
        plainShader = loadBuiltinShader
        (
            "plainShader"
            ,
            "attribute mediump vec4 vertex;\n"
            "uniform mediump mat4 projectionMatrix;\n"
            "void main(void)\n"
            "{\n"
            "    gl_Position = projectionMatrix * project(vertex);\n"
            "}\n"
            ,
            "uniform mediump vec4 globalColor;\n"
            "void main(void)\n"
            "{\n"
            "    gl_FragColor = globalColor;\n"
            "}\n"
        );
        if(NULL == plainShader)
        {
            qWarning() << "StelQGL2Renderer::init failed to load plain shader";
            return false;
        }
        builtinShaders.append(plainShader);

        // Vertices with per-vertex color.
        colorShader = loadBuiltinShader
        (
            "colorShader"
            ,
            "attribute highp vec4 vertex;\n"
            "attribute mediump vec4 color;\n"
            "uniform mediump mat4 projectionMatrix;\n"
            "varying mediump vec4 outColor;\n"
            "void main(void)\n"
            "{\n"
            "    outColor = color;\n"
            "    gl_Position = projectionMatrix * project(vertex);\n"
            "}\n"
            ,
            "varying mediump vec4 outColor;\n"
            "void main(void)\n"
            "{\n"
            "    gl_FragColor = outColor;\n"
            "}\n"
        );
        if(NULL == colorShader)
        {
            qWarning() << "StelQGL2Renderer::init failed to load color shader program";
            return false;
        }
        builtinShaders.append(colorShader);

        // Textured mesh.
        textureShader = loadBuiltinShader
        (
            "textureShader"
            ,
            "attribute highp vec4 vertex;\n"
            "attribute mediump vec2 texCoord;\n"
            "uniform mediump mat4 projectionMatrix;\n"
            "varying mediump vec2 texc;\n"
            "void main(void)\n"
            "{\n"
            "    gl_Position = projectionMatrix * project(vertex);\n"
            "    texc = texCoord;\n"
            "}\n"
            ,
            "varying mediump vec2 texc;\n"
            "uniform sampler2D tex;\n"
            "uniform mediump vec4 globalColor;\n"
            "void main(void)\n"
            "{\n"
            "    gl_FragColor = texture2D(tex, texc) * globalColor;\n"
            "}\n"
        );
        if(NULL == textureShader)
        {
            qWarning() << "StelQGL2Renderer::init failed to load texture shader";
            return false;
        }
        builtinShaders.append(textureShader);

        // Textured mesh interpolated with per-vertex color.
        colorTextureShader = loadBuiltinShader
        (
            "colorTextureShader"
            ,
            "attribute highp vec4 vertex;\n"
            "attribute mediump vec2 texCoord;\n"
            "attribute mediump vec4 color;\n"
            "uniform mediump mat4 projectionMatrix;\n"
            "varying mediump vec2 texc;\n"
            "varying mediump vec4 outColor;\n"
            "void main(void)\n"
            "{\n"
            "    gl_Position = projectionMatrix * project(vertex);\n"
            "    texc = texCoord;\n"
            "    outColor = color;\n"
            "}\n"
            ,
            "varying mediump vec2 texc;\n"
            "varying mediump vec4 outColor;\n"
            "uniform sampler2D tex;\n"
            "void main(void)\n"
            "{\n"
            "    gl_FragColor = texture2D(tex, texc) * outColor;\n"
            "}\n"
        );
        if(NULL == colorTextureShader)
        {
            qWarning() << "StelQGL2Renderer::init failed to load colorTexture shader";
            return false;
        }
        builtinShaders.append(colorTextureShader);

        // Float texture support blacklist
        // (mainly open source drivers, which don't support them due to patents on
        //  float textures)
        floatTexturesDisabled =
            glVendorString == "nouveau"                || // Open source NVidia drivers
            glVendorString == "DRI R300 Project"       || // Open source ATI R300 (Radeon 9000, X000 and X1000)
            glVendorString == "Intel"                  || // Intel drivers
            glVendorString == "Mesa Project"           || // Mesa software rasterizer
            glVendorString == "Microsoft Corporation"  || // Microsoft builtin GL (this doesn't even support GL2, though)
            glVendorString == "Tungsten Graphics, Inc" || // Some Gallium3D drivers
            glVendorString == "X.Org R300 Project"     || // More open source ATI R300
            glVendorString == "X.Org"                  || // Gallium3D on AMD GPUs
            glVendorString == "VMware, Inc."              // More Gallium3D, mainly llvmpipe, softpipe
            ;

        initialized = true;
        invariant();
        return true;
    }

private:
    //! Load a builtin shader from specified source.
    //! Overridden to use StelQGLES2GLSLShader
    //!
    //! @param name Name used for debugging.
    //! @param vSrc Vertex shader source.
    //! @param fSrc Fragment shader source.
    //!
    //! @return Complete shader program at success, NULL on compiling or linking error.
    StelQGLGLSLShader *loadBuiltinShader(const char* const name,
                                         const char* const vSrc, const char* const fSrc)
    {
        // No invariants, as this is called from init - before the Renderer is
        // in fully valid state.
        StelQGLGLSLShader* result =
            dynamic_cast<StelQGLGLSLShader*>(new StelQGLES2GLSLShader(this, true));

        // Compile and add vertex shader.
        if(!result->addVertexShader(QString(vSrc)))
        {
            qWarning() << "Failed to compile vertex shader of builtin shader \""
                       << name << "\" : " << result->log();
            delete result;
            return NULL;
        }
        // Compile and add fragment shader.
        if(!result->addFragmentShader(QString(fSrc)))
        {
            qWarning() << "Failed to compile fragment shader of builtin shader \""
                       << name << "\" : " << result->log();
            delete result;
            return NULL;
        }
        // Link the shader program.
        if(!result->build())
        {
            qWarning() << "Failed to link builtin shader \""
                       << name << "\" : " << result->log();
            delete result;
            return NULL;
        }
        if(!result->log().isEmpty())
        {
            qWarning() << "Log of the compilation of builtin shader \"" << name
                       << "\" :" << result->log();
        }

        return result;
    }
};

#endif // _STELQGLES2RENDERER_HPP_
