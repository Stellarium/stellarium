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

#include <stdio.h>
#include <unistd.h>
#include <memory>
#include <stdint.h>
#include <math.h>

#include "ssagccd.h"

#define WIDTH  1280
#define HEIGHT 1024

std::unique_ptr<SSAGCCD> camera(new SSAGCCD());

void ISGetProperties(const char *dev)
{
    camera->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    camera->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    camera->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    camera->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}

void ISSnoopDevice(XMLEle *root)
{
    camera->ISSnoopDevice(root);
}

SSAGCCD::SSAGCCD()
{
    ssag = new OpenSSAG::SSAG();
}

const char *SSAGCCD::getDefaultName()
{
    return "SSAG CCD";
}

bool SSAGCCD::initProperties()
{
    INDI::CCD::initProperties();
    IUFillNumber(&GainN[0], "CCD_GAIN", "Gain", "%.0f", 1, 15, 1, 1);
    IUFillNumberVector(&GainNP, GainN, 1, getDeviceName(), "CCD_GAIN", "Gain", MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);
    addDebugControl();
    SetCCDCapability(CCD_HAS_ST4_PORT | CCD_CAN_ABORT);
    SetCCDParams(WIDTH, HEIGHT, 8, 5.2, 5.2);
    setDriverInterface(CCD_INTERFACE | GUIDER_INTERFACE);
    return true;
}

bool SSAGCCD::updateProperties()
{
    INDI::CCD::updateProperties();
    if (isConnected())
    {
        defineNumber(&GainNP);
        PrimaryCCD.setInterlaced(false);
        PrimaryCCD.setFrame(0, 0, WIDTH, HEIGHT);
        PrimaryCCD.setFrameBufferSize(WIDTH * HEIGHT + 512);
    }
    else
    {
        deleteProperty(GainNP.name);
    }
    return true;
}

bool SSAGCCD::Connect()
{
    if (isConnected())
        return true;
    if (!ssag->Connect())
    {
        IDMessage(getDeviceName(), "Failed to connect to SSAG");
        return false;
    }
    return true;
}

bool SSAGCCD::StartExposure(float duration)
{
    ExposureTime = duration;
    InExposure   = true;
    ssag->StartExposure(duration * 1000);
    PrimaryCCD.setExposureDuration(duration);
    PrimaryCCD.setExposureLeft(duration);
    if (duration < 1)
    {
        remaining = 0;
        SetTimer(duration * 1000);
    }
    else
    {
        remaining = duration - 1;
        SetTimer(1000);
    }
    return true;
}

bool SSAGCCD::AbortExposure()
{
    return true;
}

void SSAGCCD::TimerHit()
{
    if (InExposure)
    {
        PrimaryCCD.setExposureLeft(remaining);
        if (remaining == 0)
        {
            unsigned char *data = ssag->ReadBuffer(0);
            if (data)
            {
                memcpy((unsigned char *)PrimaryCCD.getFrameBuffer(), data, WIDTH * HEIGHT);
                InExposure = false;
                ExposureComplete(&PrimaryCCD);
            }
            else
            {
                PrimaryCCD.setExposureFailed();
            }
        }
        else
        {
            remaining -= 1;
            SetTimer(1000);
        }
    }
}

bool SSAGCCD::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (!strcmp(dev, getDeviceName()))
    {
        if (!strcmp(name, GainNP.name))
        {
            IUUpdateNumber(&GainNP, values, names, n);
            ssag->SetGain(GainN[0].value);
            IDSetNumber(&GainNP, "Gain set to %.0f", GainN[0].value);
            return true;
        }
    }
    return INDI::CCD::ISNewNumber(dev, name, values, names, n);
}

void SSAGCCD::addFITSKeywords(fitsfile *fptr, INDI::CCDChip *targetChip)
{
    int status;
    fits_update_key_s(fptr, TDOUBLE, "GAIN   ", &GainN[0].value, "Gain", &status);
    return INDI::CCD::addFITSKeywords(fptr, targetChip);
}

IPState SSAGCCD::GuideWest(float time)
{
    ssag->Guide(OpenSSAG::guide_west, (int)time);
    return IPS_OK;
}

IPState SSAGCCD::GuideEast(float time)
{
    ssag->Guide(OpenSSAG::guide_east, (int)time);
    return IPS_OK;
}

IPState SSAGCCD::GuideNorth(float time)
{
    ssag->Guide(OpenSSAG::guide_north, (int)time);
    return IPS_OK;
}

IPState SSAGCCD::GuideSouth(float time)
{
    ssag->Guide(OpenSSAG::guide_south, (int)time);
    return IPS_OK;
}

bool SSAGCCD::Disconnect()
{
    if (isConnected())
    {
        ssag->Disconnect();
    }
    setConnected(false);
    return true;
}

bool SSAGCCD::saveConfigItems(FILE *fp)
{
    INDI::CCD::saveConfigItems(fp);
    IUSaveConfigNumber(fp, &GainNP);
    return true;
}
