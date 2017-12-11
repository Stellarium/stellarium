/// \file SVDMathPlugin.h
///
/// \author Roger James
/// \date 13th November 2013
///
/// This file provides the Singular Value Decomposition (Markley) math plugin functionality

#pragma once

#include "BasicMathPlugin.h"

namespace INDI
{
namespace AlignmentSubsystem
{
/*!
 * \class SVDMathPlugin
 * \brief This class implements the SVD math plugin.
 */
class SVDMathPlugin : public BasicMathPlugin
{
  private:
    /// \brief Calculate tranformation matrices from the supplied vectors
    /// \param[in] Alpha1 Pointer to the first coordinate in the alpha reference frame
    /// \param[in] Alpha2 Pointer to the second coordinate in the alpha reference frame
    /// \param[in] Alpha3 Pointer to the third coordinate in the alpha reference frame
    /// \param[in] Beta1 Pointer to the first coordinate in the beta reference frame
    /// \param[in] Beta2 Pointer to the second coordinate in the beta reference frame
    /// \param[in] Beta3 Pointer to the third coordinate in the beta reference frame
    /// \param[in] pAlphaToBeta Pointer to a matrix to receive the Alpha to Beta transformation matrix
    /// \param[in] pBetaToAlpha Pointer to a matrix to receive the Beta to Alpha transformation matrix
    void CalculateTransformMatrices(const TelescopeDirectionVector &Alpha1, const TelescopeDirectionVector &Alpha2,
                                    const TelescopeDirectionVector &Alpha3, const TelescopeDirectionVector &Beta1,
                                    const TelescopeDirectionVector &Beta2, const TelescopeDirectionVector &Beta3,
                                    gsl_matrix *pAlphaToBeta, gsl_matrix *pBetaToAlpha);
};

} // namespace AlignmentSubsystem
} // namespace INDI
