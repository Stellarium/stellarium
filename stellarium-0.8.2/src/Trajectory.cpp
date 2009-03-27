#include "trajectory.h"
#include "stellastro.h"
#include "float.h"
#include "vecmath.h"
#include <fstream>
#include <cmath>

using namespace std;

namespace
{
	const double cPi = 3.14159265358979323846f;	

	const double startJD = 2451514.250011573;

	double ctg(double x)
	{
		return cos(x) / sin(x);
	}
}

namespace MotionTestImpl
{
	Orbit::Orbit(const std::string& name, GCPtr<CoordCalc> CoordFunc) : mName(name), mCoordFunc(CoordFunc)
	{
	}
    string Orbit::GetName() const
    {
        return mName;
    }
    GCPtr<CoordCalc> Orbit::GetCoordFunc() const
    {
        return mCoordFunc;
    }
    Trajectory::Trajectory(const std::string& TrajectoryFile, const SolarSystem *SS) : 
		mLastOrbitChange(startJD), mCurrentOrbit(0)
	{
		ifstream file(TrajectoryFile.c_str());
		//const int bufSize = 1024;
		//char buf[bufSize];
		string buf;
		GCPtr<CoordCalc> posCalc;
		while (file >> buf)
		{
			//file >> buf;

			
			if (buf[0] == '#')
			{
				// # - Комментарий в файле траекторий
				char b[1024];
				file.getline(b, 1024);
			}
			else if (buf == "ellipse")
			{
				double radiusA, radiusB, period, phi, thet, psi, startAngle;
				string planet;
				file >> buf >> planet >> radiusA >> radiusB >> period >> phi >> thet >> psi >> startAngle;
				Planet* planetPtr = SS->searchByEnglishName(planet);
				GCPtr<CoordCalc> ellipseCalc = new EllipseCoordCalc(radiusA, radiusB, period / coeff, phi, thet, psi, startAngle);
                GCPtr<CoordCalc> planetCoordCalc = new PlanetCoordCalc(planetPtr);
				posCalc = new LocalSystemCoordCalc(ellipseCalc, planetCoordCalc);
				
				GCPtr<Orbit> orbit = new Orbit(buf, posCalc);
				mOrbits.push_back (orbit);
			} 
			else if (buf == "straight")
			{
				double x, y, z, vel;
				string planet, direction;
				file >> buf >> direction >> planet >> x >> y >> z >> vel;
				Planet* planetPtr = SS->searchByEnglishName(planet);
				GCPtr<CoordCalc> straightCalc;
				if (direction == "to")
				{
					straightCalc = new StraightCoordCalc(Vec3d(x, y, z), vel, 1.0f );
				}
				else if (direction == "from")
				{
					straightCalc = new StraightCoordCalc(Vec3d(x, y, z), vel, -1.0f );
				}
				else
				{
					throw invalid_argument("Bad trajectory file format");
				}
				
                GCPtr<CoordCalc> planetCoordCalc = new PlanetCoordCalc(planetPtr);
				posCalc = new LocalSystemCoordCalc(straightCalc, planetCoordCalc);
				
				GCPtr<Orbit> orbit = new Orbit(buf, posCalc);
				mOrbits.push_back (orbit);
			}
			else if (buf == "fly")
			{
				double x;
				file >> buf >> x;
                
                mTrajectory.push_back( FindOrbit(buf)->GetCoordFunc() );
				//double get_julian_day(const ln_date * date);
                mDuration.push_back(x / coeff);
			}
			else if (buf == "pass")
			{
				double duration;
				double delta;
				file >> buf >> duration >> delta;
				
				duration /= coeff;

				// Ищем эллептическую орбиту, на которую будем переходить 
				GCPtr<CoordCalc> orbit = FindOrbit(buf)->GetCoordFunc();
				LocalSystemCoordCalc* ptr = 
					dynamic_cast<LocalSystemCoordCalc*>(orbit.getBasePointer());
				if (!ptr)
					continue;
				EllipseCoordCalc* ellipseCalc = 
					dynamic_cast<EllipseCoordCalc*>(ptr->getCoordCalcLocal().getBasePointer());
				if (!ellipseCalc)
					continue;

				// Параметры эллиптической орбиты
				double a = ellipseCalc->getRadiusA();
				double b = ellipseCalc->getRadiusB();
				double w = 2.0f * cPi / ellipseCalc->getPeriod();
				double phi0 = cPi * ellipseCalc->getStartAngle() / 180.0f;
				Mat4f rotMat = ellipseCalc->getRotMat();
				
				// t1 - время начала перехода
				// t2 - время окончания перехода (заход на орбиту)
				double t1 = startJD;
				for(vector<double>::const_iterator it = mDuration.begin(); 
					it != mDuration.end(); ++it)
				{
					t1 += *it;
				}
				double t2 = t1 + duration;
				
				ellipseCalc->setStartTime(t2);
				
				// Конечная точка орбиты
				Vec3d vf = ellipseCalc->calcCoord(t2, 0.0);

				// Вычисляем касательную к эллипсу в точка захода
				double xf = a * cos(phi0);
				double yf = b * sin(phi0);
				double x = (yf > 0.0) ? xf + delta : xf - delta;
				double y = -b / a * ctg(phi0) * (x - xf) + yf;

				// Точка на каскательной
				Vec3d vk(x, y, 0.0f);
				vk.transfo4d(rotMat);

				GCPtr<CoordCalc> posCalc = 
					new LocalSystemCoordCalc
					(
						new LocalSystemCoordCalc
						(
							new DurationStraightCoordCalc(vk - vf, duration, t1), 
							new StaticCoordCalc(vf)
						), 
						ptr->getCoordCalcSystem()
					);

				mTrajectory.push_back(posCalc);
				mDuration.push_back(duration);
			}
			else if (buf == "wait")
			{
				double duration;
				file >> duration;
				
				duration /= coeff;

				double t1 = startJD;
				for(vector<double>::const_iterator it = mDuration.begin(); it != mDuration.end(); ++it)
					t1 += *it;

				GCPtr<CoordCalc> posCalc = new StaticCoordCalc(mTrajectory.back()->calcCoord(t1, 0.0));
				mTrajectory.push_back(posCalc);
				mDuration.push_back(duration);
			}	
			else if (buf == "durstraight")
			{
				double x, y, z, dur;
				string planet, direction;
				file >> buf >> direction >> planet >> x >> y >> z >> dur;

				dur /= coeff;

				Planet* planetPtr = SS->searchByEnglishName(planet);
				GCPtr<CoordCalc> straightCalc;
				if (direction == "to")
				{
					straightCalc = new DurationStraightCoordCalc(Vec3d(x, y, z), dur, 1.0f );
				}
				else if (direction == "from")
				{
					straightCalc = new DurationStraightCoordCalc(Vec3d(x, y, z), dur, -1.0f );
				}
				else
				{
					throw invalid_argument("Bad trajectory file format");
				}
				
                GCPtr<CoordCalc> planetCoordCalc = new PlanetCoordCalc(planetPtr);
				posCalc = new LocalSystemCoordCalc(straightCalc, planetCoordCalc);
				
				GCPtr<Orbit> orbit = new Orbit(buf, posCalc);
				mOrbits.push_back (orbit);
			}
			else if (buf == "flyto")
			{
				string planetName;
				double duration;
				file >> planetName >> duration;

				Planet* planetPtr = SS->searchByEnglishName(planetName);
			
				duration /= coeff;

				double t1 = startJD;
				for(vector<double>::const_iterator it = mDuration.begin(); it != mDuration.end(); ++it)
					t1 += *it;
								
				GCPtr<CoordCalc> straightCalc;
				Vec3d start = mTrajectory.back()->calcCoord(t1, 0.0) - 
					planetPtr->get_heliocentric_ecliptic_pos(t1);
				straightCalc = 
					new DurationStraightCoordCalc(start, duration, 1.0f );

				
				GCPtr<CoordCalc> planetCoordCalc = new PlanetCoordCalc(planetPtr);
				posCalc = new LocalSystemCoordCalc(straightCalc, planetCoordCalc);

				mTrajectory.push_back(posCalc);
				mDuration.push_back(duration);
			}
			else
			{
				throw invalid_argument("Bad trajectory file format");
			}

		}
	}

    GCPtr<Orbit> Trajectory::FindOrbit(std::string name) const
	{
		for (vector<GCPtr<Orbit> >::const_iterator it = mOrbits.begin(); it != mOrbits.end(); ++it)
		{
            if ((*it)->GetName() == name)
            {
                return (*it);
            }
		}
        return NULL;
	}
    
    Vec3d Trajectory::calcCoord(double t, double startJD)
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
			mTrajectory[mCurrentOrbit]->setStartTime(t);
        }

        return mTrajectory[mCurrentOrbit]->calcCoord(t, startJD);
    }

	void Trajectory::setStartJD(double t)
	{
		mLastOrbitChange = t;
	}
}