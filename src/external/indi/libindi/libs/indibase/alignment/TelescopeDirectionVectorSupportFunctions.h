/*!
 * \file TelescopeDirectionVectorSupportFunctions.h
 *
 * \author Roger James
 * \date 13th November 2013
 *
 */

#pragma once

#include "Common.h"

#include <libnova/ln_types.h>
#include <libnova/transform.h>
#include <libnova/utility.h>

namespace INDI
{
namespace AlignmentSubsystem
{
/*! \class TelescopeDirectionVectorSupportFunctions
 *  \brief These functions are used to convert different coordinate systems to and from the
 *  telescope direction vectors (normalised vector/direction cosines) used for telescope coordinates in the
 *  alignment susbsystem.
 */
class TelescopeDirectionVectorSupportFunctions
{
  public:
    /// \brief Virtual destructor
    virtual ~TelescopeDirectionVectorSupportFunctions() {}

    /*!
         * \enum AzimuthAngleDirection
         * The direction of measurement of an azimuth angle.
         * The following are the conventions for some coordinate systems.
         * - Right Ascension is measured ANTI_CLOCKWISE from the vernal equinox.
         * - Local Hour Angle is measured CLOCKWISE from the observer's meridian.
         * - Greenwich Hour Angle is measured CLOCKWISE from the Greenwich meridian.
         * - Azimuth (as in Altitude Azimuth coordinate systems ) is often measured CLOCKWISE\n
         * from north. But ESO FITS (Clockwise from South) and SDSS FITS(Anticlockwise from South)\n
         * have different conventions. Horizontal coordinates in libnova are measured clockwise from south.
         */
    typedef enum AzimuthAngleDirection {
        CLOCKWISE,     /*!< Angle is measured clockwise */
        ANTI_CLOCKWISE /*!< Angle is measured anti clockwise */
    } AzimuthAngleDirection_t;

    /*!
         * \enum PolarAngleDirection
         * The direction of measurement of a polar angle.
         * The following are conventions for some coordinate systems
         * - Declination is measured FROM_AZIMUTHAL_PLANE.
         * - Altitude is measured FROM_AZIMUTHAL_PLANE.
         * - Altitude in libnova horizontal coordinates is measured FROM_AZIMUTHAL_PLANE.
         */
    typedef enum PolarAngleDirection {
        FROM_POLAR_AXIS,     /*!< Angle is measured down from the polar axis */
        FROM_AZIMUTHAL_PLANE /*!< Angle is measured upwards from the azimuthal plane */
    } PolarAngleDirection_t;

    /*! \brief Calculates an altitude and azimuth from the supplied normalised direction vector
         * and declination.
         * \param[in] TelescopeDirectionVector
         * \param[out] HorizontalCoordinates Altitude and Azimuth in decimal degrees
         * \note This assumes a right handed coordinate system for the telescope direction vector with XY being the azimuthal plane,
         * and azimuth being measured in a clockwise direction.
         */
    void AltitudeAzimuthFromTelescopeDirectionVector(const TelescopeDirectionVector TelescopeDirectionVector,
                                                     ln_hrz_posn &HorizontalCoordinates)
    {
        double AzimuthAngle;
        double AltitudeAngle;
        SphericalCoordinateFromTelescopeDirectionVector(TelescopeDirectionVector, AzimuthAngle, CLOCKWISE,
                                                        AltitudeAngle, FROM_AZIMUTHAL_PLANE);
        HorizontalCoordinates.az  = ln_rad_to_deg(AzimuthAngle);
        HorizontalCoordinates.alt = ln_rad_to_deg(AltitudeAngle);
    };

    /*! \brief Calculates an altitude and azimuth from the supplied normalised direction vector
         * and declination.
         * \param[in] TelescopeDirectionVector
         * \param[out] HorizontalCoordinates Altitude and Azimuth in degrees minutes seconds
         * \note This assumes a right handed coordinate system for the telescope direction vector with XY being the azimuthal plane,
         * and azimuth being measured in a clockwise direction.
         */
    void AltitudeAzimuthFromTelescopeDirectionVector(const TelescopeDirectionVector TelescopeDirectionVector,
                                                     lnh_hrz_posn &HorizontalCoordinates)
    {
        double AzimuthAngle;
        double AltitudeAngle;
        SphericalCoordinateFromTelescopeDirectionVector(TelescopeDirectionVector, AzimuthAngle, CLOCKWISE,
                                                        AltitudeAngle, FROM_AZIMUTHAL_PLANE);
        ln_rad_to_dms(AzimuthAngle, &HorizontalCoordinates.az);
        ln_rad_to_dms(AltitudeAngle, &HorizontalCoordinates.alt);
    };
    /*! \brief Calculates equatorial coordinates from the supplied telescope direction vector
         * and declination.
         * \param[in] TelescopeDirectionVector
         * \param[out] EquatorialCoordinates The equatorial coordinates in decimal degrees
         * \note This assumes a right handed coordinate system for the direction vector with the right ascension being in the XY plane.
         */
    void EquatorialCoordinatesFromTelescopeDirectionVector(const TelescopeDirectionVector TelescopeDirectionVector,
                                                           struct ln_equ_posn &EquatorialCoordinates)
    {
        double AzimuthAngle;
        double PolarAngle;
        SphericalCoordinateFromTelescopeDirectionVector(TelescopeDirectionVector, AzimuthAngle, ANTI_CLOCKWISE,
                                                        PolarAngle, FROM_AZIMUTHAL_PLANE);
        EquatorialCoordinates.ra  = ln_rad_to_deg(AzimuthAngle);
        EquatorialCoordinates.dec = ln_rad_to_deg(PolarAngle);
    };

    /*! \brief Calculates equatorial coordinates from the supplied telescope direction vector
         * and declination.
         * \param[in] TelescopeDirectionVector
         * \param[out] EquatorialCoordinates The equatorial coordinates in hours minutes seconds and degrees minutes seconds
         * \note This assumes a right handed coordinate system for the direction vector with the right ascension being in the XY plane.
         */
    void EquatorialCoordinatesFromTelescopeDirectionVector(const TelescopeDirectionVector TelescopeDirectionVector,
                                                           struct lnh_equ_posn &EquatorialCoordinates)
    {
        double AzimuthAngle;
        double PolarAngle;
        SphericalCoordinateFromTelescopeDirectionVector(TelescopeDirectionVector, AzimuthAngle, ANTI_CLOCKWISE,
                                                        PolarAngle, FROM_AZIMUTHAL_PLANE);
        ln_rad_to_hms(AzimuthAngle, &EquatorialCoordinates.ra);
        ln_rad_to_dms(PolarAngle, &EquatorialCoordinates.dec);
    };

    /*! \brief Calculates a local hour angle and declination from the supplied telescope direction vector
         * and declination.
         * \param[in] TelescopeDirectionVector
         * \param[out] EquatorialCoordinates The local hour angle and declination in decimal degrees
         * \note This assumes a right handed coordinate system for the direction vector with the hour angle being in the XY plane.
         */
    void LocalHourAngleDeclinationFromTelescopeDirectionVector(const TelescopeDirectionVector TelescopeDirectionVector,
                                                               struct ln_equ_posn &EquatorialCoordinates)
    {
        double AzimuthAngle;
        double PolarAngle;
        SphericalCoordinateFromTelescopeDirectionVector(TelescopeDirectionVector, AzimuthAngle, CLOCKWISE, PolarAngle,
                                                        FROM_AZIMUTHAL_PLANE);
        EquatorialCoordinates.ra  = ln_rad_to_deg(AzimuthAngle);
        EquatorialCoordinates.dec = ln_rad_to_deg(PolarAngle);
    };

    /*! \brief Calculates a local hour angle and declination from the supplied telescope direction vector
         * and declination.
         * \param[in] TelescopeDirectionVector
         * \param[out] EquatorialCoordinates The local hour angle and declination in hours minutes seconds and degrees minutes seconds
         * \note This assumes a right handed coordinate system for the direction vector with the hour angle being in the XY plane.
         */
    void LocalHourAngleDeclinationFromTelescopeDirectionVector(const TelescopeDirectionVector TelescopeDirectionVector,
                                                               struct lnh_equ_posn &EquatorialCoordinates)
    {
        double AzimuthAngle;
        double PolarAngle;
        SphericalCoordinateFromTelescopeDirectionVector(TelescopeDirectionVector, AzimuthAngle, CLOCKWISE, PolarAngle,
                                                        FROM_AZIMUTHAL_PLANE);
        ln_rad_to_hms(AzimuthAngle, &EquatorialCoordinates.ra);
        ln_rad_to_dms(PolarAngle, &EquatorialCoordinates.dec);
    };

    /*! \brief Calculates a spherical coordinate from the supplied telescope direction vector
         * \param[in] TelescopeDirectionVector
         * \param[out] AzimuthAngle The azimuth angle in radians
         * \param[in] AzimuthAngleDirection The direction the azimuth angle has been measured either CLOCKWISE or ANTI_CLOCKWISE
         * \param[out] PolarAngle The polar angle in radians
         * \param[in] PolarAngleDirection The direction the polar angle has been measured either FROM_POLAR_AXIS or FROM_AZIMUTHAL_PLANE
         * \note TelescopeDirectionVectors are always normalised and right handed.
         */
    void SphericalCoordinateFromTelescopeDirectionVector(const TelescopeDirectionVector TelescopeDirectionVector,
                                                         double &AzimuthAngle,
                                                         AzimuthAngleDirection_t AzimuthAngleDirection,
                                                         double &PolarAngle, PolarAngleDirection_t PolarAngleDirection);

    /*! \brief Calculates a normalised direction vector from the supplied altitude and azimuth.
         * \param[in] HorizontalCoordinates Altitude and Azimuth in decimal degrees
         * \return A TelescopeDirectionVector
         * \note This assumes a right handed coordinate system for the telescope direction vector with XY being the azimuthal plane,
         * and azimuth being measured in a clockwise direction.
         */
    const TelescopeDirectionVector TelescopeDirectionVectorFromAltitudeAzimuth(ln_hrz_posn HorizontalCoordinates)
    {
        return TelescopeDirectionVectorFromSphericalCoordinate(ln_deg_to_rad(HorizontalCoordinates.az), CLOCKWISE,
                                                               ln_deg_to_rad(HorizontalCoordinates.alt),
                                                               FROM_AZIMUTHAL_PLANE);
    };

    /*! \brief Calculates a normalised direction vector from the supplied altitude and azimuth.
         * \param[in] HorizontalCoordinates Altitude and Azimuth in degrees minutes seconds
         * \return A TelescopeDirectionVector
         * \note This assumes a right handed coordinate system for the telescope direction vector with XY being the azimuthal plane,
         * and azimuth being measured in a clockwise direction.
         */
    const TelescopeDirectionVector TelescopeDirectionVectorFromAltitudeAzimuth(lnh_hrz_posn HorizontalCoordinates)
    {
        return TelescopeDirectionVectorFromSphericalCoordinate(ln_dms_to_rad(&HorizontalCoordinates.az), CLOCKWISE,
                                                               ln_dms_to_rad(&HorizontalCoordinates.alt),
                                                               FROM_AZIMUTHAL_PLANE);
    };

    /*! \brief Calculates a telescope direction vector from the supplied equatorial coordinates.
         * \param[in] EquatorialCoordinates The equatorial coordinates in decimal degrees
         * \return A TelescopeDirectionVector
         * \note This assumes a right handed coordinate system for the direction vector with the right ascension being in the XY plane.
         */
    const TelescopeDirectionVector
    TelescopeDirectionVectorFromEquatorialCoordinates(struct ln_equ_posn EquatorialCoordinates)
    {
        return TelescopeDirectionVectorFromSphericalCoordinate(ln_deg_to_rad(EquatorialCoordinates.ra), ANTI_CLOCKWISE,
                                                               ln_deg_to_rad(EquatorialCoordinates.dec),
                                                               FROM_AZIMUTHAL_PLANE);
    };

    /*! \brief Calculates a telescope direction vector from the supplied equatorial coordinates.
         * \param[in] EquatorialCoordinates The equatorial coordinates in hours minutes seconds and degrees minutes seconds
         * \return A TelescopeDirectionVector
         * \note This assumes a right handed coordinate system for the direction vector with the right ascension being in the XY plane.
         */
    const TelescopeDirectionVector
    TelescopeDirectionVectorFromEquatorialCoordinates(struct lnh_equ_posn EquatorialCoordinates)
    {
        return TelescopeDirectionVectorFromSphericalCoordinate(ln_hms_to_rad(&EquatorialCoordinates.ra), ANTI_CLOCKWISE,
                                                               ln_dms_to_rad(&EquatorialCoordinates.dec),
                                                               FROM_AZIMUTHAL_PLANE);
    };

    /*! \brief Calculates a telescope direction vector from the supplied local hour angle and declination.
         * \param[in] EquatorialCoordinates The local hour angle and declination in decimal degrees
         * \return A TelescopeDirectionVector
         * \note This assumes a right handed coordinate system for the direction vector with the hour angle being in the XY plane.
         */
    const TelescopeDirectionVector
    TelescopeDirectionVectorFromLocalHourAngleDeclination(struct ln_equ_posn EquatorialCoordinates)
    {
        return TelescopeDirectionVectorFromSphericalCoordinate(ln_deg_to_rad(EquatorialCoordinates.ra), CLOCKWISE,
                                                               ln_deg_to_rad(EquatorialCoordinates.dec),
                                                               FROM_AZIMUTHAL_PLANE);
    };

    /*! \brief Calculates a telescope direction vector from the supplied spherical coordinate information
         * \param[in] AzimuthAngle The azimuth angle in radians
         * \param[in] AzimuthAngleDirection The direction the azimuth angle has been measured either CLOCKWISE or ANTI_CLOCKWISE
         * \param[in] PolarAngle The polar angle in radians
         * \param[in] PolarAngleDirection The direction the polar angle has been measured either FROM_POLAR_AXIS or FROM_AZIMUTHAL_PLANE
         * \return A TelescopeDirectionVector
         * \note TelescopeDirectionVectors are always assumed to be normalised and right handed.
         */
    const TelescopeDirectionVector
    TelescopeDirectionVectorFromSphericalCoordinate(const double AzimuthAngle,
                                                    AzimuthAngleDirection_t AzimuthAngleDirection,
                                                    const double PolarAngle, PolarAngleDirection_t PolarAngleDirection);
};

} // namespace AlignmentSubsystem
} // namespace INDI
