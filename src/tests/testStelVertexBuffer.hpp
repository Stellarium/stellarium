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
