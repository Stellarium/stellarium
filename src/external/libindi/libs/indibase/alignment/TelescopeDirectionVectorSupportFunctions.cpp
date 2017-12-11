/*!
 * \file TelescopeDirectionVectorSupportFunctions.cpp
 *
 * \author Roger James
 * \date 13th November 2013
 *
 */

#include "TelescopeDirectionVectorSupportFunctions.h"

namespace INDI
{
namespace AlignmentSubsystem
{
void TelescopeDirectionVectorSupportFunctions::SphericalCoordinateFromTelescopeDirectionVector(
    const TelescopeDirectionVector TelescopeDirectionVector, double &AzimuthAngle,
    AzimuthAngleDirection AzimuthAngleDirection, double &PolarAngle, PolarAngleDirection PolarAngleDirection)
{
    if (ANTI_CLOCKWISE == AzimuthAngleDirection)
    {
        if (FROM_AZIMUTHAL_PLANE == PolarAngleDirection)
        {
            AzimuthAngle = atan2(TelescopeDirectionVector.y, TelescopeDirectionVector.x);
            PolarAngle   = asin(TelescopeDirectionVector.z);
        }
        else
        {
            AzimuthAngle = atan2(TelescopeDirectionVector.y, TelescopeDirectionVector.x);
            PolarAngle   = acos(TelescopeDirectionVector.z);
        }
    }
    else
    {
        if (FROM_AZIMUTHAL_PLANE == PolarAngleDirection)
        {
            AzimuthAngle = atan2(-TelescopeDirectionVector.y, TelescopeDirectionVector.x);
            PolarAngle   = asin(TelescopeDirectionVector.z);
        }
        else
        {
            AzimuthAngle = atan2(-TelescopeDirectionVector.y, TelescopeDirectionVector.x);
            PolarAngle   = acos(TelescopeDirectionVector.z);
        }
    }
}

const TelescopeDirectionVector
TelescopeDirectionVectorSupportFunctions::TelescopeDirectionVectorFromSphericalCoordinate(
    const double AzimuthAngle, AzimuthAngleDirection AzimuthAngleDirection, const double PolarAngle,
    PolarAngleDirection PolarAngleDirection)
{
    TelescopeDirectionVector Vector;

    if (ANTI_CLOCKWISE == AzimuthAngleDirection)
    {
        if (FROM_AZIMUTHAL_PLANE == PolarAngleDirection)
        {
            Vector.x = cos(PolarAngle) * cos(AzimuthAngle);
            Vector.y = cos(PolarAngle) * sin(AzimuthAngle);
            Vector.z = sin(PolarAngle);
        }
        else
        {
            Vector.x = sin(PolarAngle) * sin(AzimuthAngle);
            Vector.y = sin(PolarAngle) * cos(AzimuthAngle);
            Vector.z = cos(PolarAngle);
        }
    }
    else
    {
        if (FROM_AZIMUTHAL_PLANE == PolarAngleDirection)
        {
            Vector.x = cos(PolarAngle) * cos(-AzimuthAngle);
            Vector.y = cos(PolarAngle) * sin(-AzimuthAngle);
            Vector.z = sin(PolarAngle);
        }
        else
        {
            Vector.x = sin(PolarAngle) * sin(-AzimuthAngle);
            Vector.y = sin(PolarAngle) * cos(-AzimuthAngle);
            Vector.z = cos(PolarAngle);
        }
    }

    return Vector;
}

} // namespace AlignmentSubsystem
} // namespace INDI
