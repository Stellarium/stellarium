/*******************************************************************************
 Copyright(c) 2014 Jasem Mutlaq. All rights reserved.

 Baader Planetarium Dome INDI Driver

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#pragma once

#include "indibase/indidome.h"

class BaaderDome : public INDI::Dome
{
  public:
    typedef enum { DOME_UNKNOWN, DOME_CALIBRATING, DOME_READY } DomeStatus;
    typedef enum {
        CALIBRATION_UNKNOWN,
        CALIBRATION_STAGE1,
        CALIBRATION_STAGE2,
        CALIBRATION_STAGE3,
        CALIBRATION_COMPLETE
    } CalibrationStage;
    typedef enum { FLAP_OPEN, FLAP_CLOSE } FlapOperation;
    typedef enum { FLAP_OPENED, FLAP_CLOSED, FLAP_MOVING, FLAP_UNKNOWN } FlapStatus;

    BaaderDome();
    virtual ~BaaderDome() = default;

    virtual const char *getDefaultName() override;
    virtual bool initProperties() override;
    virtual bool updateProperties() override;
    virtual bool saveConfigItems(FILE *fp) override;

    virtual bool Handshake() override;

    virtual void TimerHit() override;

    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;

    virtual IPState MoveRel(double azDiff) override;
    virtual IPState MoveAbs(double az) override;
    virtual IPState ControlShutter(ShutterOperation operation) override;
    virtual bool Abort() override;

    // Parking
    virtual IPState Park() override;
    virtual IPState UnPark() override;
    virtual bool SetCurrentPark() override;
    virtual bool SetDefaultPark() override;

  protected:
    // Commands
    bool Ack();
    bool UpdatePosition();
    bool UpdateShutterStatus();
    int ControlDomeFlap(FlapOperation operation);
    bool UpdateFlapStatus();
    bool SaveEncoderPosition();
    const char *GetFlapStatusString(FlapStatus status);

    // Misc
    unsigned short MountAzToDomeAz(double mountAz);
    double DomeAzToMountAz(unsigned short domeAz);
    bool SetupParms();

    ISwitch CalibrateS[1];
    ISwitchVectorProperty CalibrateSP;

    ISwitch DomeFlapS[2];
    ISwitchVectorProperty DomeFlapSP;

    DomeStatus status { DOME_UNKNOWN };
    FlapStatus flapStatus { FLAP_OPENED };
    CalibrationStage calibrationStage { CALIBRATION_UNKNOWN };
    double targetAz { 0 };
    double calibrationStart { 0 };
    double calibrationTarget1 { 0 };
    double calibrationTarget2 { 0 };
    ShutterOperation targetShutter { SHUTTER_OPEN };
    FlapOperation targetFlap { FLAP_OPEN };
    bool sim { false };
    double simShutterTimer { 0 };
    double simFlapTimer { 0 };
    ShutterStatus simShutterStatus { SHUTTER_OPENED };
    FlapStatus simFlapStatus { FLAP_OPENED };
};
