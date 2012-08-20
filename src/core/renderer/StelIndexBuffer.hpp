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

#ifndef _STELINDEXBUFFER_HPP_
#define _STELINDEXBUFFER_HPP_


#include <QtGlobal>


//! Data types of indices.
enum IndexType
{
	//! 16-bit indices. Can be used with at most 65536 vertices in a vertex buffer.
	//!
	//! This uses less memory and is more efficient (especially on older GPUs).
	//! Sufficient for most uses, but for buffers that might be too large 
	//! 32bit should be used.
	IndexType_U16,
	//! 32-bit indices. Should be used for buffers that might have more than 65536 vertices.
	IndexType_U32
};

//! Generic index buffer interface usable with all Renderer backends.
//!
//! Used to specify order in which vertices from a vertex buffer are drawn
//! (allowing to e.g. draw the same vertex in multiple triangles without 
//! duplicating it, saving memory and RAM-VRAM bandwidth).
class StelIndexBuffer
{
public:
	//! Virtual destructor.
	virtual ~StelIndexBuffer(){}
	
	//! Add a new index to the end of the buffer.
	//!
	//! The buffer must not be locked.
	//!
	//! @param index Index to add.
	void addIndex(const uint index)
	{
		Q_ASSERT_X(!locked_, Q_FUNC_INFO,
		           "Trying to set an index in a locked index buffer");
		Q_ASSERT_X(indexType_ == IndexType_U32 || index < 65536, Q_FUNC_INFO,
		           "Trying to add a 16-bit index with value greater than 65536");

		addIndex_(index);
		++indexCount_;
	}

	//! Return index at specified position in the buffer.
	//!
	//! The buffer must not be locked.
	//!
	//! @param which Position of the index to get.
	//! @return Index at specified position.
	uint getIndex(const int which) const 
	{
		Q_ASSERT_X(!locked_, Q_FUNC_INFO,
		           "Trying to get an index from a locked index buffer");
		Q_ASSERT_X(which < indexCount_, Q_FUNC_INFO, 
		           "Index to an index buffer element out of bounds");
		return getIndex_(which);
	}

	//! Set specified index in the buffer.
	//!
	//! The buffer must not be locked.
	//!
	//! @param which Position of the index to set.
	//! @param index Value to set the index to.
	void setIndex(const int which, const uint index)
	{
		Q_ASSERT_X(!locked_, Q_FUNC_INFO,
		           "Trying to set an index in a locked index buffer");
		Q_ASSERT_X(which < indexCount_, Q_FUNC_INFO, 
		           "Index to an index buffer element out of bounds");
		Q_ASSERT_X(indexType_ == IndexType_U32 || index < 65536, Q_FUNC_INFO,
		           "Trying to set a 16-bit index to a value greater than 65536");
		setIndex_(which, index);
	}

	//! Clear the buffer, removing all indices.
	//!
	//! Can only be called when unlocked.
	//!
	//! The backend might reuse previously allocated storage after 
	//! clearing, so calling clear() might be more efficient than destroying 
	//! a buffer and then constructing a new one.
	void clear()
	{
		Q_ASSERT_X(!locked_, Q_FUNC_INFO, "Trying to clear a locked index buffer");
		clear_();
		indexCount_ = 0;
	}

	//! Lock the buffer. Must be called before drawing.
	void lock()
	{
		locked_ = true;
		lock_();
	}

	//! Unlock the buffer. Must be called to modify the buffer after drawing.
	void unlock()
	{
		unlock_();
		locked_ = false;
	}

	//! Is this buffer locked?
	bool locked() const 
	{
		return locked_;
	}

	//! Get type of indices (16bit or 32bit)
	IndexType indexType() const
	{
		return indexType_;
	}

	//! Returns the number of indices in the buffer.
	int length() const
	{
		return indexCount_;
	}

protected:
	//! Index type used (16 or 32 bit)
	const IndexType indexType_;

	//! Initialize data common for all index buffer implementations.
	StelIndexBuffer(IndexType indexType)
		: indexType_(indexType)
		, locked_(false)
		, indexCount_(0)
	{
	}

	//! Implementation of addIndex.
	//!
	//! @see addIndex
	virtual void addIndex_(const uint index) = 0;

	//! Implementation of getIndex.
	//!
	//! @see getIndex
	virtual uint getIndex_(const int which) const = 0;

	//! Implementation of setIndex.
	//!
	//! @see setIndex
	virtual void setIndex_(const int which, const uint index) = 0;

	//! Implementation of clear.
	//!
	//! @see clear
	virtual void clear_() = 0;

	//! Implementation of lock.
	//!
	//! @see lock
	virtual void lock_() = 0;

	//! Implementation of unlock.
	//!
	//! @see unlock
	virtual void unlock_() = 0;

private:
	//! Is the buffer locked (ready for drawing, possibly in GPU memory) ?
	bool locked_;

	//! Number of indices in the buffer.
	int indexCount_;
};


#endif //_STELINDEXBUFFER_HPP_

