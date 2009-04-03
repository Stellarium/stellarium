#include "CoordCalc.h"
#include <cmath>
#include "float.h"

using namespace std;

namespace
{
	const double cPi = 3.14159265358979323846f;
}

namespace MotionTestImpl
{
	CoordCalc::CoordCalc() : mStartTime(0.0)
	{
	}

	void CoordCalc::setStartTime(double startTime)
	{
		mStartTime = startTime;
	}

	StaticCoordCalc::StaticCoordCalc(const Vec3d& coord) :
		mCoord(coord)
	{
	}

	void StaticCoordCalc::setCoord(const Vec3d& coord)
	{
		mCoord = coord;
	}

	Vec3d StaticCoordCalc::calcCoord(double /*t*/)
	{
		return mCoord;
	}

	EllipseCoordCalc::EllipseCoordCalc(double radiusA, double radiusB, double period, double phi, double thet, double psi, 
									   double startAngle) : 
		mRadiusA(radiusA), mRadiusB(radiusB), mPeriod(period), mStartAngle(startAngle),
		mRotMat(Mat4f::zrotation(phi) * Mat4f::xrotation(thet) * Mat4f::yrotation(psi))
	{
	}

	Vec3d EllipseCoordCalc::calcCoord(double t)
	{
		Vec3d coord;
		coord[0] = mRadiusA * cos(2.0f * cPi / mPeriod * (t - mStartTime) + cPi * mStartAngle / 180.0f);
		coord[1] = mRadiusB * sin(2.0f * cPi / mPeriod * (t - mStartTime) + cPi * mStartAngle / 180.0f);
		coord[2] = 0.0f;
		coord.transfo4d(mRotMat);
		return coord;
	}

	double EllipseCoordCalc::getRadiusA() const
	{
		return mRadiusA;
	}

	double EllipseCoordCalc::getRadiusB() const
	{
		return mRadiusB;
	}

	double EllipseCoordCalc::getStartTime() const
	{
		return mStartTime;
	}

	double EllipseCoordCalc::getStartAngle() const
	{
		return mStartAngle;
	}

	double EllipseCoordCalc::getPeriod() const
	{
		return mPeriod;
	}

	Mat4f EllipseCoordCalc::getRotMat() const
	{
		return mRotMat;
	}

	StraightCoordCalc::StraightCoordCalc(const Vec3d& s, double d, double direction) : 
	mStart(s), delta(d)
	{
		if (direction >= 0) mDirection = 1.0f;
		else mDirection = -1.0f;
	}
	
	Vec3d StraightCoordCalc::calcCoord(double t)
	{		
		Vec3d pos = mStart;
		double q = -mDirection*(t - mStartTime) * delta;		
		pos += q*mStart;
		return pos;
	}

	/*DurationStraightCoordCalc::DurationStraightCoordCalc(const Vec3d& s, double dur, double startTime) :
		StraightCoordCalc(s, 1.0f / dur, startTime)
	{
	}*/

	DurationStraightCoordCalc::DurationStraightCoordCalc(const Vec3d& s, double dur, double direction) : 
		mStart(s), mDur(dur)
	{
		if (direction >= 0) mDirection = 1.0f;
		else mDirection = -1.0f;
	}
	
	Vec3d DurationStraightCoordCalc::calcCoord(double t)
	{
		//double q = 1.0 - pow(4.0 / cPi * atan( mDirection*(t - mStartTime) / mDur), 2.0);	
		double q = 1.0 - mDirection * (t - mStartTime) / mDur;
		return q * mStart;
	}

	LocalSystemCoordCalc::LocalSystemCoordCalc(GCPtr<CoordCalc> coordCalcLocal, GCPtr<CoordCalc> coordCalcSystem) :
		mCoordCalcLocal(coordCalcLocal), mCoordCalcSystem(coordCalcSystem)
	{
	}

	Vec3d LocalSystemCoordCalc::calcCoord(double t)
	{
		return mCoordCalcLocal->calcCoord(t) + mCoordCalcSystem->calcCoord(t);
	}

	GCPtr<CoordCalc> LocalSystemCoordCalc::getCoordCalcLocal() const
	{
		return mCoordCalcLocal;
	}

	GCPtr<CoordCalc> LocalSystemCoordCalc::getCoordCalcSystem() const
	{
		return mCoordCalcSystem;
	}

	void LocalSystemCoordCalc::setStartTime(double startTime)
	{
		mCoordCalcLocal->setStartTime(startTime);
		mCoordCalcSystem->setStartTime(startTime);
	}

    PlanetCoordCalc::PlanetCoordCalc(Planet* planetPtr) : planet(planetPtr)
    {
    }

    Vec3d PlanetCoordCalc::calcCoord(double t)
    {
		return planet->get_heliocentric_ecliptic_pos(t);
    }
}