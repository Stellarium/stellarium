/*
 * Stellarium
 * Copyright (C) Stellarium Team
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

#ifndef _TESTSTELVERTEXBUFFER_HPP_
#define _TESTSTELVERTEXBUFFER_HPP_

#include <QObject>
#include <QtTest>

#include "renderer/StelVertexBuffer.hpp"
#include "renderer/StelQGL2ArrayVertexBufferBackend.hpp"

//! Unittests for StelVertexBuffer implementations.
class TestStelVertexBuffer : public QObject
{
Q_OBJECT
private slots:
	void initTestCase();
	
	void testStelQGL2ArrayVertexBuffer();
	
private:
	//! Test specified StelVertexBuffer backend with specified vertex type.
	//!
	//! @tparam BufferBackend Vertex buffer backend.
	//! @tparam Vertex        Vertex type stored in the buffer. 
	template<class BufferBackend, class Vertex>
	void testVertexBuffer();
};

#endif // _TESTSTELVERTEXBUFFER_HPP_
