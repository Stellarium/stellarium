/*
 * Stellarium
 * Copyright (C) 2008 Stellarium Developers
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

#ifndef _STELPROJECTIONS_HPP_
#define _STELPROJECTIONS_HPP_

#include <limits>
#include "StelProjector.hpp"
#include "StelSphereGeometry.hpp"
#include "renderer/StelGLSLShader.hpp"

class StelProjectorPerspective : public StelProjector
{
public:
	StelProjectorPerspective(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 120.f;}
	bool forward(Vec3f &v) const
	{
		const float r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
		if (v[2] < 0) {
			v[0] /= (-v[2]);
			v[1] /= (-v[2]);
			v[2] = r;
			return true;
		}
		if (v[2] > 0) {
			v[0] /= v[2];
			v[1] /= v[2];
			v[2] = r;
			return false;
		}
		v[0] = std::numeric_limits<float>::max();
		v[1] = std::numeric_limits<float>::max();
		v[2] = r;
		return false;
	}
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const {return false;}
};

class StelProjectorEqualArea : public StelProjector
{
public:
	StelProjectorEqualArea(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 360.f;}
	bool forward(Vec3f &v) const
	{
		const float r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
		const float f = std::sqrt(2.f/(r*(r-v[2])));
		v[0] *= f;
		v[1] *= f;
		v[2] = r;
		return true;
	}
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const {return false;}
};

class StelProjectorStereographic : public StelProjector
{
	friend class StereographicGLSLProjectorShader;
private:
	//! GLSL projector code for StelProjectorStereographic.
	//!
	//! Note that when adding GLSL projection to other projectors, a lot 
	//! of this code might be shared (maybe the main GLSL projection function) 
	//! and need to be refactored to the parent class.
	class StereographicGLSLProjector : public StelProjector::GLSLProjector
	{
	friend class StelProjectorStereographic;
		StelProjectorStereographic& projector;

	public:
		virtual bool init(StelGLSLShader* shader)
		{
			if(projector.disableShaderProjection)
			{
				return false;
			}

			shader->unlock();

			// Add (or enable, if already added) shader for the used modelview transform.
			// Not all ModelViewTranforms have GLSL implementations (e.g. Refraction)
			if(!projector.modelViewTransform->setupGLSLTransform(shader))
			{
				// Failed to add the transform shader, return the shader into locked state.
				if(!shader->build())
				{
					Q_ASSERT_X(false, Q_FUNC_INFO, 
					           "Failed to restore shader after failing to add a modelview "
					           "transform shader to it");
				}
				return false;
			}

			// Add the stereographic projector shader if not yet added.
			if(!shader->hasVertexShader("StereographicProjector"))
			{
				static const QString projectorSource(
					"vec4 modelViewForward(in vec4 v);\n"
					"\n"
					"uniform vec2 viewportCenter;\n"
					"uniform float flipHorz;\n"
					"uniform float flipVert;\n"
					"uniform float pixelPerRad;\n"
					"uniform float zNear;\n"
					"uniform float oneOverZNearMinusZFar;\n"
					"\n"
					"vec4 projectorForward(in vec4 v)\n"
					"{\n"
					"	float r = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);\n"
					"	float h = 0.5 * (r - v.z);\n"
					"	if (h <= 0.0) \n"
					"	{\n"
					"		return vec4(1000000.0, 1000000.0, -1000000.0, 1.0);\n"
					"	}\n"
					"	float f = 1.0 / h;\n"
					"	return vec4(v.x * f, v.y * f, r, 1.0);\n"
					"}\n"
					"\n"
					"vec4 project(in vec4 posIn)\n"
					"{\n"
					"	vec4 v = posIn;\n"
					"	v = modelViewForward(v);\n"
					"	v = projectorForward(v);\n"
					"	return vec4(viewportCenter.x + flipHorz * pixelPerRad * v.x,\n"
					"	            viewportCenter.y + flipVert * pixelPerRad * v.y,\n"
					"	            (v.z - zNear) * oneOverZNearMinusZFar, 1.0);\n"
					"}\n");

				if(!shader->addVertexShader("StereographicProjector", projectorSource))
				{
					// Failed to add the projector shader, return the shader into locked state.
					if(!shader->build())
					{
						Q_ASSERT_X(false, Q_FUNC_INFO, 
						           "Failed to restore shader after failing to add a stereographic "
						           "projector shader to it");
					}
					projector.disableShaderProjection = true;
					return false;
				}
				qDebug() << "Build log after adding a stereographic projection shader: "
				         << shader->log();
			}

			shader->disableVertexShader("DefaultProjector");
			shader->enableVertexShader("StereographicProjector");
			if(!shader->build())
			{
				qDebug() << "Failed to build with a stereographic projector shader: "
				         << shader->log();
				projector.disableShaderProjection = true;
				shader->enableVertexShader("DefaultProjector");
				shader->disableVertexShader("StereographicProjector");
				if(!shader->build())
				{
					Q_ASSERT_X(false, Q_FUNC_INFO, 
					           "Failed to restore default projector shader after failing to link "
					           "with a (succesfully compiled) stereographic projector shader");
				}

			}
			return true;
		}

		virtual void preDraw(StelGLSLShader* shader)
		{
			projector.modelViewTransform->setGLSLUniforms(shader);

			shader->setUniformValue("viewportCenter",        projector.viewportCenter);
			shader->setUniformValue("flipHorz",              projector.flipHorz);
			shader->setUniformValue("flipVert",              projector.flipVert);
			shader->setUniformValue("pixelPerRad",           projector.pixelPerRad);
			shader->setUniformValue("zNear",                 projector.zNear);
			shader->setUniformValue("oneOverZNearMinusZFar", projector.oneOverZNearMinusZFar);
		}

		virtual void postDraw(StelGLSLShader* shader)
		{
			shader->unlock();
			projector.modelViewTransform->disableGLSLTransform(shader);
			shader->disableVertexShader("StereographicProjector");
			shader->enableVertexShader("DefaultProjector");
			if(!shader->build())
			{
				Q_ASSERT_X(false, Q_FUNC_INFO, "Failed to disable projector shader");
			}
		}

	private:
		//! Constructor. Only a StelProjectorStereographic can call this.
		StereographicGLSLProjector(StelProjectorStereographic& projector)
			: projector(projector)
		{
		}
	};

public:
	StelProjectorStereographic(ModelViewTranformP func) 
		: StelProjector(func)
		, glslProjector(*this)
	{;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 235.f;}
	bool forward(Vec3f &v) const
	{
		const float r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
		const float h = 0.5f*(r-v[2]);
		if (h <= 0.f) {
			v[0] = std::numeric_limits<float>::max();
			v[1] = std::numeric_limits<float>::max();
			v[2] = -std::numeric_limits<float>::min();
			return false;
		}
		const float f = 1.f / h;
		v[0] *= f;
		v[1] *= f;
		v[2] = r;
		return true;
	}

	virtual void project(int n, const Vec3d* in, Vec3f* out)
	{
		Vec3d v;
		for (int i = 0; i < n; ++i, ++out)
		{
			v = in[i];
			modelViewTransform->forward(v);
			out->set(v[0], v[1], v[2]);
			StelProjectorStereographic::forward(*out);
			out->set(viewportCenter[0] + flipHorz * pixelPerRad * (*out)[0],
				viewportCenter[1] + flipVert * pixelPerRad * (*out)[1],
				((*out)[2] - zNear) * oneOverZNearMinusZFar);
		}
	}

	virtual GLSLProjector* getGLSLProjector()
	{
		return &glslProjector;
	}

	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const {return false;}

private:
	//! If GLSL is supported, this is used for GPU projection.
	StereographicGLSLProjector glslProjector;
};

class StelProjectorFisheye : public StelProjector
{
public:
	StelProjectorFisheye(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 180.00001f;}
	bool forward(Vec3f &v) const
	{
		const float rq1 = v[0]*v[0] + v[1]*v[1];
		if (rq1 > 0.f) {
			const float h = std::sqrt(rq1);
			const float f = std::atan2(h,-v[2]) / h;
			v[0] *= f;
			v[1] *= f;
			v[2] = std::sqrt(rq1 + v[2]*v[2]);
			return true;
		}
		if (v[2] < 0.f) {
			v[0] = 0.f;
			v[1] = 0.f;
			v[2] = 1.f;
			return true;
		}
		v[0] = std::numeric_limits<float>::max();
		v[1] = std::numeric_limits<float>::max();
		v[2] = std::numeric_limits<float>::min();
		return false;
	}
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const {return false;}
};

class StelProjectorHammer : public StelProjector
{
public:
	StelProjectorHammer(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 360.f;}
	virtual void project(int n, const Vec3d* in, Vec3f* out)
	{
		Vec3d v;
		for (int i = 0; i < n; ++i)
		{
			v = in[i];
			modelViewTransform->forward(v);
			out[i].set(v[0], v[1], v[2]);
			StelProjectorHammer::forward(out[i]);
			out[i][0] = viewportCenter[0] + flipHorz * pixelPerRad * out[i][0];
			out[i][1] = viewportCenter[1] + flipVert * pixelPerRad * out[i][1];
			out[i][2] = (out[i][2] - zNear) * oneOverZNearMinusZFar;
		}
	}
	bool forward(Vec3f &v) const
	{
		// Hammer Aitoff
		const float r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
		const float alpha = std::atan2(v[0],-v[2]);
		const float cosDelta = std::sqrt(1.f-v[1]*v[1]/(r*r));
		float z = std::sqrt(1.+cosDelta*std::cos(alpha/2.f));
		v[0] = 2.f*M_SQRT2*cosDelta*std::sin(alpha/2.f)/z;
		v[1] = M_SQRT2*v[1]/r/z;
		v[2] = r;
		return true;
	}
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return true;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& p1, const Vec3d& p2) const {return p1[0]*p2[0]<0 && !(p1[2]<0 && p2[2]<0);}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& capN, double capD) const
	{
		static const SphericalCap cap1(1,0,0);
		static const SphericalCap cap2(-1,0,0);
		static const SphericalCap cap3(0,0,-1);
		SphericalCap cap(capN, capD);
		return cap.intersects(cap1) && cap.intersects(cap2) && cap.intersects(cap2);
	}
};

class StelProjectorCylinder : public StelProjector
{
public:
	StelProjectorCylinder(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 175.f * 4.f/3.f;} // assume aspect ration of 4/3 for getting a full 360 degree horizon
	bool forward(Vec3f &win) const;
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return true;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& p1, const Vec3d& p2) const
	{
		return p1[0]*p2[0]<0 && !(p1[2]<0 && p2[2]<0);
	}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& capN, double capD) const
	{
		static const SphericalCap cap1(1,0,0);
		static const SphericalCap cap2(-1,0,0);
		static const SphericalCap cap3(0,0,-1);
		SphericalCap cap(capN, capD);
		return cap.intersects(cap1) && cap.intersects(cap2) && cap.intersects(cap2);
	}
};

class StelProjectorMercator : public StelProjector
{
public:
	StelProjectorMercator(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 175.f * 4.f/3.f;} // assume aspect ration of 4/3 for getting a full 360 degree horizon
	bool forward(Vec3f &win) const;
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return true;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& p1, const Vec3d& p2) const
	{
		return p1[0]*p2[0]<0 && !(p1[2]<0 && p2[2]<0);
	}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d& capN, double capD) const
	{
		static const SphericalCap cap1(1,0,0);
		static const SphericalCap cap2(-1,0,0);
		static const SphericalCap cap3(0,0,-1);
		SphericalCap cap(capN, capD);
		return cap.intersects(cap1) && cap.intersects(cap2) && cap.intersects(cap2);
	}
};

class StelProjectorOrthographic : public StelProjector
{
public:
	StelProjectorOrthographic(ModelViewTranformP func) : StelProjector(func) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 179.9999f;}
	bool forward(Vec3f &win) const;
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const {return false;}
};

class StelProjector2d : public StelProjector
{
public:
	StelProjector2d() : StelProjector(ModelViewTranformP(new StelProjector::Mat4dTransform(Mat4d::identity()))) {;}
	virtual QString getNameI18() const;
	virtual QString getDescriptionI18() const;
	virtual float getMaxFov() const {return 360.f;}
	bool forward(Vec3f &win) const;
	bool backward(Vec3d &v) const;
	float fovToViewScalingFactor(float fov) const;
	float viewScalingFactorToFov(float vsf) const;
	float deltaZoom(float fov) const;
protected:
	virtual bool hasDiscontinuity() const {return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, const Vec3d&) const {Q_ASSERT(0); return false;}
	virtual bool intersectViewportDiscontinuityInternal(const Vec3d&, double) const {Q_ASSERT(0); return false;}
	virtual void computeBoundingCap() {;}
};

#endif // _STELPROJECTIONS_HPP_

