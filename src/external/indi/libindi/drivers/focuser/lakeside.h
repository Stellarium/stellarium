/*
    Lakeside Focuser
    Copyright (C) 2017 Phil Shepherd (psjshep@googlemail.com)
    Technical Information kindly supplied by Peter Chance at LakesideAstro (info@lakeside-astro.com)

    Code template from original Moonlite code by Jasem Mutlaq (mutlaqja@ikarustech.com)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#pragma once

#include "indifocuser.h"

#include <cstdint>

class Lakeside : public INDI::Focuser
{
public:
    Lakeside();
    ~Lakeside() = default;

    const char * getDefaultName();
    virtual bool initProperties();
    virtual bool updateProperties();
    virtual bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n);    

protected:

    virtual bool Handshake();
    virtual IPState MoveAbsFocuser(uint32_t ticks);
    virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks);
    virtual bool AbortFocuser();
    virtual void TimerHit();

private:

    double targetPos, lastPos, lastTemperature;
    uint32_t currentSpeed;

    struct timeval focusMoveStart;
    float focusMoveRequest;

    void GetFocusParams();
    bool updateMoveDirection();
    bool updateMaxTravel();
    bool updateStepSize();
    bool updateBacklash();
    bool updateTemperature();
    bool updateTemperatureK();
    bool updatePosition();
    bool LakesideOnline();

    bool updateActiveTemperatureSlope();
    bool updateSlope1Inc();
    bool updateSlope1Dir();
    bool updateSlope1Deadband();
    bool updateSlope1Period();
    bool updateSlope2Inc();
    bool updateSlope2Dir();
    bool updateSlope2Deadband();
    bool updateSlope2Period();

    bool GetLakesideStatus();

    char DecodeBuffer(char* in_response);
    bool SendCmd(const char *in_cmd);
    bool ReadBuffer(char* response);
  
    bool gotoPosition(uint32_t position);

    bool setCalibration();

    bool setTemperatureTracking(bool enable);
    bool setBacklash(int backlash );
    bool setStepSize(int stepsize );
    bool setMaxTravel(int);
    bool setMoveDirection(int direction);

    bool setActiveTemperatureSlope(uint32_t active_slope);
    bool setSlope1Inc(uint32_t slope1_inc);
    bool setSlope1Dir(uint32_t slope1_direction);
    bool setSlope1Deadband(uint32_t slope1_deadband);
    bool setSlope1Period(uint32_t slope1_period);
    bool setSlope2Inc(uint32_t slope2_inc);
    bool setSlope2Dir(uint32_t slope2_direction);
    bool setSlope2Deadband(uint32_t slope2_deadband);
    bool setSlope2Period(uint32_t slope2_period);

    INumber TemperatureN[1];
    INumberVectorProperty TemperatureNP;

    INumber TemperatureKN[1];
    INumberVectorProperty TemperatureKNP;

    ISwitch MoveDirectionS[2];
    ISwitchVectorProperty MoveDirectionSP;

    INumber StepSizeN[1];
    INumberVectorProperty StepSizeNP;

    INumber BacklashN[1];
    INumberVectorProperty BacklashNP;

    INumber MaxTravelN[1];
    INumberVectorProperty MaxTravelNP;

    ISwitch TemperatureTrackingS[2];
    ISwitchVectorProperty TemperatureTrackingSP;

    ISwitch ActiveTemperatureSlopeS[2];
    ISwitchVectorProperty ActiveTemperatureSlopeSP;

    INumber Slope1IncN[1];
    INumberVectorProperty Slope1IncNP;

    ISwitch Slope1DirS[2];
    ISwitchVectorProperty Slope1DirSP;

    INumber Slope1DeadbandN[1];
    INumberVectorProperty Slope1DeadbandNP;

    INumber Slope1PeriodN[1];
    INumberVectorProperty Slope1PeriodNP;

    INumber Slope2IncN[1];
    INumberVectorProperty Slope2IncNP;

    ISwitch Slope2DirS[2];
    ISwitchVectorProperty Slope2DirSP;

    INumber Slope2DeadbandN[1];
    INumberVectorProperty Slope2DeadbandNP;

    INumber Slope2PeriodN[1];
    INumberVectorProperty Slope2PeriodNP;

};
