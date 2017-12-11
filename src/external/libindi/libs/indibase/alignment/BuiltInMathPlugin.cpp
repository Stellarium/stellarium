/// \file BuiltInMathPlugin.cpp
/// \author Roger James
/// \date 13th November 2013

#include "BuiltInMathPlugin.h"

#include "DriverCommon.h"

namespace INDI
{
namespace AlignmentSubsystem
{
// Private methods

void BuiltInMathPlugin::CalculateTransformMatrices(const TelescopeDirectionVector &Alpha1,
                                                   const TelescopeDirectionVector &Alpha2,
                                                   const TelescopeDirectionVector &Alpha3,
                                                   const TelescopeDirectionVector &Beta1,
                                                   const TelescopeDirectionVector &Beta2,
                                                   const TelescopeDirectionVector &Beta3, gsl_matrix *pAlphaToBeta,
                                                   gsl_matrix *pBetaToAlpha)
{
    // Derive the Actual to Apparent transformation matrix
    gsl_matrix *pAlphaMatrix = gsl_matrix_alloc(3, 3);
    gsl_matrix_set(pAlphaMatrix, 0, 0, Alpha1.x);
    gsl_matrix_set(pAlphaMatrix, 1, 0, Alpha1.y);
    gsl_matrix_set(pAlphaMatrix, 2, 0, Alpha1.z);
    gsl_matrix_set(pAlphaMatrix, 0, 1, Alpha2.x);
    gsl_matrix_set(pAlphaMatrix, 1, 1, Alpha2.y);
    gsl_matrix_set(pAlphaMatrix, 2, 1, Alpha2.z);
    gsl_matrix_set(pAlphaMatrix, 0, 2, Alpha3.x);
    gsl_matrix_set(pAlphaMatrix, 1, 2, Alpha3.y);
    gsl_matrix_set(pAlphaMatrix, 2, 2, Alpha3.z);

    Dump3x3("AlphaMatrix", pAlphaMatrix);

    gsl_matrix *pBetaMatrix = gsl_matrix_alloc(3, 3);
    gsl_matrix_set(pBetaMatrix, 0, 0, Beta1.x);
    gsl_matrix_set(pBetaMatrix, 1, 0, Beta1.y);
    gsl_matrix_set(pBetaMatrix, 2, 0, Beta1.z);
    gsl_matrix_set(pBetaMatrix, 0, 1, Beta2.x);
    gsl_matrix_set(pBetaMatrix, 1, 1, Beta2.y);
    gsl_matrix_set(pBetaMatrix, 2, 1, Beta2.z);
    gsl_matrix_set(pBetaMatrix, 0, 2, Beta3.x);
    gsl_matrix_set(pBetaMatrix, 1, 2, Beta3.y);
    gsl_matrix_set(pBetaMatrix, 2, 2, Beta3.z);

    Dump3x3("BetaMatrix", pBetaMatrix);

    // Use the quick and dirty method
    // This can result in matrices which are not true transforms
    gsl_matrix *pInvertedAlphaMatrix = gsl_matrix_alloc(3, 3);

    if (!MatrixInvert3x3(pAlphaMatrix, pInvertedAlphaMatrix))
    {
        // pAlphaMatrix is singular and therefore is not a true transform
        // and cannot be inverted. This probably means it contains at least
        // one row or column that contains only zeroes
        gsl_matrix_set_identity(pInvertedAlphaMatrix);
        ASSDEBUG("CalculateTransformMatrices - Alpha matrix is singular!");
        IDMessage(nullptr, "Alpha matrix is singular and cannot be inverted.");
    }
    else
    {
        MatrixMatrixMultiply(pBetaMatrix, pInvertedAlphaMatrix, pAlphaToBeta);

        Dump3x3("AlphaToBeta", pAlphaToBeta);

        if (nullptr != pBetaToAlpha)
        {
            // Invert the matrix to get the Apparent to Actual transform
            if (!MatrixInvert3x3(pAlphaToBeta, pBetaToAlpha))
            {
                // pAlphaToBeta is singular and therefore is not a true transform
                // and cannot be inverted. This probably means it contains at least
                // one row or column that contains only zeroes
                gsl_matrix_set_identity(pBetaToAlpha);
                ASSDEBUG("CalculateTransformMatrices - AlphaToBeta matrix is singular!");
                IDMessage(
                    nullptr,
                    "Calculated Celestial to Telescope transformation matrix is singular (not a true transform).");
            }

            Dump3x3("BetaToAlpha", pBetaToAlpha);
        }
    }

    // Clean up
    gsl_matrix_free(pInvertedAlphaMatrix);
    gsl_matrix_free(pBetaMatrix);
    gsl_matrix_free(pAlphaMatrix);
}

} // namespace AlignmentSubsystem
} // namespace INDI
