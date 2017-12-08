/*******************************************************************************
  Copyright(c) 2016 Gerry Rozema. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.

*******************************************************************************/

#pragma once

#include "indiguiderinterface.h"
#include "inditelescope.h"
#include "alignment/AlignmentSubsystemForDrivers.h"

#define TEMMA_SLEW_RATES 4

class TemmaMount : public INDI::Telescope,
                   public INDI::GuiderInterface,
                   public INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers
{
  public:
    TemmaMount();
    virtual ~TemmaMount() = default;

    //bool initProperties();
    virtual bool Handshake();
    virtual void ISGetProperties(const char *dev);
    virtual bool updateProperties();
    virtual const char *getDefaultName();

    virtual bool initProperties();
    virtual bool ReadScopeStatus();
    bool Goto(double, double);
    bool Park();
    bool UnPark();
    bool Abort();
    bool SetSlewRate(int);
    bool MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command);
    bool MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command);
    //bool ReadTime();
    //bool ReadLocation();
    bool updateLocation(double latitude, double longitude, double elevation);
    bool updateTime(ln_date *utc, double utc_offset);
    bool SetCurrentPark();
    bool SetDefaultPark();

    //  methods added for alignment subsystem
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    bool Sync(double ra, double dec);
    //  methods added for guider interface
    virtual IPState GuideNorth(float ms);
    virtual IPState GuideSouth(float ms);
    virtual IPState GuideEast(float ms);
    virtual IPState GuideWest(float ms);
    //  Initial implementation doesn't need this one
    //virtual void GuideComplete(INDI_EQ_AXIS axis);

private:
    int TemmaRead(char *buf, int size);
    bool GetTemmaVersion();
    bool GetTemmaMotorStatus();
    bool SetTemmaMotorStatus(bool);
    bool SetTemmaLst();
    int GetTemmaLst();
    bool SetTemmaLattitude(double);
    double GetTemmaLattitude();

    bool TemmaSync(double, double);

    ln_equ_posn TelescopeToSky(double ra, double dec);
    ln_equ_posn SkyToTelescope(double ra, double dec);

    //bool TemmaConnect(const char *port);

    double currentRA { 0 };
    double currentDEC { 0 };

    bool MotorStatus { false };
    bool GotoInProgress { false };
    bool ParkInProgress { false };
    bool TemmaInitialized { false };
    double Longitude { 0 };
    double Latitude { 0 };
    int SlewRate { 1 };
    bool SlewActive { false };
    unsigned char Slewbits { 0 };
    //INumber GuideRateN[2];
    //INumberVectorProperty GuideRateNP;
};
