#ifndef _TESTSTELVERTEXBUFFER_HPP_
#define _TESTSTELVERTEXBUFFER_HPP_

#include <QObject>
#include <QtTest>

#include "renderer/StelVertexBuffer.hpp"
#include "renderer/StelTestGLVertexBuffer.hpp"

//! Unittests for StelVertexBuffer implementations.
class TestStelVertexBuffer : public QObject
{
Q_OBJECT
private slots:
	void initTestCase();
	
	void testStelTestGLVertexBuffer();
};

#endif // _TESTSTELVERTEXBUFFER_HPP_
