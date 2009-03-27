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

	Vec3d StaticCoordCalc::calcCoord(double /*t*/, double /*startJD*/)
	{
		return mCoord;
	}

	EllipseCoordCalc::EllipseCoordCalc(double radiusA, double radiusB, double period, double phi, double thet, double psi, 
									   double startAngle) : 
		mRadiusA(radiusA), mRadiusB(radiusB), mPeriod(period), mStartAngle(startAngle),
		mRotMat(Mat4f::zrotation(phi) * Mat4f::xrotation(thet) * Mat4f::yrotation(psi))
	{
	}

	Vec3d EllipseCoordCalc::calcCoord(double t, double /*startJD*/)
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
	start(s), delta(d)
	{
		if (direction >= 0) mDirection = 1.0f;
		else mDirection = -1.0f;
	}
	
	Vec3d StraightCoordCalc::calcCoord(double t, double startJD)
	{		
		Vec3d pos = start;
		double q = -mDirection*(t - mStartTime) * delta;		
		pos += q*start;
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
	
	Vec3d DurationStraightCoordCalc::calcCoord(double t, double startJD)
	{
		double q = 1.0 - 4.0 / cPi * atan( mDirection*(t - mStartTime) / mDur);		
		return q * mStart;
	}

	LocalSystemCoordCalc::LocalSystemCoordCalc(GCPtr<CoordCalc> coordCalcLocal, GCPtr<CoordCalc> coordCalcSystem) :
		mCoordCalcLocal(coordCalcLocal), mCoordCalcSystem(coordCalcSystem)
	{
	}

	Vec3d LocalSystemCoordCalc::calcCoord(double t, double startJD)
	{
		return mCoordCalcLocal->calcCoord(t,startJD) + mCoordCalcSystem->calcCoord(t,startJD);
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

	/*MyltipleCoordCalc::MyltipleCoordCalc() : mLastOrbitChange(0.0), mCurrentOrbit(0)
	{
	}

	void MyltipleCoordCalc::addCoordCalc(double duration, GCPtr<CoordCalc> coordCalc)
	{
		mTrajectory.push_back(coordCalc);
		mDuration.push_back(duration);
	}
	
	Vec3d MyltipleCoordCalc::calcCoord(double t, double startJD)
	{
		if ( (t - mLastOrbitChange) >= mDuration[mCurrentOrbit] )
        {
            mCurrentOrbit++;
            mLastOrbitChange = t;
            if (mCurrentOrbit == mTrajectory.size())
            {
                GCPtr<CoordCalc> posCalc = new StaticCoordCalc(mTrajectory[mCurrentOrbit - 1]->calcCoord(t,startJD));
                mTrajectory.push_back(posCalc);
                mDuration.push_back(FLT_MAX);
            }
        }
        Vec3d vec = mTrajectory[mCurrentOrbit]->calcCoord(t,startJD);
        return vec;
	}*/

    PlanetCoordCalc::PlanetCoordCalc(Planet* planetPtr) : planet(planetPtr)
    {
    }

    Vec3d PlanetCoordCalc::calcCoord(double t, double startJD)
    {
		return planet->get_heliocentric_ecliptic_pos(t + startJD);
    }
}