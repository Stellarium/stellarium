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
									   double startAngle, double startTime) : 
		mRadiusA(radiusA), mRadiusB(radiusB), mPeriod(period), mStartAngle(startAngle), mStartTime(startTime),
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

	void EllipseCoordCalc::setStartTime(double startTime)
	{
		mStartTime = startTime;
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

	StraightCoordCalc::StraightCoordCalc(const Vec3d& s, double d, double startTime, double direction) : 
	start(s), delta(d), mStartTime(startTime)
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

	DurationStraightCoordCalc::DurationStraightCoordCalc(const Vec3d& s, double dur, double startTime, double direction) : 
		mStart(s), mDur(dur), mStartTime(startTime)
	{
		if (direction >= 0) mDirection = 1.0f;
		else mDirection = -1.0f;
	}
	
	Vec3d DurationStraightCoordCalc::calcCoord(double t, double startJD)
	{		
		//double q = 1.0 - sqrt(abs(t) / mDur);
		double q = 1.0 - 4.0 / cPi * atan( mDirection*(t - mStartTime) / mDur);		
		return q * mStart;
	}

	LocalSystemCoordCalc::LocalSystemCoordCalc(GCPtr<CoordCalc> coordCalcLocal, GCPtr<CoordCalc> coordCalcSystem) :
		mCoordCalcLocal(coordCalcLocal), mCoordCalcSystem(coordCalcSystem)
	{
	}

	Vec3d LocalSystemCoordCalc::calcCoord(double t, double startJD)
	{
		//Vec3d coord = mCoordCalcSystem->calcCoord(t);
		//coord.transfo4d(Mat4f::translation(mCoordCalcLocal->calcCoord(t)));
		Vec3d coord = mCoordCalcLocal->calcCoord(t,startJD);
		coord.transfo4d(Mat4f::translation(mCoordCalcSystem->calcCoord(t,startJD)));
		return coord;
	}

	GCPtr<CoordCalc> LocalSystemCoordCalc::getCoordCalcLocal() const
	{
		return mCoordCalcLocal;
	}

	GCPtr<CoordCalc> LocalSystemCoordCalc::getCoordCalcSystem() const
	{
		return mCoordCalcSystem;
	}

	MyltipleCoordCalc::MyltipleCoordCalc() : mLastOrbitChange(0.0), mCurrentOrbit(0)
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
	}

    PlanetCoordCalc::PlanetCoordCalc(Planet* planetPtr) : planet(planetPtr)
    {
        
    }
    Vec3d PlanetCoordCalc::calcCoord(double t, double startJD)
    {
		/*double rot[9];
		const double parent_rot_obliquity = planet->get_parent() && planet->get_parent()->get_parent()
				? planet->get_parent()->getRotObliquity()
				: 0.0;
      const double parent_rot_ascendingnode = planet->get_parent() && planet->get_parent()->get_parent()
				? planet->get_parent()->getRotAscendingnode()
				: 0.0;
//const double parent_rot_obliquity = 0.0f;
//const double parent_rot_ascendingnode = 0.0f;
		const double c_obl = cos(parent_rot_obliquity);
  const double s_obl = sin(parent_rot_obliquity);
  const double c_nod = cos(parent_rot_ascendingnode);
  const double s_nod = sin(parent_rot_ascendingnode);
  rot[0] =  c_nod;
  rot[1] = -s_nod * c_obl;
  rot[2] =  s_nod * s_obl;
  rot[3] =  s_nod;
  rot[4] =  c_nod * c_obl;
  rot[5] = -c_nod * s_obl;
  rot[6] =  0.0;
  rot[7] =          s_obl;
  rot[8] =          c_obl;*/
		//Mat4d mat(rot[1], rot[2], rot[0], 0, rot[4], rot[5], rot[3], 0, rot[7], rot[8], rot[6], 0, 0, 0, 0, 1);
		//Mat4d mat(rot[0], rot[1], rot[2], 0, rot[3], rot[4], rot[5], 0, rot[6], rot[7], rot[8], 0, 0, 0, 0, 1);
		//mat = mat.inverse();
		Vec3d v = planet->get_heliocentric_ecliptic_pos(t + startJD);
		//Vec3d v = planet->get_ecliptic_pos(t + startJD);
		//v.transfo4d(mat);
		return v;
    }
}