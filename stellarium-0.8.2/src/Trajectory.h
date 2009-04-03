#ifndef TRAJECTORY_H
#define TRAJECTORY_H

#include <string>
#include <vector>
#include "GCPtr.h"
#include "CoordCalc.h"
#include "SolarSystem.h"

namespace MotionTestImpl
{
	class Orbit
    {
	public:
		Orbit(const std::string& name, GCPtr<CoordCalc> CoordFunc);

        std::string GetName() const;
        GCPtr<CoordCalc> GetCoordFunc() const;
	private:
		std::string mName;
		GCPtr<CoordCalc> mCoordFunc;       
	};

	class Trajectory : public CoordCalc
    {
	public:
		Trajectory(const std::string& TrajectoryFile, const SolarSystem *SS);
        virtual Vec3d calcCoord(double t);
		double getStartJD() const;
	private:
		std::vector<GCPtr<Orbit> > mOrbits;
		std::vector<GCPtr<CoordCalc> > mTrajectory;
		std::vector<double> mDuration;

        double mLastOrbitChange;
        int mCurrentOrbit;
		double startJD;

		GCPtr<Orbit> FindOrbit(std::string name) const;     
	};
}

#endif