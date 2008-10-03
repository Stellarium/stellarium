/*
 * The 3D Studio File Format Library
 * Copyright (C) 1996-2007 by Jan Eric Kyprianidis <www.kyprianidis.com>
 * All rights reserved.
 *
 * This program is  free  software;  you can redistribute it and/or modify it
 * under the terms of the  GNU Lesser General Public License  as published by 
 * the  Free Software Foundation;  either version 2.1 of the License,  or (at 
 * your option) any later version.
 *
 * This  program  is  distributed in  the  hope that it will  be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or  FITNESS FOR A  PARTICULAR PURPOSE.  See the  GNU Lesser General Public  
 * License for more details.
 *
 * You should  have received  a copy of the GNU Lesser General Public License
 * along with  this program;  if not, write to the  Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: matrix.c,v 1.14 2007/06/20 17:04:08 jeh Exp $
 */
#include <lib3ds/matrix.h>
#include <lib3ds/quat.h>
#include <lib3ds/vector.h>
#include <string.h>
#include <math.h>


/*!
 * \defgroup matrix Matrix Mathematics
 */


/*!
* \typedef Lib3dsMatrix
* \ingroup matrix
*/


/*!
 * Clear a matrix to all zeros.
 *
 * \param m Matrix to be cleared.
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_zero(Lib3dsMatrix m)
{
  int i,j;

  for (i=0; i<4; i++) {
    for (j=0; j<4; j++) m[i][j]=0.0f;
  }
}


/*!
 * Set a matrix to identity.
 *
 * \param m Matrix to be set.
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_identity(Lib3dsMatrix m)
{
  int i,j;

  for (i=0; i<4; i++) {
    for (j=0; j<4; j++) m[i][j]=0.0;
  }
  for (i=0; i<4; i++) m[i][i]=1.0;
}


/*!
 * Copy a matrix.
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_copy(Lib3dsMatrix dest, Lib3dsMatrix src)
{
  memcpy(dest, src, sizeof(Lib3dsMatrix)); 
}


/*!
 * Negate a matrix -- all elements negated.
 *
 * \ingroup matrix
 */
void 
lib3ds_matrix_neg(Lib3dsMatrix m)
{
  int i,j;

  for (j=0; j<4; j++) {
    for (i=0; i<4; i++) {
      m[j][i]=-m[j][i];
    }
  }
}


/*!
 * Set all matrix elements to their absolute value.
 *
 * \ingroup matrix
 */
void 
lib3ds_matrix_abs(Lib3dsMatrix m)
{
  int i,j;

  for (j=0; j<4; j++) {
    for (i=0; i<4; i++) {
      m[j][i]=(Lib3dsFloat)fabs(m[j][i]);
    }
  }
}


/*!
 * Transpose a matrix in place.
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_transpose(Lib3dsMatrix m)
{
  int i,j;
  Lib3dsFloat swp;

  for (j=0; j<4; j++) {
    for (i=j+1; i<4; i++) {
      swp=m[j][i];
      m[j][i]=m[i][j];
      m[i][j]=swp;
    }
  }
}


/*!
 * Add two matrices.
 *
 * \ingroup matrix
 */
void
_lib3ds_matrix_add(Lib3dsMatrix m, Lib3dsMatrix a, Lib3dsMatrix b)
{
  int i,j;

  for (j=0; j<4; j++) {
    for (i=0; i<4; i++) {
      m[j][i]=a[j][i]+b[j][i];
    }
  }
}


/*!
 * Subtract two matrices.
 *
 * \param m Result.
 * \param a Addend.
 * \param b Minuend.
 *
 * \ingroup matrix
 */
void
_lib3ds_matrix_sub(Lib3dsMatrix m, Lib3dsMatrix a, Lib3dsMatrix b)
{
  int i,j;

  for (j=0; j<4; j++) {
    for (i=0; i<4; i++) {
      m[j][i]=a[j][i]-b[j][i];
    }
  }
}


/*!
 * Multiplies a matrix by a second one (m = m * n).
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_mult(Lib3dsMatrix m, Lib3dsMatrix n)
{
  Lib3dsMatrix tmp;
  int i,j,k;
  Lib3dsFloat ab;

  memcpy(tmp, m, sizeof(Lib3dsMatrix)); 
  for (j=0; j<4; j++) {
    for (i=0; i<4; i++) {
      ab=0.0f;
      for (k=0; k<4; k++) ab+=tmp[k][i]*n[j][k];
      m[j][i]=ab;
    }
  }
}


/*!
 * Multiply a matrix by a scalar.
 *
 * \param m Matrix to be set.
 * \param k Scalar.
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_scalar(Lib3dsMatrix m, Lib3dsFloat k)
{
  int i,j;

  for (j=0; j<4; j++) {
    for (i=0; i<4; i++) {
      m[j][i]*=k;
    }
  }
}


static Lib3dsFloat
det2x2(
  Lib3dsFloat a, Lib3dsFloat b,
  Lib3dsFloat c, Lib3dsFloat d) 
{
  return((a)*(d)-(b)*(c));
}


static Lib3dsFloat
det3x3(
  Lib3dsFloat a1, Lib3dsFloat a2, Lib3dsFloat a3,
  Lib3dsFloat b1, Lib3dsFloat b2, Lib3dsFloat b3,
  Lib3dsFloat c1, Lib3dsFloat c2, Lib3dsFloat c3)
{
  return(
    a1*det2x2(b2,b3,c2,c3)-
    b1*det2x2(a2,a3,c2,c3)+
    c1*det2x2(a2,a3,b2,b3)
  );
}


/*!
 * Find determinant of a matrix.
 *
 * \ingroup matrix
 */
Lib3dsFloat
lib3ds_matrix_det(Lib3dsMatrix m)
{
  Lib3dsFloat a1,a2,a3,a4,b1,b2,b3,b4,c1,c2,c3,c4,d1,d2,d3,d4;

  a1 = m[0][0];
  b1 = m[1][0];
  c1 = m[2][0];
  d1 = m[3][0];
  a2 = m[0][1];
  b2 = m[1][1];
  c2 = m[2][1];
  d2 = m[3][1];
  a3 = m[0][2];
  b3 = m[1][2];
  c3 = m[2][2];
  d3 = m[3][2];
  a4 = m[0][3];
  b4 = m[1][3];
  c4 = m[2][3];
  d4 = m[3][3];
  return(
    a1 * det3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4)-
    b1 * det3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4)+
    c1 * det3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4)-
    d1 * det3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4)
  );
}


/*!
 * Find the adjoint of a matrix.
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_adjoint(Lib3dsMatrix m)
{
  Lib3dsFloat a1,a2,a3,a4,b1,b2,b3,b4,c1,c2,c3,c4,d1,d2,d3,d4;

  a1 = m[0][0];
  b1 = m[1][0];
  c1 = m[2][0];
  d1 = m[3][0];
  a2 = m[0][1];
  b2 = m[1][1];
  c2 = m[2][1];
  d2 = m[3][1];
  a3 = m[0][2];
  b3 = m[1][2];
  c3 = m[2][2];
  d3 = m[3][2];
  a4 = m[0][3];
  b4 = m[1][3];
  c4 = m[2][3];
  d4 = m[3][3];
  m[0][0]=  det3x3 (b2, b3, b4, c2, c3, c4, d2, d3, d4);
  m[0][1]= -det3x3 (a2, a3, a4, c2, c3, c4, d2, d3, d4);
  m[0][2]=  det3x3 (a2, a3, a4, b2, b3, b4, d2, d3, d4);
  m[0][3]= -det3x3 (a2, a3, a4, b2, b3, b4, c2, c3, c4);
  m[1][0]= -det3x3 (b1, b3, b4, c1, c3, c4, d1, d3, d4);
  m[1][1]=  det3x3 (a1, a3, a4, c1, c3, c4, d1, d3, d4);
  m[1][2]= -det3x3 (a1, a3, a4, b1, b3, b4, d1, d3, d4);
  m[1][3]=  det3x3 (a1, a3, a4, b1, b3, b4, c1, c3, c4);
  m[2][0]=  det3x3 (b1, b2, b4, c1, c2, c4, d1, d2, d4);
  m[2][1]= -det3x3 (a1, a2, a4, c1, c2, c4, d1, d2, d4);
  m[2][2]=  det3x3 (a1, a2, a4, b1, b2, b4, d1, d2, d4);
  m[2][3]= -det3x3 (a1, a2, a4, b1, b2, b4, c1, c2, c4);
  m[3][0]= -det3x3 (b1, b2, b3, c1, c2, c3, d1, d2, d3);
  m[3][1]=  det3x3 (a1, a2, a3, c1, c2, c3, d1, d2, d3);
  m[3][2]= -det3x3 (a1, a2, a3, b1, b2, b3, d1, d2, d3);
  m[3][3]=  det3x3 (a1, a2, a3, b1, b2, b3, c1, c2, c3);
}


/*!
 * Invert a matrix in place.
 *
 * \param m Matrix to invert.
 *
 * \return LIB3DS_TRUE on success, LIB3DS_FALSE on failure.
 * \ingroup matrix
 *
 * GGemsII, K.Wu, Fast Matrix Inversion 
 */
Lib3dsBool
lib3ds_matrix_inv(Lib3dsMatrix m)
{                          
  int i,j,k;               
  int pvt_i[4], pvt_j[4];            /* Locations of pivot elements */
  Lib3dsFloat pvt_val;               /* Value of current pivot element */
  Lib3dsFloat hold;                  /* Temporary storage */
  Lib3dsFloat determinat;            

  determinat = 1.0f;
  for (k=0; k<4; k++)  {
    /* Locate k'th pivot element */
    pvt_val=m[k][k];            /* Initialize for search */
    pvt_i[k]=k;
    pvt_j[k]=k;
    for (i=k; i<4; i++) {
      for (j=k; j<4; j++) {
        if (fabs(m[i][j]) > fabs(pvt_val)) {
          pvt_i[k]=i;
          pvt_j[k]=j;
          pvt_val=m[i][j];
        }
      }
    }

    /* Product of pivots, gives determinant when finished */
    determinat*=pvt_val;
    if (fabs(determinat)<LIB3DS_EPSILON) {    
      return(LIB3DS_FALSE);  /* Matrix is singular (zero determinant) */
    }

    /* "Interchange" rows (with sign change stuff) */
    i=pvt_i[k];
    if (i!=k) {               /* If rows are different */
      for (j=0; j<4; j++) {
        hold=-m[k][j];
        m[k][j]=m[i][j];
        m[i][j]=hold;
      }
    }

    /* "Interchange" columns */
    j=pvt_j[k];
    if (j!=k) {              /* If columns are different */
      for (i=0; i<4; i++) {
        hold=-m[i][k];
        m[i][k]=m[i][j];
        m[i][j]=hold;
      }
    }
    
    /* Divide column by minus pivot value */
    for (i=0; i<4; i++) {
      if (i!=k) m[i][k]/=( -pvt_val) ; 
    }

    /* Reduce the matrix */
    for (i=0; i<4; i++) {
      hold = m[i][k];
      for (j=0; j<4; j++) {
        if (i!=k && j!=k) m[i][j]+=hold*m[k][j];
      }
    }

    /* Divide row by pivot */
    for (j=0; j<4; j++) {
      if (j!=k) m[k][j]/=pvt_val;
    }

    /* Replace pivot by reciprocal (at last we can touch it). */
    m[k][k] = 1.0f/pvt_val;
  }

  /* That was most of the work, one final pass of row/column interchange */
  /* to finish */
  for (k=4-2; k>=0; k--) { /* Don't need to work with 1 by 1 corner*/
    i=pvt_j[k];            /* Rows to swap correspond to pivot COLUMN */
    if (i!=k) {            /* If rows are different */
      for(j=0; j<4; j++) {
        hold = m[k][j];
        m[k][j]=-m[i][j];
        m[i][j]=hold;
      }
    }

    j=pvt_i[k];           /* Columns to swap correspond to pivot ROW */
    if (j!=k)             /* If columns are different */
    for (i=0; i<4; i++) {
      hold=m[i][k];
      m[i][k]=-m[i][j];
      m[i][j]=hold;
    }
  }
  return(LIB3DS_TRUE);                          
}


/*!
 * Apply a translation to a matrix.
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_translate_xyz(Lib3dsMatrix m, Lib3dsFloat x, Lib3dsFloat y, Lib3dsFloat z)
{
  int i;
  
  for (i=0; i<3; i++) {
    m[3][i]+= m[0][i]*x + m[1][i]*y + m[2][i]*z;
  }
}


/*!
 * Apply a translation to a matrix.
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_translate(Lib3dsMatrix m, Lib3dsVector t)
{
  int i;
  
  for (i=0; i<3; i++) {
    m[3][i]+= m[0][i]*t[0] + m[1][i]*t[1] + m[2][i]*t[2];
  }
}


/*!
 * Apply scale factors to a matrix.
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_scale_xyz(Lib3dsMatrix m, Lib3dsFloat x, Lib3dsFloat y, Lib3dsFloat z)
{
  int i;

  for (i=0; i<4; i++) {
    m[0][i]*=x;
    m[1][i]*=y;
    m[2][i]*=z;
  }
}


/*!
 * Apply scale factors to a matrix.
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_scale(Lib3dsMatrix m, Lib3dsVector s)
{
  int i;

  for (i=0; i<4; i++) {
    m[0][i]*=s[0];
    m[1][i]*=s[1];
    m[2][i]*=s[2];
  }
}


/*!
 * Apply a rotation about the x axis to a matrix.
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_rotate_x(Lib3dsMatrix m, Lib3dsFloat phi)
{
  Lib3dsFloat SinPhi,CosPhi;
  Lib3dsFloat a1[4],a2[4];

  SinPhi=(Lib3dsFloat)sin(phi);
  CosPhi=(Lib3dsFloat)cos(phi);
  memcpy(a1,m[1],4*sizeof(Lib3dsFloat));
  memcpy(a2,m[2],4*sizeof(Lib3dsFloat));
  m[1][0]=CosPhi*a1[0]+SinPhi*a2[0];
  m[1][1]=CosPhi*a1[1]+SinPhi*a2[1];
  m[1][2]=CosPhi*a1[2]+SinPhi*a2[2];
  m[1][3]=CosPhi*a1[3]+SinPhi*a2[3];
  m[2][0]=-SinPhi*a1[0]+CosPhi*a2[0];
  m[2][1]=-SinPhi*a1[1]+CosPhi*a2[1];
  m[2][2]=-SinPhi*a1[2]+CosPhi*a2[2];
  m[2][3]=-SinPhi*a1[3]+CosPhi*a2[3];
}


/*!
 * Apply a rotation about the y axis to a matrix.
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_rotate_y(Lib3dsMatrix m, Lib3dsFloat phi)
{
  Lib3dsFloat SinPhi,CosPhi;
  Lib3dsFloat a0[4],a2[4];

  SinPhi=(Lib3dsFloat)sin(phi);
  CosPhi=(Lib3dsFloat)cos(phi);
  memcpy(a0,m[0],4*sizeof(Lib3dsFloat));
  memcpy(a2,m[2],4*sizeof(Lib3dsFloat));
  m[0][0]=CosPhi*a0[0]-SinPhi*a2[0];
  m[0][1]=CosPhi*a0[1]-SinPhi*a2[1];
  m[0][2]=CosPhi*a0[2]-SinPhi*a2[2];
  m[0][3]=CosPhi*a0[3]-SinPhi*a2[3];
  m[2][0]=SinPhi*a0[0]+CosPhi*a2[0];
  m[2][1]=SinPhi*a0[1]+CosPhi*a2[1];
  m[2][2]=SinPhi*a0[2]+CosPhi*a2[2];
  m[2][3]=SinPhi*a0[3]+CosPhi*a2[3];
}


/*!
 * Apply a rotation about the z axis to a matrix.
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_rotate_z(Lib3dsMatrix m, Lib3dsFloat phi)
{
  Lib3dsFloat SinPhi,CosPhi;
  Lib3dsFloat a0[4],a1[4];
  
  SinPhi=(Lib3dsFloat)sin(phi);
  CosPhi=(Lib3dsFloat)cos(phi);
  memcpy(a0,m[0],4*sizeof(Lib3dsFloat));
  memcpy(a1,m[1],4*sizeof(Lib3dsFloat));
  m[0][0]=CosPhi*a0[0]+SinPhi*a1[0];
  m[0][1]=CosPhi*a0[1]+SinPhi*a1[1];
  m[0][2]=CosPhi*a0[2]+SinPhi*a1[2];
  m[0][3]=CosPhi*a0[3]+SinPhi*a1[3];
  m[1][0]=-SinPhi*a0[0]+CosPhi*a1[0];
  m[1][1]=-SinPhi*a0[1]+CosPhi*a1[1];
  m[1][2]=-SinPhi*a0[2]+CosPhi*a1[2];
  m[1][3]=-SinPhi*a0[3]+CosPhi*a1[3];
}


/*!
 * Apply a rotation about an arbitrary axis to a matrix.
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_rotate(Lib3dsMatrix m, Lib3dsQuat q)
{
  Lib3dsFloat s,xs,ys,zs,wx,wy,wz,xx,xy,xz,yy,yz,zz,l;
  Lib3dsMatrix R;

  l=q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3];
  if (fabs(l)<LIB3DS_EPSILON) {
    s=1.0f;
  }
  else {
    s=2.0f/l;
  }

  xs = q[0] * s;   ys = q[1] * s;  zs = q[2] * s;
  wx = q[3] * xs;  wy = q[3] * ys; wz = q[3] * zs;
  xx = q[0] * xs;  xy = q[0] * ys; xz = q[0] * zs;
  yy = q[1] * ys;  yz = q[1] * zs; zz = q[2] * zs;

  R[0][0]=1.0f - (yy +zz);
  R[1][0]=xy - wz;
  R[2][0]=xz + wy;
  R[0][1]=xy + wz;
  R[1][1]=1.0f - (xx +zz);
  R[2][1]=yz - wx;
  R[0][2]=xz - wy;
  R[1][2]=yz + wx;
  R[2][2]=1.0f - (xx + yy);
  R[3][0]=R[3][1]=R[3][2]=R[0][3]=R[1][3]=R[2][3]=0.0f;
  R[3][3]=1.0f;

  lib3ds_matrix_mult(m,R);
}


/*!
 * Apply a rotation about an arbitrary axis to a matrix.
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_rotate_axis(Lib3dsMatrix m, Lib3dsVector axis, Lib3dsFloat angle)
{
  Lib3dsQuat q;
  
  lib3ds_quat_axis_angle(q,axis,angle);
  lib3ds_matrix_rotate(m,q);
}


/*!
 * Compute a camera matrix based on position, target and roll.
 *
 * Generates a translate/rotate matrix that maps world coordinates
 * to camera coordinates.  Resulting matrix does not include perspective
 * transform.
 *
 * \param matrix Destination matrix.
 * \param pos Camera position
 * \param tgt Camera target
 * \param roll Roll angle
 *
 * \ingroup matrix
 */
void
lib3ds_matrix_camera(Lib3dsMatrix matrix, Lib3dsVector pos,
  Lib3dsVector tgt, Lib3dsFloat roll)
{
  Lib3dsMatrix M;
  Lib3dsVector x, y, z;

  lib3ds_vector_sub(y, tgt, pos);
  lib3ds_vector_normalize(y);

  if (y[0] != 0. || y[1] != 0) {
    z[0] = 0;
    z[1] = 0;
    z[2] = 1.0;
  }
  else {	/* Special case:  looking straight up or down z axis */
    z[0] = -1.0;
    z[1] = 0;
    z[2] = 0;
  }

  lib3ds_vector_cross(x, y, z);
  lib3ds_vector_cross(z, x, y);
  lib3ds_vector_normalize(x);
  lib3ds_vector_normalize(z);

  lib3ds_matrix_identity(M);
  M[0][0] = x[0];
  M[1][0] = x[1];
  M[2][0] = x[2];
  M[0][1] = y[0];
  M[1][1] = y[1];
  M[2][1] = y[2];
  M[0][2] = z[0];
  M[1][2] = z[1];
  M[2][2] = z[2];

  lib3ds_matrix_identity(matrix);
  lib3ds_matrix_rotate_y(matrix, roll);
  lib3ds_matrix_mult(matrix, M);
  lib3ds_matrix_translate_xyz(matrix, -pos[0],-pos[1],-pos[2]);
}


/*!
 * \ingroup matrix
 */
void
lib3ds_matrix_dump(Lib3dsMatrix matrix)
{
  int i,j;

  for (i=0; i<4; ++i) {
    for (j=0; j<4; ++j) {
      printf("%f ", matrix[j][i]);
    }
    printf("\n");
  }
}





