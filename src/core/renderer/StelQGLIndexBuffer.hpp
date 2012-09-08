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

#ifndef _STELQGLINDEXBUFFER_HPP_
#define _STELQGLINDEXBUFFER_HPP_

#include <algorithm>
#include <QtOpenGL>
#include <QVector>
#include "StelIndexBuffer.hpp"


//! Qt-OpenGL index buffer implementation.
//!
//! Currently, this is a straghtforward index array.
//! In future, it should be replaced with a Qt3D implementation
//! (based on QGLIndexBuffer), or with a direct VBO implementation
//! (although we can still keep this implementation for GL1).
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
class StelQGLIndexBuffer : public StelIndexBuffer
{
// StelQGLRenderer constructs QGL index buffers.
friend class StelQGLRenderer;
// For optimized projection code.
friend class StelQGLArrayVertexBufferBackend;
friend class StelQGLInterleavedArrayVertexBufferBackend;
public:

	// Member functions used by the QGL backends

	//! Get a raw pointer to index data for OpenGL.
	const GLvoid* indices() const
	{
		Q_ASSERT_X(locked(), Q_FUNC_INFO,
		           "Trying to access raw data of an unlocked index buffer");
		if(indexType_ == IndexType_U16)      {return indices16.constData();}
		else if(indexType_ == IndexType_U32) {return indices32.constData();}
		Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown index type");
		// Prevents GCC from complaining about exiting a non-void function:
		return NULL;
	}

	//! Get the greatest index value.
	uint maxIndex() const
	{
		return maxIndex_;
	}
protected:
	// All bound checks are done in StelIndexBuffer.

	virtual void addIndex_(const uint index)
	{
		const int previousIndexCount = length();
		maxIndex_ = std::max(index, maxIndex_);
		if(previousIndexCount < indexCapacity())
		{
			// We have the capacity to store the index, so store it.
			// Parent addIndex increments index count.
			//
			// This is copied from setIndex_ for inlining 
			// (setIndex_ can't be inlined as it's virtual)
			if(indexType_ == IndexType_U16)      {indices16[previousIndexCount] = index;}
			else if(indexType_ == IndexType_U32) {indices32[previousIndexCount] = index;}
			else{Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown index type");}
			return;
		}
		if(indexType_ == IndexType_U16)      {indices16.append(index);}
		else if(indexType_ == IndexType_U32) {indices32.append(index);}
		else{Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown index type");}
	}

	virtual uint getIndex_(const int which) const
	{
		if(indexType_ == IndexType_U16)      {return indices16[which];}
		else if(indexType_ == IndexType_U32) {return indices32[which];}
		Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown index type");
		// Prevents GCC from complaining about exiting a non-void function:
		return -1;
	}

	virtual void setIndex_(const int which, const uint index)
	{
		if(indexType_ == IndexType_U16)      {indices16[which] = index;}
		else if(indexType_ == IndexType_U32) {indices32[which] = index;}
		else{Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown index type");}
	}

	// No need to do anything, length is zeroed in the parent class.
	virtual void clear_() {}

	// No need to do anything here until we use VBOs/QGLIndexBuffer
	virtual void lock_() {}
	virtual void unlock_() {}

private:
	// This is not very elegant, but not too wasteful compared to amount 
	// of data in the index buffer, and safe.
	
	//! Index storage when using 32bit indices.
	QVector<uint> indices32;

	//! Index storage when using 16bit indices.
	QVector<ushort> indices16;
	
	//! Construct a StelQGLIndexBuffer (only StelQGLRenderer can do this).
	StelQGLIndexBuffer(const IndexType indexType)
		: StelIndexBuffer(indexType)
		, maxIndex_(0)
	{}

	//! Maximum index value in the buffer.
	uint maxIndex_;

	//! Get number of indices we can hold without enlarging indices16/indices32.
	int indexCapacity() const
	{
		if(indexType_ == IndexType_U16)      {return indices16.size();}
		else if(indexType_ == IndexType_U32) {return indices32.size();}
		Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown index type");
		// Prevents GCC from complaining about exiting a non-void function:
		return -1;
	}
};

#endif // _STELQGLINDEXBUFFER_HPP_
