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
    Trajectory::Trajectory(const std::string& TrajectoryFile, const SolarSystem *SS) : mLastOrbitChange(0.0), mCurrentOrbit(0)
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
					straightCalc = new StraightCoordCalc(Vec3d(x, y, z), vel, 0.0f, 1.0f );
				}
				else if (direction == "from")
				{
					straightCalc = new StraightCoordCalc(Vec3d(x, y, z), vel, 0.0f, -1.0f );
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
				double t1 = 0.0f;
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

				double t1 = 0.0f;
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
					straightCalc = new DurationStraightCoordCalc(Vec3d(x, y, z), dur, 0.0f, 1.0f );
				}
				else if (direction == "from")
				{
					straightCalc = new DurationStraightCoordCalc(Vec3d(x, y, z), dur, 0.0f, -1.0f );
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
			else
			{
				throw invalid_argument("Bad trajectory file format");
			}

		}
		/*const double c_obl = 1.0;
		const double s_obl = 0.0;
		const double c_nod = 1.0;
		const double s_nod = 0.0;
		  rotate_to_vsop87[0] =  c_nod;
		  rotate_to_vsop87[1] = -s_nod * c_obl;
		  rotate_to_vsop87[2] =  s_nod * s_obl;
		  rotate_to_vsop87[3] =  s_nod;
		  rotate_to_vsop87[4] =  c_nod * c_obl;
		  rotate_to_vsop87[5] = -c_nod * s_obl;
		  rotate_to_vsop87[6] =  0.0;
		  rotate_to_vsop87[7] =          s_obl;
		rotate_to_vsop87[8] =          c_obl;*/
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
        }
        Vec3d pos = mTrajectory[mCurrentOrbit]->calcCoord(t, startJD);//, v;
		/*v[0] = rotate_to_vsop87[0]*pos[2] + rotate_to_vsop87[1]*pos[0] + rotate_to_vsop87[2]*pos[1];
		v[1] = rotate_to_vsop87[3]*pos[2] + rotate_to_vsop87[4]*pos[0] + rotate_to_vsop87[5]*pos[1];
		v[2] = rotate_to_vsop87[6]*pos[2] + rotate_to_vsop87[7]*pos[0] + rotate_to_vsop87[8]*pos[1];*/
		/*v[0] = rotate_to_vsop87[0]*pos[0] + rotate_to_vsop87[1]*pos[1] + rotate_to_vsop87[2]*pos[2];
		v[1] = rotate_to_vsop87[3]*pos[0] + rotate_to_vsop87[4]*pos[1] + rotate_to_vsop87[5]*pos[2];
		v[2] = rotate_to_vsop87[6]*pos[0] + rotate_to_vsop87[7]*pos[1] + rotate_to_vsop87[8]*pos[2];*/
        
        return pos;
    }
}