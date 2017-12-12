/*!
 * \file Common.h
 *
 * \author Roger James
 * \date 13th November 2013
 *
 */

#pragma once

#include <cstring>
#include <cmath>
#include <memory>

/// \defgroup AlignmentSubsystem INDI Alignment Subsystem

namespace INDI
{
/// \namespace INDI::AlignmentSubsystem
/// \brief Namespace to encapsulate the INDI Alignment Subsystem classes.
/// For more information see "INDI Alignment Subsystem" in "Related Pages" accessible via the banner at the
/// top of this page.
/// \ingroup AlignmentSubsystem
namespace AlignmentSubsystem
{
/** \enum MountAlignment
    \brief Describe the alignment of a telescope axis. This is normally used to differentiate between
    equatorial mounts in differnet hemispheres and altaz or dobsonian mounts.
*/
typedef enum MountAlignment { ZENITH, NORTH_CELESTIAL_POLE, SOUTH_CELESTIAL_POLE } MountAlignment_t;

/// \enum AlignmentDatabaseActions
/// \brief Action to perform on Alignment Database
enum AlignmentDatabaseActions
{
    APPEND,
    INSERT,
    EDIT,
    DELETE,
    CLEAR,
    READ,
    READ_INCREMENT,
    LOAD_DATABASE,
    SAVE_DATABASE
};

/// \enum AlignmentPointSetEnum
/// \brief The offsets to the fields in the alignment point set property
/// \note This must match the definitions given to INDI
enum AlignmentPointSetEnum
{
    ENTRY_OBSERVATION_JULIAN_DATE,
    ENTRY_RA,
    ENTRY_DEC,
    ENTRY_VECTOR_X,
    ENTRY_VECTOR_Y,
    ENTRY_VECTOR_Z
};

/*!
 * \struct TelescopeDirectionVector
 * \brief Holds a nomalised direction vector (direction cosines)
 *
 * The x y,z fields of this class should normally represent a normalised (unit length)
 * vector in a right handed rectangular coordinate space. However, for convenience a number
 * a number of standard 3d vector methods are also supported.
 */
struct TelescopeDirectionVector
{
    /// \brief Default constructor
    TelescopeDirectionVector() : x(0), y(0), z(0) {}

    /// \brief Copy constructor
    TelescopeDirectionVector(double X, double Y, double Z) : x(X), y(Y), z(Z) {}

    double x;
    double y;
    double z;

    /// \brief Override the * operator to return a cross product
    inline const TelescopeDirectionVector operator*(const TelescopeDirectionVector &RHS) const
    {
        TelescopeDirectionVector Result;

        Result.x = y * RHS.z - z * RHS.y;
        Result.y = z * RHS.x - x * RHS.z;
        Result.z = x * RHS.y - y * RHS.x;
        return Result;
    }

    /// \brief Override the * operator to return a scalar product
    inline const TelescopeDirectionVector operator*(const double &RHS) const
    {
        TelescopeDirectionVector Result;

        Result.x = x * RHS;
        Result.y = y * RHS;
        Result.z = z * RHS;
        return Result;
    }

    /// \brief Override the *= operator to return a  unary scalar product
    inline const TelescopeDirectionVector &operator*=(const double &RHS)
    {
        x *= RHS;
        y *= RHS;
        z *= RHS;
        return *this;
    }

    /// \brief Override the - operator to return a binary vector subtract
    inline const TelescopeDirectionVector operator-(const TelescopeDirectionVector &RHS) const
    {
        return TelescopeDirectionVector(x - RHS.x, y - RHS.y, z - RHS.z);
    }

    /// \brief Override the ^ operator to return a dot product
    inline double operator^(const TelescopeDirectionVector &RHS) const { return x * RHS.x + y * RHS.y + z * RHS.z; }

    /// \brief Return the length of the vector
    /// \return Length of the vector
    inline double Length() const { return sqrt(x * x + y * y + z * z); }

    /// \brief Normalise the vector
    inline void Normalise()
    {
        double length = sqrt(x * x + y * y + z * z);
        x /= length;
        y /= length;
        z /= length;
    }

    /// \brief Rotate the reference frame around the Y axis. This has the affect of rotating the vector
    /// itself in the opposite direction
    /// \param[in] Angle The angle to rotate the reference frame by. Positive values give an anti-clockwise
    /// rotation from the perspective of looking down the positive axis towards the origin.
    void RotateAroundY(double Angle);
};

/*!
 * \struct AlignmentDatabaseEntry
 * \brief Entry in the in memory alignment database
 *
 */
struct AlignmentDatabaseEntry
{
    /// \brief Default constructor
    AlignmentDatabaseEntry() : ObservationJulianDate(0), RightAscension(0), Declination(0), PrivateDataSize(0) {}

    /// \brief Copy constructor
    AlignmentDatabaseEntry(const AlignmentDatabaseEntry &Source)
        : ObservationJulianDate(Source.ObservationJulianDate), RightAscension(Source.RightAscension),
          Declination(Source.Declination), TelescopeDirection(Source.TelescopeDirection),
          PrivateDataSize(Source.PrivateDataSize)
    {
        if (0 != PrivateDataSize)
        {
            PrivateData.reset(new unsigned char[PrivateDataSize]);
            memcpy(PrivateData.get(), Source.PrivateData.get(), PrivateDataSize);
        }
    }

    /// Override the assignment operator to provide a const version
    inline const AlignmentDatabaseEntry &operator=(const AlignmentDatabaseEntry &RHS)
    {
        ObservationJulianDate = RHS.ObservationJulianDate;
        RightAscension        = RHS.RightAscension;
        Declination           = RHS.Declination;
        TelescopeDirection    = RHS.TelescopeDirection;
        PrivateDataSize       = RHS.PrivateDataSize;
        if (0 != PrivateDataSize)
        {
            PrivateData.reset(new unsigned char[PrivateDataSize]);
            memcpy(PrivateData.get(), RHS.PrivateData.get(), PrivateDataSize);
        }

        return *this;
    }

    double ObservationJulianDate;

    /// \brief Right ascension in decimal hours. N.B. libnova works in decimal degrees
    /// so conversion is always needed!
    double RightAscension;

    /// \brief Declination in decimal degrees
    double Declination;

    /// \brief Normalised vector giving telescope pointing direction.
    /// This is referred to elsewhere as the "apparent" direction.
    TelescopeDirectionVector TelescopeDirection;

    /// \brief Private data associated with this sync point
    std::unique_ptr<unsigned char> PrivateData;

    /// \brief This size in bytes of any private data
    int PrivateDataSize;
};

} // namespace AlignmentSubsystem
} // namespace INDI
