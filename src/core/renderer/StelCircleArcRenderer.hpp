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

#ifndef _STELCIRCLEARCRENDERER_HPP_
#define _STELCIRCLEARCRENDERER_HPP_

#include <QLinkedList>

#include "GenericVertexTypes.hpp"
#include "StelProjector.hpp"
#include "StelSphereGeometry.hpp"
#include "StelRenderer.hpp"


//! Provides functions to draw circle arcs using StelRenderer.
//!
//! Performance could be improved if drawXXXCircleArc returned a list 
//! of vertex buffers to draw (perhaps generateXXXCircleArc), which could 
//! be cached as long as the StelProjector used is not modified. This 
//! would require a way to track the last update when StelProjector was 
//! modified.
//!
//! Example:
//! @code
//! // Drawing a single great circle arc
//! StelCircleArcRenderer(renderer, projector)
//! 	.drawGreatCircleArc(star1, star2, &viewportHalfspace);
//!
//! // Create a circle arc renderer to draw multiple circle arcs
//! StelCircleArcRenderer circleArcRenderer = StelCircleArcRenderer(renderer, projector);
//!
//! circleArcRenderer.drawGreatCircleArc(star1, star2, &viewportHalfspace);
//! circleArcRenderer.drawGreatCircleArc(star3, star4, &viewportHalfspace);
//! @endcode
class StelCircleArcRenderer
{
public:
	//! Construct a StelCircleArcRenderer using specified renderer.
	//!
	//! @param renderer  Renderer to create adn draw vertex buffers.
	//! @param projector Projector to project 3D coordinates to the screen.
	StelCircleArcRenderer(StelRenderer* renderer, StelProjectorP projector)
		: renderer(renderer)
		, projector(&(*projector))
	{
		smallCircleVertexBuffer = renderer->createVertexBuffer<VertexP2>(PrimitiveType_LineStrip);
	}

	//! Construct a StelCircleArcRenderer using specified renderer.
	//!
	//! @param renderer  Renderer to create adn draw vertex buffers.
	//! @param projector Projector to project 3D coordinates to the screen.
	StelCircleArcRenderer(StelRenderer* renderer, StelProjector* projector)
		: renderer(renderer)
		, projector(projector)
	{
		smallCircleVertexBuffer = renderer->createVertexBuffer<VertexP2>(PrimitiveType_LineStrip);
	}

	//! Destroy the StelCircleArcRenderer, freeing any vertex buffers.
	~StelCircleArcRenderer()
	{
		delete smallCircleVertexBuffer;
	}

	//! Draw a great circle arc between points start and stop.
	//!
	//! The angle between start and stop must be < 180 deg.
	//!
	//! The line will look smooth, even for non linear distortion.
	//! Each time the small circle crosses the edge of the viewport,
	//! viewportEdgeIntersectCallback is called with the
	//! screen 2d position of the intersection, normalized direction of the 
	//! currently drawn arc toward the inside of the viewport, and any extra
	//! user specified data.
	//!
	//! @param start       Start point of the arc.
	//! @param stop        End point of the arc.
	//! @param clippingCap if not NULL, try to clip part of the arc outside the cap.
	//! @param viewportEdgeIntersectCallback 
	//!                    Callback called on intersection with viewportEdge.
	//! @param userData    User-defined data passed to viewportEdgeIntersectCallback.
	void drawGreatCircleArc
		(const Vec3d& start, const Vec3d& stop, const SphericalCap* clippingCap = NULL,
		 void (*viewportEdgeIntersectCallback)
		      (const Vec3d& screenPos, const Vec3d& direction, void* userData) = NULL, 
		 void* userData = NULL)
	{
		Vec3d pt1, pt2;
		rotCenter = Vec3d(0);
		if (NULL != clippingCap)
		{
			pt1 = start;
			pt2 = stop;
			if (clippingCap->clipGreatCircle(pt1, pt2))
			{
			  drawSmallCircleArc(pt1, pt2, viewportEdgeIntersectCallback, userData);
			}
			return;
		}
		drawSmallCircleArc(start, stop, viewportEdgeIntersectCallback, userData);
	}

	//! Draw a series of great circle arcs between specified points. 
	//!
	//! primitiveType specifies how to interpret the points. 
	//!
	//! If primitiveType is PrimitiveType_Lines,
	//! the arcs are drawn between the first and the second point, third and fourth, and so on.
	//! In this case, the number of points must be divisible by 2.
	//!
	//! If primitiveType is PrimitiveType_LineStrip, the arcs are drawn between the  
	//! first and the second point, second and third, third and fourth, and so on.
	//!
	//! If primitiveType is PrimitiveType_LineLoop, the arcs are drawn like with 
	//! PrimitiveType_LineStrip, but there is also an arc between the last and the first point.
	//!
	//! The angle between two points of any arc must be < 180 deg.
	//!
	//! @param points        Points to use to draw the arcs.
	//! @param primitiveType Determines how the points to draw arcs between are chosen
	//!                      (see above). Must be PrimitiveType_Lines, PrimitiveType_LineStrip
	//!                      or PrimitiveType_LineLoop.
	//! @param clippingCap   If not NULL, try to clip the parts of the arcs outside the cap.
	void drawGreatCircleArcs(const QVector<Vec3d>& points, const PrimitiveType primitiveType,
	                         const SphericalCap* clippingCap)
	{
		Q_ASSERT_X(points.size() != 1, Q_FUNC_INFO, 
		           "Need zero or at least 2 points to draw circle arcs");
		switch(primitiveType)
		{
			case PrimitiveType_Lines:
				Q_ASSERT_X(points.size() % 2 == 0, Q_FUNC_INFO, 
				           "Need an even number of points to draw great circle arcs from a "
				           "lines primitive type.");
				for (int p = 0; p < points.size(); p += 2 ) 
				{
					drawGreatCircleArc(points.at(p), points.at(p+1), clippingCap);
				}
				break;
			case PrimitiveType_LineLoop:
				if(points.size() > 0)
				{
					drawGreatCircleArc(points.at(points.size() - 1), points.at(0));
				}
			case PrimitiveType_LineStrip:
				for (int p = 1; p < points.size(); p++) 
				{
					drawGreatCircleArc(points.at(p - 1), points.at(p));
				}
				break;
			default:
				Q_ASSERT_X(false, Q_FUNC_INFO, "Unsupported primitive type to draw circle arcs from");
		}
	}

	//! Set rotation point to draw a small circle around.
	void setRotCenter(const Vec3d center)
	{
		rotCenter = center;
	}

	//! Draw a small circle arc between points start and stop 
	//!
	//! The rotation point is set by setRotCenter().
	//!
	//! The angle between start and stop must be < 180 deg.
	//! The line will look smooth, even for non linear distortion.
	//! Each time the small circle crosses the edge of the viewport, 
	//! viewportEdgeIntersectCallback is called with the
	//! screen 2d position of the intersection, normalized direction of the 
	//! currently drawn arc toward the inside of the viewport, and any extra
	//! user specified data.
	//!
	//! If rotCenter is (0,0,0), the method draws a great circle.
	//!
	//! @param start       Start point of the arc.
	//! @param stop        End point of the arc.
	//! @param viewportEdgeIntersectCallback 
	//!                    Callback called on intersection with viewportEdge.
	//! @param userData    User-defined data passed to viewportEdgeIntersectCallback.
	void drawSmallCircleArc
		(const Vec3d& start, const Vec3d& stop,
		 void (*viewportEdgeIntersectCallback)
		      (const Vec3d& screenPos, const Vec3d& direction, void* userData), 
		 void* userData)
	{
		Q_ASSERT_X(smallCircleVertexBuffer->length() == 0, Q_FUNC_INFO,
		           "Small circle buffer must be empty before drawing");

		smallCircleVertexBuffer->unlock();

		vertexList.clear();	
		Vec3d win1, win2;
		win1[2] = projector->project(start, win1) ? 1.0 : -1.1;
		win2[2] = projector->project(stop, win2)  ? 1.0 : -1.0;
		vertexList.append(win1);

		if (rotCenter.lengthSquared() < 0.00000001)
		{
			// Great circle
			// Tesselate the arc in small segments in a way so that the lines look smooth
			fIter(start, stop, win1, win2, 
			      vertexList.insert(vertexList.end(), win2), 1.0);
		}
		else
		{
			const Vec3d tmp = (rotCenter ^ start) / rotCenter.length();
			const double radius = fabs(tmp.length());
			// Tesselate the arc in small segments in a way so that the lines look smooth
			fIter(start - rotCenter, stop - rotCenter, win1, win2, 
			      vertexList.insert(vertexList.end(), win2), radius);
		}

		// And draw.
		QLinkedList<Vec3d>::ConstIterator i = vertexList.constBegin();
		while (i + 1 != vertexList.constEnd())
		{
			const Vec3d& p1 = *i;
			const Vec3d& p2 = *(++i);
			const bool p1InViewport = projector->checkInViewport(p1);
			const bool p2InViewport = projector->checkInViewport(p2);
			
			if ((p1[2] > 0.0 && p1InViewport) || (p2[2] > 0.0 && p2InViewport))
			{
				smallCircleVertexBuffer->addVertex(VertexP2(p1[0], p1[1]));
				if (i + 1 == vertexList.constEnd())
				{
					smallCircleVertexBuffer->addVertex(VertexP2(p2[0], p2[1]));
					drawSmallCircleVertexBuffer();
				}
				// We crossed the edge of the viewport
				if (NULL != viewportEdgeIntersectCallback && p1InViewport != p2InViewport)
				{
					// if !p1InViewport, then p2InViewport
					const Vec3d& a = p1InViewport ? p1 : p2;
					const Vec3d& b = p1InViewport ? p2 : p1;
					Vec3d dir = b - a;
					dir.normalize();
					viewportEdgeIntersectCallback(projector->viewPortIntersect(a, b),
					                              dir, userData);
				}
			}
			else
			{
				// Break the line, draw the stored vertex and flush the list
				if (smallCircleVertexBuffer->length() > 0)
				{
					smallCircleVertexBuffer->addVertex(VertexP2(p1[0], p1[1]));
				}
				drawSmallCircleVertexBuffer();
			}
		}

		Q_ASSERT_X(smallCircleVertexBuffer->length() == 0, Q_FUNC_INFO,
		           "Small circle buffer must be empty after drawing");
	}

private:
	//! Renderer used to construct and draw vertex buffers.
	StelRenderer* renderer;

	//! Projector to project vertices to viewport.
	StelProjector* projector;

	//! Vertex buffer we're drawing the arc with.
	StelVertexBuffer<VertexP2>* smallCircleVertexBuffer;

	//! List of projected points from the tesselated arc.
	QLinkedList<Vec3d> vertexList;	

	//! Center of rotation around which we're tesselating the arc.
	Vec3d rotCenter;

	//! Recursive function cutting a small circle into smaller segments
	void fIter(const Vec3d& p1, const Vec3d& p2, Vec3d& win1, Vec3d& win2, 
	           const QLinkedList<Vec3d>::iterator& iter, double radius, 
	           int nbI = 0, bool checkCrossDiscontinuity = true)
	{
		const bool crossDiscontinuity = 
			checkCrossDiscontinuity && 
			projector->intersectViewportDiscontinuity(p1 + rotCenter, p2 + rotCenter);

		if (crossDiscontinuity && nbI >= 5)
		{
			win1[2] = -2.0;
			win2[2] = -2.0;
			vertexList.insert(iter, win1);
			vertexList.insert(iter, win2);
			return;
		}

		// Point splitting the tesselated line in two
		Vec3d newVertex(p1 + p2); 
		newVertex.normalize();
		newVertex *= radius;
		Vec3d win3(newVertex + rotCenter);
		const bool isValidVertex = projector->projectInPlace(win3);

		const float v10 = win1[0] - win3[0];
		const float v11 = win1[1] - win3[1];
		const float v20 = win2[0] - win3[0];
		const float v21 = win2[1] - win3[1];

		const float dist = std::sqrt((v10 * v10 + v11 * v11) * (v20 * v20 + v21 * v21));
		const float cosAngle = (v10 * v20 + v11 * v21) / dist;
		if ((cosAngle > -0.999f || dist > 50 * 50 || crossDiscontinuity) && nbI < 5)
		{
			// Use the 3rd component of the vector to store whether the vertex is valid
			win3[2]= isValidVertex ? 1.0 : -1.0;
			fIter(p1, newVertex, win1, win3, vertexList.insert(iter, win3),
			      radius, nbI + 1, crossDiscontinuity || dist > 50 * 50);
			fIter(newVertex, p2, win3, win2, iter, 
			      radius, nbI + 1, crossDiscontinuity || dist > 50 * 50);
		}
	}

	//! Draw vertices stored in smallCircleVertexBuffer and remove them.
	void drawSmallCircleVertexBuffer()
	{
		smallCircleVertexBuffer->lock();
		renderer->drawVertexBuffer(smallCircleVertexBuffer);
		smallCircleVertexBuffer->unlock();
		smallCircleVertexBuffer->clear();
	}
};

#endif // _STELCIRCLEARCRENDERER_HPP_
