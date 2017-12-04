/*

  Copyright (c) 2000 Finger Lakes Instrumentation (FLI), LLC.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

        Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

        Redistributions in binary form must reproduce the above
        copyright notice, this list of conditions and the following
        disclaimer in the documentation and/or other materials
        provided with the distribution.

        Neither the name of Finger Lakes Instrumentation (FLI), LLC
        nor the names of its contributors may be used to endorse or
        promote products derived from this software without specific
        prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

  ======================================================================

  Finger Lakes Instrumentation (FLI)
  web: http://www.fli-cam.com
  email: fli@rpa.net

*/

#ifndef _FLI_IOCTL_H
#define _FLI_IOCTL_H

#include <asm/ioctl.h>

/* 8-bit special value to identify ioctl 'type' */
#define FLI_IOCTL_TYPE 'F'

/* Macros declaring ioctl commands and the variables they operate on */
#define FLI_IOCTL_MISC_CMDS                             \
  FLI_IOCTL_CMD(FLI_RESET_PORT_VALUES, **NONE**)        \
  FLI_IOCTL_CMD(FLI_LOCK_PORT, **NONE**)                \
  FLI_IOCTL_CMD(FLI_UNLOCK_PORT, **NONE**)

#define FLI_IOCTL_SPECIAL_SET_CMDS                      \
  FLI_IOCTL_CMD(FLI_SET_DMABUFFSIZE, dmabuffsize)

#define FLI_IOCTL_SET_CMDS                      \
  FLI_IOCTL_CMD(FLI_SET_DMATHRESH, dmathresh)   \
  FLI_IOCTL_CMD(FLI_SET_DTO, dto)               \
  FLI_IOCTL_CMD(FLI_SET_RTO, rto)               \
  FLI_IOCTL_CMD(FLI_SET_WTO, wto)               \
  FLI_IOCTL_CMD(FLI_SET_LTL, ltl)               \
  FLI_IOCTL_CMD(FLI_SET_DIR, dir)               \
  FLI_IOCTL_CMD(FLI_SET_NUMREAD, numread)       \
  FLI_IOCTL_CMD(FLI_SET_NUMWRITE, numwrite)     \
  FLI_IOCTL_CMD(FLI_SET_NUMDTO, numdto)         \
  FLI_IOCTL_CMD(FLI_SET_NUMRTO, numrto)         \
  FLI_IOCTL_CMD(FLI_SET_NUMWTO, numwto)

#define FLI_IOCTL_GET_CMDS                              \
  FLI_IOCTL_CMD(FLI_GET_DMABUFFSIZE, dmabuffsize)       \
  FLI_IOCTL_CMD(FLI_GET_DMATHRESH, dmathresh)           \
  FLI_IOCTL_CMD(FLI_GET_DTO, dto)                       \
  FLI_IOCTL_CMD(FLI_GET_RTO, rto)                       \
  FLI_IOCTL_CMD(FLI_GET_WTO, wto)                       \
  FLI_IOCTL_CMD(FLI_GET_DIR, dir)                       \
  FLI_IOCTL_CMD(FLI_GET_LTL, ltl)                       \
  FLI_IOCTL_CMD(FLI_GET_NUMREAD, numread)               \
  FLI_IOCTL_CMD(FLI_GET_NUMWRITE, numwrite)             \
  FLI_IOCTL_CMD(FLI_GET_NUMDTO, numdto)                 \
  FLI_IOCTL_CMD(FLI_GET_NUMRTO, numrto)                 \
  FLI_IOCTL_CMD(FLI_GET_NUMWTO, numwto)

/* Enumerate ioctl numbers */
#undef FLI_SET_CMD
#define FLI_IOCTL_CMD(cmd, var) cmd##_NUM,

enum {
  FLI_IOCTL_MISC_CMDS
  FLI_IOCTL_SPECIAL_SET_CMDS
  FLI_IOCTL_SET_CMDS
  FLI_IOCTL_GET_CMDS
};

/* Enumerate the actual ioctl commands  */
#undef FLI_IOCTL_CMD
#define FLI_IOCTL_CMD(cmd, var)                 \
  enum {cmd = _IO(FLI_IOCTL_TYPE, cmd##_NUM)};

FLI_IOCTL_MISC_CMDS;

#undef FLI_IOCTL_CMD
#define FLI_IOCTL_CMD(cmd, var)                         \
  enum {cmd = _IOW(FLI_IOCTL_TYPE, cmd##_NUM, int)};

FLI_IOCTL_SPECIAL_SET_CMDS;
FLI_IOCTL_SET_CMDS;

#undef FLI_IOCTL_CMD
#define FLI_IOCTL_CMD(cmd, var )                        \
  enum {cmd = _IOR(FLI_IOCTL_TYPE, cmd##_NUM, int)};

FLI_IOCTL_GET_CMDS;

#endif /* _FLI_IOCTL_H */
