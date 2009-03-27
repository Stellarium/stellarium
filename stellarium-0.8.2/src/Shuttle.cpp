#include "Shuttle.h"

//#define TRAJECTORY_PATH "stellarium\\data\\trajectory.ini"
//#define TRAJECTORY_PATH "F:\\stel\\stellarium\\stellarium\\data\\trajectory.ini"

using namespace MotionTestImpl;

namespace 
{
    bool firstTime = true; //Lame, I guess... :)
    const double startJD = 2451514.250011573;
	//double startJD = 0.0;

    const float coef = 1.0f;
}

Shuttle::Shuttle(Planet *parent,
           const string& englishName,
           int flagHalo,
           int flag_lighting,
           double radius,
           double oblateness,
           Vec3f color,
           float albedo,
           const string& tex_map_name,
           const string& tex_halo_name,
           pos_func_type _coord_func,
           OsulatingFunctType *osculating_func,
		   bool hidden,
		   bool flag_bump,
		   const string& tex_norm_name,
		   bool flag_night,
		   const string& tex_night_name,
		   const string& tex_specular_name,
		   bool flag_cloud,
		   const string& tex_cloud_name,
		   const string& tex_shadow_cloud_name,
		   const string& tex_norm_cloud_name,
           bool flag_noise,
           SolarSystem *s, const std::string& dataDir, 
		   const std::string& trajectoryFile) : Planet(parent,
           englishName,
           flagHalo,
           flag_lighting,
           radius,
           oblateness,
           color,
           albedo,
           tex_map_name,
           tex_halo_name,
           _coord_func,
           osculating_func,
		   hidden,
		   flag_bump,
		   tex_norm_name,
		   flag_night,
		   tex_night_name,
		   tex_specular_name,
		   flag_cloud,
		   tex_cloud_name,
		   tex_shadow_cloud_name,
		   tex_norm_cloud_name, flag_noise), trajectory(dataDir + trajectoryFile, s)
{
    ssystem = s;
}

void Shuttle::compute_position(const double date)
{
    if (/*fabs(startJD) < 0.000000001 || */firstTime)
    {
        //startJD = date;
		//trajectory.setStartJD(date);
        firstTime = false;
		return;
    }
	if (date < startJD)
		return;
	//Vec3f pos = trajectory.calcCoord( (date - startJD) * coef, startJD );
    Vec3f pos = trajectory.calcCoord( date * coef, 0.0 );
	ecliptic_pos = Vec3d(pos[0], pos[1], pos[2]);
}
Shuttle::~Shuttle(void)
{
}
