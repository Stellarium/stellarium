/*!
 * \file Common.cpp
 *
 * \author Roger James
 * \date 28th January 2014
 *
 */

#include "Common.h"

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>

namespace INDI
{
namespace AlignmentSubsystem
{
void TelescopeDirectionVector::RotateAroundY(double Angle)
{
    Angle                       = Angle * M_PI / 180.0;
    gsl_vector *pGSLInputVector = gsl_vector_alloc(3);
    gsl_vector_set(pGSLInputVector, 0, x);
    gsl_vector_set(pGSLInputVector, 1, y);
    gsl_vector_set(pGSLInputVector, 2, z);
    gsl_matrix *pRotationMatrix = gsl_matrix_alloc(3, 3);
    gsl_matrix_set(pRotationMatrix, 0, 0, cos(Angle));
    gsl_matrix_set(pRotationMatrix, 0, 1, 0.0);
    gsl_matrix_set(pRotationMatrix, 0, 2, sin(Angle));
    gsl_matrix_set(pRotationMatrix, 1, 0, 0.0);
    gsl_matrix_set(pRotationMatrix, 1, 1, 1.0);
    gsl_matrix_set(pRotationMatrix, 1, 2, 0.0);
    gsl_matrix_set(pRotationMatrix, 2, 0, -sin(Angle));
    gsl_matrix_set(pRotationMatrix, 2, 1, 0.0);
    gsl_matrix_set(pRotationMatrix, 2, 2, cos(Angle));
    gsl_vector *pGSLOutputVector = gsl_vector_alloc(3);
    gsl_vector_set_zero(pGSLOutputVector);
    gsl_blas_dgemv(CblasNoTrans, 1.0, pRotationMatrix, pGSLInputVector, 0.0, pGSLOutputVector);
    x = gsl_vector_get(pGSLOutputVector, 0);
    y = gsl_vector_get(pGSLOutputVector, 1);
    z = gsl_vector_get(pGSLOutputVector, 2);
    gsl_vector_free(pGSLInputVector);
    gsl_vector_free(pGSLOutputVector);
    gsl_matrix_free(pRotationMatrix);
}

} // namespace AlignmentSubsystem
} // namespace INDI
