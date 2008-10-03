/* -*- c -*- */
#ifndef INCLUDED_LIB3DS_TRACKS_H
#define INCLUDED_LIB3DS_TRACKS_H
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
 * $Id: tracks.h,v 1.11 2007/06/20 17:04:09 jeh Exp $
 */

#ifndef INCLUDED_LIB3DS_TCB_H
#include <lib3ds/tcb.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Track flags
 * \ingroup tracks
 */
typedef enum {
  LIB3DS_REPEAT    =0x0001,
  LIB3DS_SMOOTH    =0x0002,
  LIB3DS_LOCK_X    =0x0008,
  LIB3DS_LOCK_Y    =0x0010,
  LIB3DS_LOCK_Z    =0x0020,
  LIB3DS_UNLINK_X  =0x0100,
  LIB3DS_UNLINK_Y  =0x0200,
  LIB3DS_UNLINK_Z  =0x0400
} Lib3dsTrackFlags;

/**
 * Boolean track key
 * \ingroup tracks
 */
struct Lib3dsBoolKey {
    Lib3dsTcb tcb;
    Lib3dsBoolKey *next;
};

/**
 * Boolean track
 * \ingroup tracks
 */
struct Lib3dsBoolTrack {
    Lib3dsDword flags;
    Lib3dsBoolKey *keyL;
};

/**
 * Floating-point track key
 * \ingroup tracks
 */
struct Lib3dsLin1Key {
    Lib3dsTcb tcb;
    Lib3dsLin1Key *next;
    Lib3dsFloat value;
    Lib3dsFloat dd;
    Lib3dsFloat ds;
};
  
/**
 * Floating-point track
 * \ingroup tracks
 */
struct Lib3dsLin1Track {
    Lib3dsDword flags;
    Lib3dsLin1Key *keyL;
};

/**
 * Vector track key
 * \ingroup tracks
 */
struct Lib3dsLin3Key {
    Lib3dsTcb tcb;
    Lib3dsLin3Key *next;  
    Lib3dsVector value;
    Lib3dsVector dd;
    Lib3dsVector ds;
};
  
/**
 * Vector track
 * \ingroup tracks
 */
struct Lib3dsLin3Track {
    Lib3dsDword flags;
    Lib3dsLin3Key *keyL;
};

/**
 * Rotation track key
 * \ingroup tracks
 */
struct Lib3dsQuatKey {
    Lib3dsTcb tcb;
    Lib3dsQuatKey *next;  
    Lib3dsVector axis;
    Lib3dsFloat angle;
    Lib3dsQuat q;
    Lib3dsQuat dd;
    Lib3dsQuat ds;
};
  
/**
 * Rotation track 
 * \ingroup tracks
 */
struct Lib3dsQuatTrack {
    Lib3dsDword flags;
    Lib3dsQuatKey *keyL;
};

/**
 * Morph track key
 * \ingroup tracks
 */
struct Lib3dsMorphKey {
    Lib3dsTcb tcb;
    Lib3dsMorphKey *next;  
    char name[64];
};
  
/**
 * Morph track
 * \ingroup tracks
 */
struct Lib3dsMorphTrack {
    Lib3dsDword flags;
    Lib3dsMorphKey *keyL;
};

extern LIB3DSAPI Lib3dsBoolKey* lib3ds_bool_key_new();
extern LIB3DSAPI void lib3ds_bool_key_free(Lib3dsBoolKey* key);
extern LIB3DSAPI void lib3ds_bool_track_free_keys(Lib3dsBoolTrack *track);
extern LIB3DSAPI void lib3ds_bool_track_insert(Lib3dsBoolTrack *track, Lib3dsBoolKey* key);
extern LIB3DSAPI void lib3ds_bool_track_remove(Lib3dsBoolTrack *track, Lib3dsIntd frame);
extern LIB3DSAPI void lib3ds_bool_track_eval(Lib3dsBoolTrack *track, Lib3dsBool *p, Lib3dsFloat t);
extern LIB3DSAPI Lib3dsBool lib3ds_bool_track_read(Lib3dsBoolTrack *track, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_bool_track_write(Lib3dsBoolTrack *track, Lib3dsIo *io);

extern LIB3DSAPI Lib3dsLin1Key* lib3ds_lin1_key_new();
extern LIB3DSAPI void lib3ds_lin1_key_free(Lib3dsLin1Key* key);
extern LIB3DSAPI void lib3ds_lin1_track_free_keys(Lib3dsLin1Track *track);
extern LIB3DSAPI void lib3ds_lin1_key_setup(Lib3dsLin1Key *p, Lib3dsLin1Key *cp, Lib3dsLin1Key *c,
  Lib3dsLin1Key *cn, Lib3dsLin1Key *n);
extern LIB3DSAPI void lib3ds_lin1_track_setup(Lib3dsLin1Track *track);
extern LIB3DSAPI void lib3ds_lin1_track_insert(Lib3dsLin1Track *track, Lib3dsLin1Key *key);
extern LIB3DSAPI void lib3ds_lin1_track_remove(Lib3dsLin1Track *track, Lib3dsIntd frame);
extern LIB3DSAPI void lib3ds_lin1_track_eval(Lib3dsLin1Track *track, Lib3dsFloat *p, Lib3dsFloat t);
extern LIB3DSAPI Lib3dsBool lib3ds_lin1_track_read(Lib3dsLin1Track *track, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_lin1_track_write(Lib3dsLin1Track *track, Lib3dsIo *io);

extern LIB3DSAPI Lib3dsLin3Key* lib3ds_lin3_key_new();
extern LIB3DSAPI void lib3ds_lin3_key_free(Lib3dsLin3Key* key);
extern LIB3DSAPI void lib3ds_lin3_track_free_keys(Lib3dsLin3Track *track);
extern LIB3DSAPI void lib3ds_lin3_key_setup(Lib3dsLin3Key *p, Lib3dsLin3Key *cp, Lib3dsLin3Key *c,
  Lib3dsLin3Key *cn, Lib3dsLin3Key *n);
extern LIB3DSAPI void lib3ds_lin3_track_setup(Lib3dsLin3Track *track);
extern LIB3DSAPI void lib3ds_lin3_track_insert(Lib3dsLin3Track *track, Lib3dsLin3Key *key);
extern LIB3DSAPI void lib3ds_lin3_track_remove(Lib3dsLin3Track *track, Lib3dsIntd frame);
extern LIB3DSAPI void lib3ds_lin3_track_eval(Lib3dsLin3Track *track, Lib3dsVector p, Lib3dsFloat t);
extern LIB3DSAPI Lib3dsBool lib3ds_lin3_track_read(Lib3dsLin3Track *track, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_lin3_track_write(Lib3dsLin3Track *track, Lib3dsIo *io);

extern LIB3DSAPI Lib3dsQuatKey* lib3ds_quat_key_new();
extern LIB3DSAPI void lib3ds_quat_key_free(Lib3dsQuatKey* key);
extern LIB3DSAPI void lib3ds_quat_track_free_keys(Lib3dsQuatTrack *track);
extern LIB3DSAPI void lib3ds_quat_key_setup(Lib3dsQuatKey *p, Lib3dsQuatKey *cp, Lib3dsQuatKey *c,
  Lib3dsQuatKey *cn, Lib3dsQuatKey *n);
extern LIB3DSAPI void lib3ds_quat_track_setup(Lib3dsQuatTrack *track);
extern LIB3DSAPI void lib3ds_quat_track_insert(Lib3dsQuatTrack *track, Lib3dsQuatKey *key);
extern LIB3DSAPI void lib3ds_quat_track_remove(Lib3dsQuatTrack *track, Lib3dsIntd frame);
extern LIB3DSAPI void lib3ds_quat_track_eval(Lib3dsQuatTrack *track, Lib3dsQuat p, Lib3dsFloat t);
extern LIB3DSAPI Lib3dsBool lib3ds_quat_track_read(Lib3dsQuatTrack *track, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_quat_track_write(Lib3dsQuatTrack *track, Lib3dsIo *io);

extern LIB3DSAPI Lib3dsMorphKey* lib3ds_morph_key_new();
extern LIB3DSAPI void lib3ds_morph_key_free(Lib3dsMorphKey* key);
extern LIB3DSAPI void lib3ds_morph_track_free_keys(Lib3dsMorphTrack *track);
extern LIB3DSAPI void lib3ds_morph_track_insert(Lib3dsMorphTrack *track, Lib3dsMorphKey *key);
extern LIB3DSAPI void lib3ds_morph_track_remove(Lib3dsMorphTrack *track, Lib3dsIntd frame);
extern LIB3DSAPI void lib3ds_morph_track_eval(Lib3dsMorphTrack *track, char *p, Lib3dsFloat t);
extern LIB3DSAPI Lib3dsBool lib3ds_morph_track_read(Lib3dsMorphTrack *track, Lib3dsIo *io);
extern LIB3DSAPI Lib3dsBool lib3ds_morph_track_write(Lib3dsMorphTrack *track, Lib3dsIo *io);

#ifdef __cplusplus
}
#endif
#endif

