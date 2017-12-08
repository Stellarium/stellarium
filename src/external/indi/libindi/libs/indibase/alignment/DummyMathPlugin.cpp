/// \file DummyMathPlugin.cpp
/// \author Roger James
/// \date 13th November 2013

#include "DummyMathPlugin.h"

namespace INDI
{
namespace AlignmentSubsystem
{
// Standard functions required for all plugins
extern "C" {
DummyMathPlugin *Create()
{
    return new DummyMathPlugin;
}

void Destroy(DummyMathPlugin *pPlugin)
{
    delete pPlugin;
}

const char *GetDisplayName()
{
    return "Dummy Math Plugin";
}
}

DummyMathPlugin::DummyMathPlugin()
{
    //ctor
}

DummyMathPlugin::~DummyMathPlugin()
{
    //dtor
}

bool DummyMathPlugin::Initialise(InMemoryDatabase *pInMemoryDatabase)
{
    // Call the base class to initialise to in in memory database pointer
    MathPlugin::Initialise(pInMemoryDatabase);

    return false;
}

bool DummyMathPlugin::TransformCelestialToTelescope(const double RightAscension, const double Declination,
                                                    double JulianOffset,
                                                    TelescopeDirectionVector &ApparentTelescopeDirectionVector)
{
    return false;
}

bool DummyMathPlugin::TransformTelescopeToCelestial(const TelescopeDirectionVector &ApparentTelescopeDirectionVector,
                                                    double &RightAscension, double &Declination)
{
    return false;
}

} // namespace AlignmentSubsystem
} // namespace INDI
