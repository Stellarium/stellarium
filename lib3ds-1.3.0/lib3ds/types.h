/* -*- c -*- */
#ifndef INCLUDED_LIB3DS_TYPES_H
#define INCLUDED_LIB3DS_TYPES_H
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
 * $Id: types.h,v 1.25 2007/06/21 08:36:41 jeh Exp $
 */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#ifdef LIB3DS_EXPORTS
#define LIB3DSAPI __declspec(dllexport)
#else               
#define LIB3DSAPI __declspec(dllimport)
#endif           
#else
#define LIB3DSAPI
#endif

#define LIB3DS_TRUE 1
#define LIB3DS_FALSE 0

#ifdef _MSC_VER
typedef __int32 Lib3dsBool;
typedef unsigned __int8 Lib3dsByte;
typedef unsigned __int16 Lib3dsWord;
typedef unsigned __int32 Lib3dsDword;
typedef signed __int8 Lib3dsIntb;
typedef signed __int16 Lib3dsIntw;
typedef signed __int16 Lib3dsIntd;
#else
#include <stdint.h>
typedef int32_t Lib3dsBool;
typedef uint8_t Lib3dsByte;
typedef uint16_t Lib3dsWord;
typedef uint32_t Lib3dsDword;
typedef int8_t Lib3dsIntb;
typedef int16_t Lib3dsIntw;
typedef int32_t Lib3dsIntd;
#endif

typedef float Lib3dsFloat;
typedef double Lib3dsDouble;

typedef float Lib3dsVector[3];
typedef float Lib3dsTexel[2];
typedef float Lib3dsQuat[4];
typedef float Lib3dsMatrix[4][4];
typedef float Lib3dsRgb[3];
typedef float Lib3dsRgba[4];

#define LIB3DS_EPSILON (1e-8)
#define LIB3DS_PI 3.14159265358979323846
#define LIB3DS_TWOPI (2.0*LIB3DS_PI)
#define LIB3DS_HALFPI (LIB3DS_PI/2.0)
#define LIB3DS_RAD_TO_DEG(x) ((180.0/LIB3DS_PI)*(x))
#define LIB3DS_DEG_TO_RAD(x) ((LIB3DS_PI/180.0)*(x))
  
#include <stdio.h>

#ifdef _DEBUG
  #ifndef ASSERT
  #include <assert.h>
  #define ASSERT(__expr) assert(__expr)
  #endif
  #define LIB3DS_ERROR_LOG \
    {printf("\t***LIB3DS_ERROR_LOG*** %s : %d\n", __FILE__, __LINE__);}
#else 
  #ifndef ASSERT
  #define ASSERT(__expr)
  #endif
  #define LIB3DS_ERROR_LOG
#endif

typedef struct Lib3dsIo Lib3dsIo;
typedef struct Lib3dsFile Lib3dsFile;
typedef struct Lib3dsBackground Lib3dsBackground;
typedef struct Lib3dsAtmosphere Lib3dsAtmosphere;
typedef struct Lib3dsShadow Lib3dsShadow;
typedef struct Lib3dsViewport Lib3dsViewport;
typedef struct Lib3dsMaterial Lib3dsMaterial;
typedef struct Lib3dsFace Lib3dsFace; 
typedef struct Lib3dsBoxMap Lib3dsBoxMap; 
typedef struct Lib3dsMapData Lib3dsMapData; 
typedef struct Lib3dsMesh Lib3dsMesh;
typedef struct Lib3dsCamera Lib3dsCamera;
typedef struct Lib3dsLight Lib3dsLight;
typedef struct Lib3dsBoolKey Lib3dsBoolKey;
typedef struct Lib3dsBoolTrack Lib3dsBoolTrack;
typedef struct Lib3dsLin1Key Lib3dsLin1Key;
typedef struct Lib3dsLin1Track Lib3dsLin1Track;
typedef struct Lib3dsLin3Key Lib3dsLin3Key;
typedef struct Lib3dsLin3Track Lib3dsLin3Track;
typedef struct Lib3dsQuatKey Lib3dsQuatKey;
typedef struct Lib3dsQuatTrack Lib3dsQuatTrack;
typedef struct Lib3dsMorphKey Lib3dsMorphKey;
typedef struct Lib3dsMorphTrack Lib3dsMorphTrack;
               
typedef enum Lib3dsNodeTypes {
  LIB3DS_UNKNOWN_NODE =0,
  LIB3DS_AMBIENT_NODE =1,
  LIB3DS_OBJECT_NODE  =2,
  LIB3DS_CAMERA_NODE  =3,
  LIB3DS_TARGET_NODE  =4,
  LIB3DS_LIGHT_NODE   =5,
  LIB3DS_SPOT_NODE    =6
} Lib3dsNodeTypes;

typedef struct Lib3dsNode Lib3dsNode;

typedef enum Lib3dsObjectFlags {
  LIB3DS_OBJECT_HIDDEN          =0x01, 
  LIB3DS_OBJECT_VIS_LOFTER      =0x02, 
  LIB3DS_OBJECT_DOESNT_CAST     =0x04, 
  LIB3DS_OBJECT_MATTE           =0x08, 
  LIB3DS_OBJECT_DONT_RCVSHADOW  =0x10, 
  LIB3DS_OBJECT_FAST            =0x20, 
  LIB3DS_OBJECT_FROZEN          =0x40 
} Lib3dsObjectFlags;

typedef union Lib3dsUserData {
    void *p;
    Lib3dsIntd i;
    Lib3dsDword d;
    Lib3dsFloat f;
    Lib3dsMaterial *material;
    Lib3dsMesh *mesh;
    Lib3dsCamera *camera;
    Lib3dsLight *light;
    Lib3dsNode *node;
} Lib3dsUserData;

#ifdef __cplusplus
}
#endif
#endif
