/*
 * Stellarium
 * Copyright (C) 2016 Florian Schaukowitsch
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

#ifndef STELOPENGLARRAY_HPP
#define STELOPENGLARRAY_HPP

#include "VecMath.hpp"

#include <QLoggingCategory>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QVector>

class StelOBJ;
class QOpenGLFunctions;

Q_DECLARE_LOGGING_CATEGORY(stelOpenGLArray)

//! Encapsulates vertex data stored in the OpenGL server memory, which can be used for fast drawing commands,
//! without repeated CPU-GPU data uploads.
//! Each StelOpenGLArray uses a single vertex buffer for vertex data, an element index buffer,
//! and a vertex array object for faster binding if the hardware supports it.
class StelOpenGLArray
{
public:
	//! Specifies the attribute location used for the glVertexAttribPointer calls.
	//! Shaders should be configured to use these locations before linking,
	//! so that a single vertex format can be used for all shaders.
	enum AttributeLocation
	{
		//! This is the OpenGL attribute location where 3D vertex positions are mapped to
		ATTLOC_VERTEX,
		//! This is the OpenGL attribute location where 2D vertex texture coordinates are mapped to
		ATTLOC_TEXCOORD,
		//! This is the OpenGL attribute location where 3D vertex normals are mapped to
		ATTLOC_NORMAL,
		//! This is the OpenGL attribute location where vertex tangents are mapped to
		ATTLOC_TANGENT,
		//! This is the OpenGL attribute location where vertex bitangents are mapped to
		ATTLOC_BITANGENT,
		//! The attribute locations starting with this index are unused, and should be used for custom
		//! vertex attributes not supported by this class
		ATTLOC_SIZE
	};

	//! Constructs an empty StelOpenGLArray. No OpenGL objects are created with this call,
	//! so this can be used without a GL context.
	StelOpenGLArray();
	//! Releases the OpenGL data, if it has not happened yet.
	~StelOpenGLArray();

	//! Releases the OpenGL data. Requires a valid context.
	void clear();

	//! Loads the data from the given OBJ file into OpenGL buffers.
	//! The StelOBJ instance is not required after this constructor,
	//! the data has been copied to OpenGL.
	//!
	//! The data is stored in an interleaved format, i.e. the vertex buffer is
	//! a vector of StelOBJ::Vertex structs
	//! @return false if the data could not be loaded into GL
	//! @param obj The StelOBJ instance which data should be transferred to GL
	//! @param useTangents Whether to also load tangent/bitangent data, or skip it to save GL memory
	//! @note Requires a valid bound GL context
	bool load(const StelOBJ* obj, bool useTangents = true);

	//! Binds this array for drawing/manipulating with OpenGL
	//! @note Requires a valid bound GL context
	void bind();
	//! Releases the bound array data.
	//! @note Requires a valid bound GL context
	void release();

	//! Issues a glDrawElements call for all indices stored in this array.
	//! Does not bind the array first.
	inline void draw() const
	{
		draw(0,m_indexCount);
	}
	//! Issues a glDrawElements call for the \p count indices starting at \p offset
	//! Does not bind the array first.
	inline void draw(int offset, int count) const
	{
		gl->glDrawElements(GL_TRIANGLES, count, m_indexBufferType, reinterpret_cast<const GLvoid*>(static_cast<unsigned long long>(offset) * m_indexBufferTypeSize));
	}

	//! Returns the buffer used for the vertex data.
	const QOpenGLBuffer& getVertexBuffer() const { return m_vertexBuffer; }
	//! Returns the buffer used for the element index data.
	const QOpenGLBuffer& getIndexBuffer() const { return m_indexBuffer; }
	//! Returns the VAO which is used, when possible
	const QOpenGLVertexArrayObject& getVAO() const { return m_vao; }

	//! Returns the type used for element index data in this instance.
	//! This is either GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT or GL_UNSIGNED_INT (if hardware supports it).
	//! The actual type is chosen depending on the requirements of the data passed into load().
	GLenum getIndexBufferType() const { return m_indexBufferType; }
	//! Returns the size in bytes for the type from getIndexBufferType()
	size_t getIndexBufferTypeSize() const { return m_indexBufferTypeSize; }
	//! Returns the number of indices stored in this array
	int getIndexCount() const { return m_indexCount; }

	//! Initializes some global GL state. Requires a valid bound GL context.
	static void initGL();
	//! Returns true if the hardware supports use of
	//! [vertex array objects](https://www.opengl.org/wiki/Vertex_Specification#Vertex_Array_Object).
	//! These are used if possible to increase performance, because the driver
	//! does not need to re-validate the vertex attribute state for each draw call.
	//! @note The result of this function is valid only after calling initGL() in a GL context.
	static bool supportsVertexArrayObject() { return s_vaosSupported; }
	//! Returns true if the hardware supports use of (unsigned) integer element indices, instead of just byte and short.
	//! This is required for loading larger models (>65535 vertices) without manual splitting
	static bool supportsIntegerElementIndex() { return s_intElementBuffersSupported; }
private:
	QOpenGLBuffer m_vertexBuffer;
	QOpenGLBuffer m_indexBuffer;
	QOpenGLVertexArrayObject m_vao;
	GLenum m_indexBufferType;
	size_t m_indexBufferTypeSize;
	int m_indexCount;
	size_t m_memoryUsage;

	quintptr m_offsets[ATTLOC_SIZE];
	GLint m_sizes[ATTLOC_SIZE];
	GLenum m_types[ATTLOC_SIZE];
	GLsizei m_strides[ATTLOC_SIZE];

	void bindBuffers();
	void releaseBuffers();

	static QOpenGLFunctions* gl;
	static bool s_vaosSupported;
	static bool s_intElementBuffersSupported;
};

#endif // STELOPENGLARRAY_HPP
