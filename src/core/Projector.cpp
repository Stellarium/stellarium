/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "Translator.hpp"

#include "Projector.hpp"
#include "ProjectorClasses.hpp"


#include <QDebug>
#include <QString>

const QString Projector::maskTypeToString(ProjectorMaskType type)
{
	if (type == MaskDisk )
		return "disk";
	else
		return "none";
}

Projector::ProjectorMaskType Projector::stringToMaskType(const QString &s)
{
	if (s=="disk")
		return MaskDisk;
	return MaskNone;
}

void Projector::init(const ProjectorParams& params)
{
	maskType = (ProjectorMaskType)params.maskType;
	zNear = params.zNear;
	zFar = params.zFar;
	viewportXywh = params.viewportXywh;
	viewportCenter = params.viewportCenter;
	gravityLabels = params.gravityLabels;
	flipHorz = params.flipHorz ? -1. : 1.;
	flipVert = params.flipVert ? -1. : 1.;
	viewportFovDiameter = params.viewportFovDiameter;
	pixelPerRad = 0.5 * viewportFovDiameter / fovToViewScalingFactor(params.fov*(M_PI/360.0));
	//Q_ASSERT(params.fov<=getMaxFov());
}

QString Projector::getHtmlSummary() const
{
	return QString("<h3>%1</h3><p>%2</p><b>%3</b>%4").arg(getNameI18()).arg(getDescriptionI18()).arg(q_("Maximum FOV: ")).arg(getMaxFov())+QChar(0x00B0);
}

/*************************************************************************
 Return a convex polygon on the sphere which includes the viewport in the 
 current frame
*************************************************************************/
StelGeom::ConvexPolygon Projector::getViewportConvexPolygon(double marginX, double marginY) const
{
	Vec3d e0, e1, e2, e3;
	unProject(0-marginX,0-marginY,e0);
	unProject(getViewportWidth()+marginX,0-marginY,e1);
	unProject(getViewportWidth()+marginX,getViewportHeight()+marginY,e2);
	unProject(0-marginX,getViewportHeight()+marginY,e3);
	e0.normalize();
	e1.normalize();
	e2.normalize();
	e3.normalize();
	return needGlFrontFaceCW() ? StelGeom::ConvexPolygon(e1, e0, e3, e2) : StelGeom::ConvexPolygon(e0, e1, e2, e3);
}

//! Un-project the entire viewport depending on mapping, maskType, viewportFovDiameter,
//! viewportCenter, and viewport dimensions.
// Last not least all halfplanes n*x>d really should have d<=0 or at least very small d/n.length().
StelGeom::ConvexS Projector::unprojectViewport(void) const
{
	double fov = 360./M_PI*viewScalingFactorToFov(0.5*viewportFovDiameter/pixelPerRad);
	if ((dynamic_cast<const ProjectorCylinder*>(this) == 0 || fov < 90) && fov < 360.0)
	{
		Vec3d e0,e1,e2,e3;
		bool ok;
		if (maskType == MaskDisk)
		{
			if (fov >= 120.0)
			{
				unProject(viewportCenter[0],viewportCenter[1],e0);
				StelGeom::ConvexS rval(1);
				rval[0].n = e0;
				rval[0].d = (fov<360.0) ? cos(fov*(M_PI/360.0)) : -1.0;
				return rval;
			}
			ok  = unProject(viewportCenter[0] - 0.5*viewportFovDiameter, viewportCenter[1] - 0.5*viewportFovDiameter,e0);
			ok &= unProject(viewportCenter[0] + 0.5*viewportFovDiameter, viewportCenter[1] + 0.5*viewportFovDiameter,e2);
			if (needGlFrontFaceCW())
			{
				ok &= unProject(viewportCenter[0] - 0.5*viewportFovDiameter, viewportCenter[1] + 0.5*viewportFovDiameter,e3);
				ok &= unProject(viewportCenter[0] + 0.5*viewportFovDiameter, viewportCenter[1] - 0.5*viewportFovDiameter,e1);
			}
			else
			{
				ok &= unProject(viewportCenter[0] - 0.5*viewportFovDiameter, viewportCenter[1] + 0.5*viewportFovDiameter,e1);
				ok &= unProject(viewportCenter[0] + 0.5*viewportFovDiameter, viewportCenter[1] - 0.5*viewportFovDiameter,e3);
			}
		}
		else
		{
			ok  = unProject(viewportXywh[0],viewportXywh[1],e0);
			ok &= unProject(viewportXywh[0]+viewportXywh[2], viewportXywh[1]+viewportXywh[3],e2);
			if (needGlFrontFaceCW())
			{
				ok &= unProject(viewportXywh[0],viewportXywh[1]+viewportXywh[3],e3);
				ok &= unProject(viewportXywh[0]+viewportXywh[2],viewportXywh[1],e1);
			}
			else
			{
				ok &= unProject(viewportXywh[0],viewportXywh[1]+viewportXywh[3],e1);
				ok &= unProject(viewportXywh[0]+viewportXywh[2],viewportXywh[1],e3);
			}
		}
		if (ok)
		{
			StelGeom::HalfSpace h0(e0^e1);
			StelGeom::HalfSpace h1(e1^e2);
			StelGeom::HalfSpace h2(e2^e3);
			StelGeom::HalfSpace h3(e3^e0);
			if (h0.contains(e2) && h0.contains(e3) && h1.contains(e3) && h1.contains(e0) &&
				h2.contains(e0) && h2.contains(e1) && h3.contains(e1) && h3.contains(e2))
			{
				StelGeom::ConvexS rval(4);
				rval[0] = h3;
				rval[1] = h2;
				rval[2] = h1;
				rval[3] = h0;
				return rval;
			}
			else
			{
				Vec3d middle;
				if (unProject(viewportXywh[0]+0.5*viewportXywh[2], viewportXywh[1]+0.5*viewportXywh[3],middle))
				{
					double d = middle*e0;
					double h = middle*e1;
					if (d > h) d = h;
					h = middle*e2;
					if (d > h) d = h;
					h = middle*e3;
					if (d > h) d = h;
					StelGeom::ConvexS rval(1);
					rval[0].n = middle;
					rval[0].d = d;
					return rval;
				}
			}
		}
	}
	StelGeom::ConvexS rval(1);
	rval[0].n = Vec3d(1.0,0.0,0.0);
	rval[0].d = -2.0;
	return rval;
}

// Return a Halfspace containing the whole viewport
StelGeom::HalfSpace Projector::getBoundingHalfSpace() const
{
	// TODO
	Q_ASSERT(0);
	return StelGeom::HalfSpace();
}

/*************************************************************************
 Project the vector v from the viewport frame into the current frame 
*************************************************************************/
bool Projector::unProject(double x, double y, Vec3d &v) const
{
	v[0] = flipHorz * (x - viewportCenter[0]) / pixelPerRad;
	v[1] = flipVert * (y - viewportCenter[1]) / pixelPerRad;
	v[2] = 0;
	const bool rval = backward(v);
	  // Even when the reprojected point comes from an region of the screen,
	  // where nothing is projected to (rval=false), we finish reprojecting.
	  // This looks good for atmosphere rendering, and it helps avoiding
	  // discontinuities when dragging around with the mouse.

	x = v[0] - modelViewMatrix.r[12];
	y = v[1] - modelViewMatrix.r[13];
	const double z = v[2] - modelViewMatrix.r[14];
	v[0] = modelViewMatrix.r[0]*x + modelViewMatrix.r[1]*y + modelViewMatrix.r[2]*z;
	v[1] = modelViewMatrix.r[4]*x + modelViewMatrix.r[5]*y + modelViewMatrix.r[6]*z;
	v[2] = modelViewMatrix.r[8]*x + modelViewMatrix.r[9]*y + modelViewMatrix.r[10]*z;

// Johannes: Get rid of the inverseModelViewMatrix.
// We need no matrix inversion because we always work with orthogonal matrices
// (where the transposed is the inverse).
//	v.transfo4d(inverseModelViewMatrix);
	return rval;
}
