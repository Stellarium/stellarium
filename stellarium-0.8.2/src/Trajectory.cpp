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

	//const double startJD = 2451514.250011573;

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
		mCurrentOrbit(0), startJD(2451514.250011573), mLastOrbitChange(2451514.250011573)
	{
		mTrajectory.push_back(new StaticCoordCalc(Vec3d(10.0, 10.0, 10.0)));
		mDuration.push_back(0.5 / coeff);

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
			else if (buf == "date")
			{
				// Format : date 2007-06-01T12:00:00
				ln_date date;
				char delim;

				file >> date.years >> delim >> date.months >> delim >> date.days >> delim >> 
					date.hours >> delim >>  date.minutes >> delim >> date.seconds;
				mLastOrbitChange = startJD = get_julian_day(&date);
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
			else if (buf == "flyto")
			{
				double dur, q, delta;
				string planet;
				
				file >> planet >> dur >> q;

				dur /= coeff;
				
				Planet* planetPtr = SS->searchByEnglishName(planet);
				
				GCPtr<CoordCalc> straightCalc;

				double t1 = startJD;
				for(vector<double>::const_iterator it = mDuration.begin(); it != mDuration.end(); ++it)
					t1 += *it;
								
				Vec3d start = mTrajectory.back()->calcCoord(t1) - 
					planetPtr->get_heliocentric_ecliptic_pos(t1);

				//delta = q * dur / (1 - q);
				//delta = dur / pow(1.0 - q, 10.0) - dur;
				delta = dur * cPi / acos(2.0 * q - 1.0) - dur;
				
				straightCalc = new DurationStraightCoordCalc(start, dur + delta, 1.0f);
				
                GCPtr<CoordCalc> planetCoordCalc = new PlanetCoordCalc(planetPtr);
				posCalc = new LocalSystemCoordCalc(straightCalc, planetCoordCalc);
				posCalc->setStartTime(t1);

				mTrajectory.push_back(posCalc);
				mDuration.push_back(dur);
			}
			else if (buf == "flyfrom")
			{
				double dur;
				string planet;
				
				file >> planet >> dur;

				dur /= coeff;
				
				Planet* planetPtr = SS->searchByEnglishName(planet);
				
				GCPtr<CoordCalc> straightCalc;

				double t1 = startJD;
				for(vector<double>::const_iterator it = mDuration.begin(); it != mDuration.end(); ++it)
					t1 += *it;
								
				Vec3d start = mTrajectory.back()->calcCoord(t1) - 
					planetPtr->get_heliocentric_ecliptic_pos(t1);
				
				straightCalc = new DurationStraightCoordCalc(start, dur, -1.0f);
				
                GCPtr<CoordCalc> planetCoordCalc = new PlanetCoordCalc(planetPtr);
				posCalc = new LocalSystemCoordCalc(straightCalc, planetCoordCalc);
				posCalc->setStartTime(t1);

				mTrajectory.push_back(posCalc);
				mDuration.push_back(dur);
			}
			else if (buf == "fly")
			{
				double x;
				file >> buf >> x;
                
				mTrajectory.push_back( FindOrbit(buf)->GetCoordFunc() );
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
				Vec3d vf = ellipseCalc->calcCoord(t2);

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
							new DurationStraightCoordCalc(vk - vf, duration, 1.0f), 
							new StaticCoordCalc(vf)
						), 
						ptr->getCoordCalcSystem()
					);
				posCalc->setStartTime(t1);

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

				GCPtr<CoordCalc> posCalc = new StaticCoordCalc(
					mTrajectory.back()->calcCoord(t1));
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
    
    Vec3d Trajectory::calcCoord(double t)
    {             
        if ( (t - mLastOrbitChange) >= mDuration[mCurrentOrbit] )
        {
            mCurrentOrbit++;
            mLastOrbitChange = t;
            if (mCurrentOrbit == mTrajectory.size())
            {
                GCPtr<CoordCalc> posCalc = new StaticCoordCalc(mTrajectory[mCurrentOrbit - 1]->calcCoord(t));
                mTrajectory.push_back(posCalc);
                mDuration.push_back(FLT_MAX);
            }
			mTrajectory[mCurrentOrbit]->setStartTime(t);
        }

        return mTrajectory[mCurrentOrbit]->calcCoord(t);
    }

	double Trajectory::getStartJD() const
	{
		return startJD;
	}
}