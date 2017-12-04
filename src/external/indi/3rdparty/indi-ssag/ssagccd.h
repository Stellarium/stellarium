/*
  SSAG CCD INDI Driver

  Copyright (c) 2016 Peter Polakovic, CloudMakers, s. r. o.
  All Rights Reserved.

  Code is based on OpenSSAG modified for libusb 1.0
  Copyright (c) 2011 Eric J. Holmes, Orion Telescopes & Binoculars
  All rights reserved.
 
  https://github.com/ejholmes/openssag

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
*/

#ifndef SSAGCCD_H_
#define SSAGCCD_H_

#include <indiccd.h>

#include "openssag.h"

void ExposureTimerCallback(void *p);
void GuideExposureTimerCallback(void *p);
void WEGuiderTimerCallback(void *p);
void NSGuiderTimerCallback(void *p);

class SSAGCCD : public INDI::CCD
{
  private:
    OpenSSAG::SSAG *ssag;
    float remaining;

    INumber GainN[1];
    INumberVectorProperty GainNP;

  public:
    SSAGCCD();

    const char *getDefaultName();
    bool initProperties();
    bool updateProperties();
    bool Connect();
    bool Disconnect();
    bool StartExposure(float duration);
    bool AbortExposure();
    void TimerHit();
    bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    IPState GuideWest(float time);
    IPState GuideEast(float time);
    IPState GuideNorth(float time);
    IPState GuideSouth(float time);

    void addFITSKeywords(fitsfile *fptr, INDI::CCDChip *targetChip);
    bool saveConfigItems(FILE *fp);

  protected:
    friend void ::ISGetProperties(const char *dev);
    friend void ::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num);
    friend void ::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num);
    friend void ::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num);
    friend void ::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                            char *formats[], char *names[], int n);
    friend void ::ISSnoopDevice(XMLEle *root);
};

#endif /* SSAGCCD_H_ */
