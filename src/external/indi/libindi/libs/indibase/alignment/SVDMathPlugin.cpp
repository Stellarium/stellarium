/// \file SVDMathPlugin.cpp
/// \author Roger James
/// \date 13th November 2013

#include "SVDMathPlugin.h"

#include "DriverCommon.h"

#include <gsl/gsl_linalg.h>

namespace INDI
{
namespace AlignmentSubsystem
{
// Standard functions required for all plugins
extern "C" {
SVDMathPlugin *Create()
{
    return new SVDMathPlugin;
}

void Destroy(SVDMathPlugin *pPlugin)
{
    delete pPlugin;
}

const char *GetDisplayName()
{
    return "SVD Math Plugin";
}
}
void SVDMathPlugin::CalculateTransformMatrices(const TelescopeDirectionVector &Alpha1,
                                               const TelescopeDirectionVector &Alpha2,
                                               const TelescopeDirectionVector &Alpha3,
                                               const TelescopeDirectionVector &Beta1,
                                               const TelescopeDirectionVector &Beta2,
                                               const TelescopeDirectionVector &Beta3, gsl_matrix *pAlphaToBeta,
                                               gsl_matrix *pBetaToAlpha)
{
    int GslRetcode;

    // Set up the column vectors
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

    // Use Markley's singular value decomposition (SVD) method
    // A detailed description can be found here
    // http://www.control.auc.dk/~tb/best/aug23-Bak-svdalg.pdf

    // 1. Transpose the alpha matrix
    GslRetcode = gsl_matrix_transpose(pAlphaMatrix);

    // 2. Compute the first intermediate matrix
    gsl_matrix *pIntermediateMatrix1 = gsl_matrix_alloc(3, 3);
    MatrixMatrixMultiply(pBetaMatrix, pAlphaMatrix, pIntermediateMatrix1);

    // 3. Compute the singular value decomoposition of the intermediate matrix
    gsl_matrix *pV    = gsl_matrix_alloc(3, 3);
    gsl_vector *pS    = gsl_vector_alloc(3);
    gsl_vector *pWork = gsl_vector_alloc(3);
    GslRetcode        = gsl_linalg_SV_decomp(pIntermediateMatrix1, pV, pS, pWork);
    // The intermediate matrix now contains the U matrix
    // The V matrix is untransposed

    // 4. Compute the diagonal matrix
    gsl_matrix *pDiagonal = gsl_matrix_calloc(3, 3);
    gsl_matrix_set(pDiagonal, 0, 0, 1);
    gsl_matrix_set(pDiagonal, 1, 1, 1);
    gsl_matrix_set(pDiagonal, 2, 2, Matrix3x3Determinant(pIntermediateMatrix1) * Matrix3x3Determinant(pV));

    // 5. Compute the transform
    gsl_matrix_transpose(pV);
    gsl_matrix *pIntermediateMatrix2 = gsl_matrix_alloc(3, 3);
    MatrixMatrixMultiply(pIntermediateMatrix1, pDiagonal, pIntermediateMatrix2);
    MatrixMatrixMultiply(pIntermediateMatrix2, pV, pAlphaToBeta);

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
            IDMessage(nullptr,
                      "Calculated Celestial to Telescope transformation matrix is singular (not a true transform).");
        }

        Dump3x3("BetaToAlpha", pBetaToAlpha);
    }

    // Clean up
    gsl_matrix_free(pIntermediateMatrix1);
    gsl_matrix_free(pV);
    gsl_vector_free(pS);
    gsl_vector_free(pWork);
    gsl_matrix_free(pDiagonal);
    gsl_matrix_free(pIntermediateMatrix2);
    gsl_matrix_free(pBetaMatrix);
    gsl_matrix_free(pAlphaMatrix);
}

} // namespace AlignmentSubsystem
} // namespace INDI
