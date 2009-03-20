#ifndef COORD_CALC_H
#define COORD_CALC_H

#include "vecmath.h"
#include "GCPtr.h"
#include <vector>
#include "planet.h"

class Section;

namespace MotionTestImpl
{
	const double coeff = 1000.0f;

	class CoordCalc
	{
	public:
		virtual ~CoordCalc() {}

		virtual Vec3d calcCoord(double t, double /*startJD*/) = 0;
	};

	class StaticCoordCalc : public CoordCalc
	{
	public:
		StaticCoordCalc(const Vec3d& coord);

		void setCoord(const Vec3d& coord);
		
		virtual Vec3d calcCoord(double /*t*/, double /*startJD*/);
	private:
		Vec3d mCoord;
	};

	class EllipseCoordCalc : public CoordCalc
	{
	public:
		EllipseCoordCalc(double radiusA, double radiusB, double period, double phi, double thet, double psi,
			double startAngle = 0.0f, double startTime = 0.0f);

		virtual Vec3d calcCoord(double t, double /*startJD*/);

		double getRadiusA() const;
		double getRadiusB() const;
		double getPeriod() const;
		Mat4f getRotMat() const;
		double getStartTime() const;
		void setStartTime(double startTime);
		double getStartAngle() const;
	private:
		double mRadiusA;
		double mRadiusB;
		double mPeriod;
		double mStartAngle;
		double mStartTime;
		Mat4f mRotMat;
	};

	class StraightCoordCalc : public CoordCalc
	{
	public:
		StraightCoordCalc(const Vec3d& s, double d = 5.0f, double startTime = 0.0f, double direction = 1.0f);

		virtual Vec3d calcCoord(double t, double startJD);        
	private:
		Vec3d start;
		double delta;
		double mStartTime;
		double mDirection;
	};

	class DurationStraightCoordCalc : public CoordCalc // : public StraightCoordCalc
	{
	public:
		DurationStraightCoordCalc(const Vec3d& s, double dur, double startTime = 0.0f, double direction = 1.0f);

		virtual Vec3d calcCoord(double t, double startJD);
	private:
		Vec3d mStart;
		double mDur;
		double mStartTime;
		double mDirection;
	};

	class LocalSystemCoordCalc : public CoordCalc
	{
	public:
		LocalSystemCoordCalc(GCPtr<CoordCalc> coordCalcLocal, GCPtr<CoordCalc> coordCalcSystem);

		virtual Vec3d calcCoord(double t, double /*startJD*/);

		GCPtr<CoordCalc> getCoordCalcLocal() const;
		GCPtr<CoordCalc> getCoordCalcSystem() const;
	private:
		GCPtr<CoordCalc> mCoordCalcLocal;
		GCPtr<CoordCalc> mCoordCalcSystem;
	};

	class MyltipleCoordCalc : public CoordCalc
	{
	public:
		MyltipleCoordCalc();

		void addCoordCalc(double duration, GCPtr<CoordCalc> coordCalc);

		virtual Vec3d calcCoord(double t, double /*startJD*/);
	private:
		std::vector<GCPtr<CoordCalc> > mTrajectory;
		std::vector<double> mDuration;

		double mLastOrbitChange;
        int mCurrentOrbit;
	};

    class PlanetCoordCalc : public CoordCalc
    {    
    public:
        PlanetCoordCalc(Planet* planetPtr);
        virtual Vec3d calcCoord(double t, double startJD);
    private:
        Planet* planet;
    };
}

#endif