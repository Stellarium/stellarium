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

#include "StelOpenGLArray.hpp"
#include "StelOBJ.hpp"

#include <QElapsedTimer>
#include <QOpenGLFunctions>

Q_LOGGING_CATEGORY(stelOpenGLArray,"stel.OpenGLArray")

QOpenGLFunctions* StelOpenGLArray::gl = Q_NULLPTR;
bool StelOpenGLArray::s_vaosSupported = false;
bool StelOpenGLArray::s_intElementBuffersSupported = false;

//static function
void StelOpenGLArray::initGL()
{
	QOpenGLContext* ctx = QOpenGLContext::currentContext();
	gl = ctx->functions();

	//disable VAOs on Intel because of serious bugs in their implemenation...
	QString vendor(reinterpret_cast<const char*>(gl->glGetString(GL_VENDOR)));
	if(vendor.contains("Intel",Qt::CaseInsensitive))
	{
		s_vaosSupported = false;
		qCDebug(stelOpenGLArray)<<"Disabling VAO usage because of Intel driver bugs";
	}
	else
	{
		//find out if VAOs are supported, simplest way is just trying to create one
		//Qt has the necessary checks inside the create method
		QOpenGLVertexArrayObject testObj;
		s_vaosSupported = testObj.create();
		testObj.destroy();
	}

	if( s_vaosSupported )
	{
		qCDebug(stelOpenGLArray)<<"Vertex Array Objects are supported";
	}
	else
	{
		qCDebug(stelOpenGLArray)<<"Vertex Array Objects are not supported on your hardware (this is not an error)";
	}

	//check if we can enable int index buffers
	if(ctx->isOpenGLES())
	{
		//query for extension
		if(ctx->hasExtension("GL_OES_element_index_uint"))
		{
			s_intElementBuffersSupported = true;
		}
	}
	else
	{
		//we are on Desktop, so int is always supported
		s_intElementBuffersSupported = true;
	}

	if(!s_intElementBuffersSupported)
	{
		qCWarning(stelOpenGLArray)<<"Your hardware does not support integer indices. Large models may not load.";
	}
}

StelOpenGLArray::StelOpenGLArray()
	: m_vertexBuffer(QOpenGLBuffer::VertexBuffer),
	  m_indexBuffer(QOpenGLBuffer::IndexBuffer),
	  m_indexBufferType(GL_UNSIGNED_INT),
	  m_indexBufferTypeSize(sizeof(GLuint)),
	  m_indexCount(0),
	  m_memoryUsage(0),
	  m_offsets(),
	  m_sizes(),
	  m_types(),
	  m_strides()
{
}

StelOpenGLArray::~StelOpenGLArray()
{
	//release is done by the Qt class destructors automatically
}

void StelOpenGLArray::clear()
{
	//QOpenGLBuffer has no good copy constructor/assignment operator
	//so we can't just do *this = StelOpenGLArray() here
	if(m_vao.isCreated())
		m_vao.destroy();
	m_vertexBuffer.destroy();
	m_indexBuffer.destroy();

	m_indexBufferType = GL_UNSIGNED_INT;
	m_indexBufferTypeSize = sizeof(GLuint);
	m_indexCount = 0;
	m_memoryUsage = 0;

	for (int i =0;i<ATTLOC_SIZE;++i)
	{
		m_offsets[i] = 0;
		m_sizes[i] = 0;
		m_types[i] = 0;
		m_strides[i] = 0;
	}
}

bool StelOpenGLArray::load(const StelOBJ* obj, bool useTangents)
{
	clear();

	QElapsedTimer timer;
	timer.start();

	if(s_vaosSupported)
	{
		//create vao, also already bind it to be core-profile compatible
		if(!m_vao.create())
		{
			qCCritical(stelOpenGLArray)<<"Could not create OpenGL vertex array object!";
			clear();
			return false;
		}
		m_vao.bind();
	}

	if(! (m_vertexBuffer.create() && m_indexBuffer.create()) )
	{
		qCCritical(stelOpenGLArray)<<"Could not create OpenGL buffers!";
		clear();
		return false;
	}

	if(m_vertexBuffer.bind())
	{
		//! The data is used for static drawing
		m_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
		//perform data upload
		const StelOBJ::VertexList& vertices = obj->getVertexList();
		if(useTangents)
		{
			//we can simply upload the whole vertex data in a single call
			m_vertexBuffer.allocate(vertices.constData(), static_cast<int>(sizeof(StelOBJ::Vertex)) * vertices.size());
			m_memoryUsage+=sizeof(StelOBJ::Vertex) * static_cast<size_t>(vertices.size());

			//setup vtx format
			for (int i = 0;i<=ATTLOC_BITANGENT;++i)
			{
				//all strides are the same, and only float is used
				m_strides[i] = sizeof(StelOBJ::Vertex);
				m_types[i] = GL_FLOAT;
			}
			m_offsets[ATTLOC_VERTEX] = offsetof(StelOBJ::Vertex,position);
			m_sizes[ATTLOC_VERTEX] = 3;
			m_offsets[ATTLOC_NORMAL] = offsetof(StelOBJ::Vertex,normal);
			m_sizes[ATTLOC_NORMAL] = 3;
			m_offsets[ATTLOC_TEXCOORD] = offsetof(StelOBJ::Vertex,texCoord);
			m_sizes[ATTLOC_TEXCOORD] = 2;
			m_offsets[ATTLOC_TANGENT] = offsetof(StelOBJ::Vertex,tangent);
			m_sizes[ATTLOC_TANGENT] = 4;
			m_offsets[ATTLOC_BITANGENT] = offsetof(StelOBJ::Vertex,bitangent);
			m_sizes[ATTLOC_BITANGENT] = 3;
		}
		else
		{
			//we have to convert to a simpler vertex format
			//we use the first 8 floats of each vertex
			const int vtxSize = sizeof(GLfloat) * 8;
			//alloc data but dont upload
			m_vertexBuffer.allocate(vertices.size() * vtxSize);
			m_memoryUsage+=static_cast<size_t>(vertices.size()) * vtxSize;
			for(int i =0;i<vertices.size();++i)
			{
				//copy the first 8 floats from each vertex
				m_vertexBuffer.write(i * vtxSize, &vertices.at(i), vtxSize);
			}
			for (int i = 0;i<=ATTLOC_NORMAL;++i)
			{
				//all strides are the same, and only float is used
				m_strides[i] = vtxSize;
				m_types[i] = GL_FLOAT;
			}
			m_offsets[ATTLOC_VERTEX] = offsetof(StelOBJ::Vertex,position);
			m_sizes[ATTLOC_VERTEX] = 3;
			m_offsets[ATTLOC_NORMAL] = offsetof(StelOBJ::Vertex,normal);
			m_sizes[ATTLOC_NORMAL] = 3;
			m_offsets[ATTLOC_TEXCOORD] = offsetof(StelOBJ::Vertex,texCoord);
			m_sizes[ATTLOC_TEXCOORD] = 2;
		}
		//dont forget to release
		m_vertexBuffer.release();
	}
	else
	{
		qCCritical(stelOpenGLArray)<<"Could not bind vertex buffer";
		clear();
		return false;
	}

	if(m_indexBuffer.bind())
	{
		m_indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
		//determine the index type to use
		if(obj->canUseShortIndices())
		{
			m_indexBufferType = GL_UNSIGNED_SHORT;
			m_indexBufferTypeSize = sizeof(GLushort);
			const StelOBJ::ShortIndexList idxList = obj->getShortIndexList();
			m_indexBuffer.allocate(idxList.constData(),static_cast<int>(m_indexBufferTypeSize) * idxList.size());
		}
		else
		{
			if(!s_intElementBuffersSupported)
			{
				qCCritical(stelOpenGLArray)<<"Can not use int element indices on this hardware, but StelOBJ requires it!";
				m_indexBuffer.release();
				clear();
				return false;
			}

			m_indexBufferType = GL_UNSIGNED_INT;
			m_indexBufferTypeSize = sizeof(GLuint);

			const StelOBJ::IndexList& idxList = obj->getIndexList();
			m_indexBuffer.allocate(idxList.constData(),static_cast<int>(m_indexBufferTypeSize) * idxList.size());
		}
		m_indexCount = obj->getIndexList().size();
		m_memoryUsage+= static_cast<size_t>(m_indexCount) * m_indexBufferTypeSize;
		m_indexBuffer.release();
	}
	else
	{
		qCCritical(stelOpenGLArray)<<"Could not bind index buffer";
		clear();
		return false;
	}

	if(m_vao.isCreated())
	{
		//let the VAO remember the bindBuffers state
		m_vao.bind();
		bindBuffers();
		m_vao.release();
		releaseBuffers();
	}

	qCDebug(stelOpenGLArray)<<"Loaded StelOBJ data into OpenGL in"<<timer.elapsed()<<"ms ("<<(m_memoryUsage / 1024.0f)<<"kb GL memory)";
	return true;
}

void StelOpenGLArray::bind()
{
	if(m_vao.isCreated())
		m_vao.bind(); //all we have to do
	else
		bindBuffers(); //we have to bind manually
}

void StelOpenGLArray::release()
{
	if(m_vao.isCreated())
		m_vao.release(); //all we have to do
	else
		releaseBuffers(); //we have to release manually
}

void StelOpenGLArray::bindBuffers()
{
	m_vertexBuffer.bind();

	// using Qt wrapper classes here is not possible because the only implementation is in QOpenGLShaderProgram, which requires a shader program instance
	// this is a bit incorrect, because the following is global state that does not depend on a shader
	// (but may be stored in a VAO to enable faster binding/unbinding)

	for(GLuint i = 0; i<ATTLOC_SIZE;++i)
	{
		if(m_sizes[i]>0) //zero size disables the location
		{
			gl->glEnableVertexAttribArray(i);
			gl->glVertexAttribPointer(i, m_sizes[i], m_types[i], GL_FALSE, m_strides[i], reinterpret_cast<const void *>(m_offsets[i]) );
		}
	}

	//vertex buffer does not need to remain bound, because the binding is stored by glVertexAttribPointer
	m_vertexBuffer.release();

	//index buffer must remain bound
	m_indexBuffer.bind();
}

void StelOpenGLArray::releaseBuffers()
{
	m_indexBuffer.release();

	for(GLuint i = 0; i<ATTLOC_SIZE;++i)
	{
		if(m_sizes[i]>0) //zero size disables the location
		{
			gl->glDisableVertexAttribArray(i);
		}
	}
}
