#pragma once
#include <string>
#include "planet.h"
#include "solarsystem.h"
#include "trajectory.h"

class Shuttle : public Planet
{
private:
    SolarSystem *ssystem;
    MotionTestImpl::Trajectory trajectory;
public:
    Shuttle(Planet *parent,
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
		   SolarSystem *s, const std::string& dataDir, const std::string& trajectoryFile);
    virtual void compute_position(const double date);
public:
    virtual ~Shuttle(void);
};
