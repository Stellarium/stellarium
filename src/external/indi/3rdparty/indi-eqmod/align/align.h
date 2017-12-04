/* Copyright 2012 Geehalel (geehalel AT gmail DOT com) */
/* This file is part of the Skywatcher Protocol INDI driver.

    The Skywatcher Protocol INDI driver is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The Skywatcher Protocol INDI driver is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Skywatcher Protocol INDI driver.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "pointset.h"

#include <inditelescope.h>

typedef struct SyncData SyncData;

class Align
{
  protected:
  private:
    enum AlignmentMode
    {
        NONE = 0,
        SYNCS,
        NEAREST,
        NSTAR
    };

    INDI::Telescope *telescope;
    PointSet *pointset;
    ITextVectorProperty *AlignDataFileTP;
    IBLOBVectorProperty *AlignDataBP;
    INumberVectorProperty *AlignPointNP;
    ISwitchVectorProperty *AlignListSP;
    INumberVectorProperty *AlignTelescopeCoordsNP;
    ISwitchVectorProperty *AlignModeSP;
    INumberVectorProperty *AlignCountNP;

    AlignData syncdata;

    enum AlignmentMode GetAlignmentMode();

    double currentdeltaRA, currentdeltaDEC;

    int lastnearestindex;

  public:
    Align(INDI::Telescope *);
    virtual ~Align();

    const char *getDeviceName(); // used for logger

    virtual bool initProperties();
    virtual bool updateProperties();
    virtual void ISGetProperties();
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n);

    virtual bool saveConfigItems(FILE *fp);

    virtual void Init();
    virtual void GetAlignedCoords(SyncData globalsync, double jd, struct ln_lnlat_posn *position, double currentRA,
                                  double currentDEC, double *alignedRA, double *alignedDEC);
    virtual void AlignNStar(double jd, struct ln_lnlat_posn *position, double currentRA, double currentDEC,
                            double *alignedRA, double *alignedDEC, bool ingoto);
    virtual void AlignNearest(double jd, struct ln_lnlat_posn *position, double currentRA, double currentDEC,
                              double *alignedRA, double *alignedDEC, bool ingoto);
    virtual void AlignGoto(SyncData globalsync, double jd, struct ln_lnlat_posn *position, double *gotoRA,
                           double *gotoDEC);
    //virtual void AlignSync(double lst, double jd, double targetRA, double targetDEC, double telescopeRA, double telescopeDEC);
    virtual void AlignSync(SyncData globalsync, SyncData thissync);
    virtual void AlignStandardSync(SyncData globalsync, SyncData *thissync, struct ln_lnlat_posn *position);
};
