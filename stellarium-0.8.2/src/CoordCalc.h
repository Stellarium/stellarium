#ifndef COORD_CALC_H
#define COORD_CALC_H

#include "vecmath.h"
#include "GCPtr.h"
#include <vector>
#include "planet.h"

namespace MotionTestImpl
{
	const double coeff = 86400.0f;

	class CoordCalc
	{
	public:
		CoordCalc();
		virtual ~CoordCalc() {}

		virtual Vec3d calcCoord(double t) = 0;

		virtual void setStartTime(double startTime);
	protected:
		double mStartTime;
	};

	class StaticCoordCalc : public CoordCalc
	{
	public:
		StaticCoordCalc(const Vec3d& coord);

		void setCoord(const Vec3d& coord);
		
		virtual Vec3d calcCoord(double /*t*/);
	private:
		Vec3d mCoord;
	};

	class EllipseCoordCalc : public CoordCalc
	{
	public:
		EllipseCoordCalc(double radiusA, double radiusB, double period, double phi, double thet, double psi,
			double startAngle = 0.0f);

		virtual Vec3d calcCoord(double t);

		double getRadiusA() const;
		double getRadiusB() const;
		double getPeriod() const;
		Mat4f getRotMat() const;
		double getStartTime() const;
		double getStartAngle() const;
	private:
		double mRadiusA;
		double mRadiusB;
		double mPeriod;
		double mStartAngle;
		Mat4f mRotMat;
	};

	class StraightCoordCalc : public CoordCalc
	{
	public:
		StraightCoordCalc(const Vec3d& s, double d = 5.0f, double direction = 1.0f);

		virtual Vec3d calcCoord(double t);        
	private:
		Vec3d start;
		double delta;
		double mDirection;
	};

	class DurationStraightCoordCalc : public CoordCalc // : public StraightCoordCalc
	{
	public:
		DurationStraightCoordCalc(const Vec3d& s, double dur, double direction = 1.0f);

		virtual Vec3d calcCoord(double t);
	private:
		Vec3d mStart;
		double mDur;
		double mDirection;
	};

	class LocalSystemCoordCalc : public CoordCalc
	{
	public:
		LocalSystemCoordCalc(GCPtr<CoordCalc> coordCalcLocal, GCPtr<CoordCalc> coordCalcSystem);

		virtual Vec3d calcCoord(double t);

		virtual void setStartTime(double startTime);

		GCPtr<CoordCalc> getCoordCalcLocal() const;
		GCPtr<CoordCalc> getCoordCalcSystem() const;
	private:
		GCPtr<CoordCalc> mCoordCalcLocal;
		GCPtr<CoordCalc> mCoordCalcSystem;
	};


    class PlanetCoordCalc : public CoordCalc
    {    
    public:
        PlanetCoordCalc(Planet* planetPtr);
        virtual Vec3d calcCoord(double t);
    private:
        Planet* planet;
    };
}

#endif