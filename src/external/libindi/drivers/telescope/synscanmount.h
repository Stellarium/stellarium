/*******************************************************************************
  Copyright(c) 2010 Gerry Rozema. All rights reserved.

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

#include "inditelescope.h"

class SynscanMount : public INDI::Telescope
{
  public:
    SynscanMount();
    virtual ~SynscanMount() = default;

    //  overrides of base class virtual functions
    //bool initProperties();
    virtual void ISGetProperties(const char *dev) override;
    virtual bool updateProperties() override;
    virtual const char *getDefaultName() override;

    virtual bool Connect() override;
    virtual bool initProperties() override;
    virtual bool ReadScopeStatus() override;
    bool StartTrackMode();
    virtual bool Goto(double, double) override;
    virtual bool Park() override;
    virtual bool UnPark() override;
    virtual bool Abort() override;
    virtual bool SetSlewRate(int index) override;
    virtual bool MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command) override;
    virtual bool MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command) override;
    bool ReadTime();
    bool ReadLocation();
    virtual bool updateLocation(double latitude, double longitude, double elevation) override;
    virtual bool updateTime(ln_date *utc, double utc_offset) override;
    virtual bool SetCurrentPark() override;
    virtual bool SetDefaultPark() override;

    //  methods added for alignment subsystem
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n) override;
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;
    virtual bool Sync(double ra, double dec) override;

  private:
    int PassthruCommand(int cmd, int target, int msgsize, int data, int numReturn);
    ln_hrz_posn GetAltAzPosition(double ra, double dec);
    int HexStrToInteger(const std::string &str);
    bool AnalyzeHandset();
    void UpdateMountInformation(bool inform_client);

    double FirmwareVersion { 0 };
    char LastParkRead[20];
    int NumPark { 0 };
    int StopCount { 0 };
    int SlewRate { 5 };
    int CustomNSSlewRate { -1 };
    int CustomWESlewRate { -1 };
    double SlewTargetAlt { -1 };
    double SlewTargetAz { -1 };
    double CurrentRA { 0 };
    double CurrentDEC { 0 };
    bool CanSetLocation { false };
    bool ReadLatLong { false };
    int RecoverTrials { 0 };
//    bool HasFailed { true };
    bool NewFirmware { false };
    const std::string MountInfoPage { "Mount Information" };
    enum class MountInfoItems
    {
        FwVersion,
        MountCode,
        AlignmentStatus,
        GotoStatus,
        MountPointingStatus,
        TrackingMode
    };
    IText BasicMountInfo[6];
    ITextVectorProperty BasicMountInfoV;
    std::string HandsetFwVersion;
    int MountCode { 0 };
    std::string AlignmentStatus;
    std::string GotoStatus;
    std::string MountPointingStatus;
    std::string TrackingMode;
};
