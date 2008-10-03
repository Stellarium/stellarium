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
 * $Id: quat.c,v 1.9 2007/06/20 17:04:09 jeh Exp $
 */
#include <lib3ds/quat.h>
#include <math.h>


/*!
 * \defgroup quat Quaternion Mathematics
 */


/*!
* \typedef Lib3dsQuat
* \ingroup quat
*/


/*!
 * Clear a quaternion.
 * \ingroup quat
 */
void
lib3ds_quat_zero(Lib3dsQuat c)
{
  c[0]=c[1]=c[2]=c[3]=0.0f;
}


/*!
 * Set a quaternion to Identity
 * \ingroup quat
 */
void
lib3ds_quat_identity(Lib3dsQuat c)
{
  c[0]=c[1]=c[2]=0.0f;
  c[3]=1.0f;
}


/*!
 * Copy a quaternion.
 * \ingroup quat
 */
void 
lib3ds_quat_copy(Lib3dsQuat dest, Lib3dsQuat src)
{
  int i;
  for (i=0; i<4; ++i) {
    dest[i]=src[i];
  }
}


/*!
 * Compute a quaternion from axis and angle.
 *
 * \param c Computed quaternion
 * \param axis Rotation axis
 * \param angle Angle of rotation, radians.
 *
 * \ingroup quat
 */
void
lib3ds_quat_axis_angle(Lib3dsQuat c, Lib3dsVector axis, Lib3dsFloat angle)
{
  Lib3dsDouble omega,s;
  Lib3dsDouble l;

  l=sqrt(axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2]);
  if (l<LIB3DS_EPSILON) {
    c[0]=c[1]=c[2]=0.0f;
    c[3]=1.0f;
  }
  else {
    omega=-0.5*angle;
    s=sin(omega)/l;
    c[0]=(Lib3dsFloat)s*axis[0];
    c[1]=(Lib3dsFloat)s*axis[1];
    c[2]=(Lib3dsFloat)s*axis[2];
    c[3]=(Lib3dsFloat)cos(omega);
  }
}


/*!
 * Negate a quaternion
 *
 * \ingroup quat
 */
void
lib3ds_quat_neg(Lib3dsQuat c)
{
  int i;
  for (i=0; i<4; ++i) {
    c[i]=-c[i];
  }
}


/*!
 * Compute the absolute value of a quaternion
 *
 * \ingroup quat
 */
void
lib3ds_quat_abs(Lib3dsQuat c)
{
  int i;
  for (i=0; i<4; ++i) {
    c[i]=(Lib3dsFloat)fabs(c[i]);
  }
}


/*!
 * Compute the conjugate of a quaternion
 *
 * \ingroup quat
 */
void
lib3ds_quat_cnj(Lib3dsQuat c)
{
  int i;
  for (i=0; i<3; ++i) {
    c[i]=-c[i];
  }
}


/*!
 * Multiply two quaternions.
 *
 * \param c Result
 * \param a,b Inputs
 * \ingroup quat
 */
void
lib3ds_quat_mul(Lib3dsQuat c, Lib3dsQuat a, Lib3dsQuat b)
{
  c[0]=a[3]*b[0] + a[0]*b[3] + a[1]*b[2] - a[2]*b[1];
  c[1]=a[3]*b[1] + a[1]*b[3] + a[2]*b[0] - a[0]*b[2];
  c[2]=a[3]*b[2] + a[2]*b[3] + a[0]*b[1] - a[1]*b[0];
  c[3]=a[3]*b[3] - a[0]*b[0] - a[1]*b[1] - a[2]*b[2];
}


/*!
 * Multiply a quaternion by a scalar.
 *
 * \ingroup quat
 */
void
lib3ds_quat_scalar(Lib3dsQuat c, Lib3dsFloat k)
{
  int i;
  for (i=0; i<4; ++i) {
    c[i]*=k;
  }
}


/*!
 * Normalize a quaternion.
 *
 * \ingroup quat
 */
void
lib3ds_quat_normalize(Lib3dsQuat c)
{
  Lib3dsDouble l,m;

  l=sqrt(c[0]*c[0] + c[1]*c[1] + c[2]*c[2] + c[3]*c[3]);
  if (fabs(l)<LIB3DS_EPSILON) {
    c[0]=c[1]=c[2]=0.0f;
    c[3]=1.0f; 
  }
  else {  
    int i;
    m=1.0f/l;
    for (i=0; i<4; ++i) {
      c[i]=(Lib3dsFloat)(c[i]*m);
    }
  }
}


/*!
 * Compute the inverse of a quaternion.
 *
 * \ingroup quat
 */
void
lib3ds_quat_inv(Lib3dsQuat c)
{
  Lib3dsDouble l,m;

  l=sqrt(c[0]*c[0] + c[1]*c[1] + c[2]*c[2] + c[3]*c[3]);
  if (fabs(l)<LIB3DS_EPSILON) {
    c[0]=c[1]=c[2]=0.0f;
    c[3]=1.0f; 
  }
  else {  
    m=1.0f/l;
    c[0]=(Lib3dsFloat)(-c[0]*m);
    c[1]=(Lib3dsFloat)(-c[1]*m);
    c[2]=(Lib3dsFloat)(-c[2]*m);
    c[3]=(Lib3dsFloat)(c[3]*m);
  }
}


/*!
 * Compute the dot-product of a quaternion.
 *
 * \ingroup quat
 */
Lib3dsFloat
lib3ds_quat_dot(Lib3dsQuat a, Lib3dsQuat b)
{
  return(a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3]);
}


/*!
 * \ingroup quat
 */
Lib3dsFloat
lib3ds_quat_squared(Lib3dsQuat c)
{
  return(c[0]*c[0] + c[1]*c[1] + c[2]*c[2] + c[3]*c[3]);
}


/*!
 * \ingroup quat
 */
Lib3dsFloat
lib3ds_quat_length(Lib3dsQuat c)
{
  return((Lib3dsFloat)sqrt(c[0]*c[0] + c[1]*c[1] + c[2]*c[2] + c[3]*c[3]));
}


/*!
 * \ingroup quat
 */
void
lib3ds_quat_ln(Lib3dsQuat c)
{
  Lib3dsDouble om,s,t;

  s=sqrt(c[0]*c[0] + c[1]*c[1] + c[2]*c[2]);
  om=atan2(s,c[3]);
  if (fabs(s)<LIB3DS_EPSILON) {
    t=0.0f;
  }
  else {
    t=om/s;
  }
  {
    int i;
    for (i=0; i<3; ++i) {
      c[i]=(Lib3dsFloat)(c[i]*t);
    }
    c[3]=0.0f;
  }
}


/*!
 * \ingroup quat
 */
void
lib3ds_quat_ln_dif(Lib3dsQuat c, Lib3dsQuat a, Lib3dsQuat b)
{
  Lib3dsQuat invp;

  lib3ds_quat_copy(invp, a);
  lib3ds_quat_inv(invp);
  lib3ds_quat_mul(c, invp, b);
  lib3ds_quat_ln(c);
}


/*!
 * \ingroup quat
 */
void
lib3ds_quat_exp(Lib3dsQuat c)
{
  Lib3dsDouble om,sinom;

  om=sqrt(c[0]*c[0] + c[1]*c[1] + c[2]*c[2]);
  if (fabs(om)<LIB3DS_EPSILON) {
    sinom=1.0f;
  }
  else {
    sinom=sin(om)/om;
  }
  {
    int i;
    for (i=0; i<3; ++i) {
      c[i]=(Lib3dsFloat)(c[i]*sinom);
    }
    c[3]=(Lib3dsFloat)cos(om);
  }
}


/*!
 * \ingroup quat
 */
void
lib3ds_quat_slerp(Lib3dsQuat c, Lib3dsQuat a, Lib3dsQuat b, Lib3dsFloat t)
{
  Lib3dsDouble l;
  Lib3dsDouble om,sinom;
  Lib3dsDouble sp,sq;
  Lib3dsQuat q;

  l=a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
  if ((1.0+l)>LIB3DS_EPSILON) {
    if (fabs(l)>1.0f) l/=fabs(l);
    om=acos(l);
    sinom=sin(om);
    if (fabs(sinom)>LIB3DS_EPSILON) {
      sp=sin((1.0f-t)*om)/sinom;
      sq=sin(t*om)/sinom;
    }
    else {
      sp=1.0f-t;
      sq=t;
    }
    c[0]=(Lib3dsFloat)(sp*a[0] + sq*b[0]);
    c[1]=(Lib3dsFloat)(sp*a[1] + sq*b[1]);
    c[2]=(Lib3dsFloat)(sp*a[2] + sq*b[2]);
    c[3]=(Lib3dsFloat)(sp*a[3] + sq*b[3]);
  }
  else {
    q[0]=-a[1];
    q[1]=a[0];
    q[2]=-a[3];
    q[3]=a[2];
    sp=sin((1.0-t)*LIB3DS_HALFPI);
    sq=sin(t*LIB3DS_HALFPI);
    c[0]=(Lib3dsFloat)(sp*a[0] + sq*q[0]);
    c[1]=(Lib3dsFloat)(sp*a[1] + sq*q[1]);
    c[2]=(Lib3dsFloat)(sp*a[2] + sq*q[2]);
    c[3]=(Lib3dsFloat)(sp*a[3] + sq*q[3]);
  }
}


/*!
 * \ingroup quat
 */
void
lib3ds_quat_squad(Lib3dsQuat c, Lib3dsQuat a, Lib3dsQuat p, Lib3dsQuat q,
  Lib3dsQuat b, Lib3dsFloat t)
{
  Lib3dsQuat ab;
  Lib3dsQuat pq;

  lib3ds_quat_slerp(ab,a,b,t);
  lib3ds_quat_slerp(pq,p,q,t);
  lib3ds_quat_slerp(c,ab,pq,2*t*(1-t));
}


/*!
 * \ingroup quat
 */
void
lib3ds_quat_tangent(Lib3dsQuat c, Lib3dsQuat p, Lib3dsQuat q, Lib3dsQuat n)
{
  Lib3dsQuat dn,dp,x;
  int i;

  lib3ds_quat_ln_dif(dn, q, n);
  lib3ds_quat_ln_dif(dp, q, p);

  for (i=0; i<4; i++) {
    x[i]=-1.0f/4.0f*(dn[i]+dp[i]);
  }
  lib3ds_quat_exp(x);
  lib3ds_quat_mul(c,q,x);
}


/*!
 * \ingroup quat
 */
void
lib3ds_quat_dump(Lib3dsQuat q)
{
  printf("%f %f %f %f\n", q[0], q[1], q[2], q[3]);
}

