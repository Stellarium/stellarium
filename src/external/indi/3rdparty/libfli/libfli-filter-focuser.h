/*

  Copyright (c) 2002 Finger Lakes Instrumentation (FLI), L.L.C.
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

  Finger Lakes Instrumentation, L.L.C. (FLI)
  web: http://www.fli-cam.com
  email: support@fli-cam.com

*/

#ifndef _FLI_FILTER_FOCUSER_H_
#define _FLI_FILTER_FOCUSER_H_

#define FLI_FILTERPOSITION_HOME (-1)

/* Filter wheel and focuser parameters */
typedef struct _flifilterfocuserdata_t {
  long tableindex;
  long stepspersec;
  long currentslot;
	long numslots;
	long numtempsensors;
	long extent;
	long hwtype;
	long uselong;
} flifilterfocuserdata_t;

typedef struct {
  int n_pos;
  int n_offset;
  int n_steps[32];
} wheeldata_t;

long fli_filter_focuser_open(flidev_t dev);
#define fli_filter_open fli_filter_focuser_open
#define fli_focuser_open fli_filter_focuser_open

long fli_filter_focuser_close(flidev_t dev);
#define fli_filter_close fli_filter_focuser_close
#define fli_focuser_close fli_filter_focuser_close

long fli_filter_command(flidev_t dev, int cmd, int argc, ...);
long fli_focuser_command(flidev_t dev, int cmd, int argc, ...);
long fli_filter_focuser_probe(flidev_t dev);

#endif /* _FLI_FILTER_FOCUSER_H_ */
