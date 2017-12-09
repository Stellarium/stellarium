/*!
 * \file MathPlugin.h
 *
 * \author Roger James
 * \date 13th November 2013
 *
 */

#pragma once

#include "InMemoryDatabase.h"

namespace INDI
{
namespace AlignmentSubsystem
{
/*!
 * \class MathPlugin
 * \brief Provides alignment subsystem functions to INDI alignment math plugins
 *
 * \note This class is intended to be implemented within a dynamic shared object. If the
 * implementation of this class uses a standard 3 by 3 transformation matrix to convert between coordinate systems
 * then it will not normally need to know the handedness of either the celestial or telescope coordinate systems, as the
 * necessary rotations and scaling will be handled in the derivation of the matrix coefficients. This will normally
 * be done using the three reference (sync) points method. Knowledge of the handedness of the coordinate systems is needed
 * when only two reference points are available and a third reference point has to artificially generated in order to
 * derive the matrix coefficients.
 */
class MathPlugin
{
  public:
    /// \brief Default constructor
    MathPlugin(MountAlignment_t ApproximateAlignment = ZENITH)
        : ApproximateMountAlignment(ApproximateAlignment), pInMemoryDatabase(NULL)
    {
    }

    /// \brief Virtual destructor
    virtual ~MathPlugin() {}

    // Public methods
    /// \brief Get the approximate alognment of the mount
    /// \return the approximate alignment
    virtual MountAlignment_t GetApproximateMountAlignment() { return ApproximateMountAlignment; }

    /// \brief Initialise or re-initialise the math plugin. Re-reading the in memory database as necessary.
    /// \return True if successful
    virtual bool Initialise(InMemoryDatabase *pInMemoryDatabase);

    /// \brief Set the approximate alognment of the mount
    /// \param[in] ApproximateAlignment - the approximate alignment of the mount
    virtual void SetApproximateMountAlignment(MountAlignment_t ApproximateAlignment)
    {
        ApproximateMountAlignment = ApproximateAlignment;
    }

    /// \brief Get the alignment corrected telescope pointing direction for the supplied celestial coordinates
    /// \param[in] RightAscension Right Ascension (Decimal Hours).
    /// \param[in] Declination Declination (Decimal Degrees).
    /// \param[in] JulianOffset to be applied to the current julian date.
    /// \param[out] ApparentTelescopeDirectionVector Parameter to receive the corrected telescope direction
    /// \return True if successful
    virtual bool TransformCelestialToTelescope(const double RightAscension, const double Declination,
                                               double JulianOffset,
                                               TelescopeDirectionVector &ApparentTelescopeDirectionVector) = 0;

    /// \brief Get the true celestial coordinates for the supplied telescope pointing direction
    /// \param[in] ApparentTelescopeDirectionVector the telescope direction
    /// \param[out] RightAscension Parameter to receive the Right Ascension (Decimal Hours).
    /// \param[out] Declination Parameter to receive the Declination (Decimal Degrees).
    /// \return True if successful
    virtual bool TransformTelescopeToCelestial(const TelescopeDirectionVector &ApparentTelescopeDirectionVector,
                                               double &RightAscension, double &Declination) = 0;

  protected:
    // Protected properties
    /// \brief Describe the approximate alignment of the mount. This information is normally used in a one star alignment
    /// calculation.
    MountAlignment_t ApproximateMountAlignment;
    InMemoryDatabase *pInMemoryDatabase;
};

} // namespace AlignmentSubsystem
} // namespace INDI
