/*
    Microtouch Focuser
    Copyright (C) 2016 Marco Peters (mpeters@rzpeters.de)
    Copyright (C) 2013 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#define CMD_GET_STATUS      0x80
#define CMD_RESET_POSITION  0x81
#define CMD_IS_MOVING       0x82
#define CMD_HALT            0x83
#define CMD_GET_TEMPERATURE 0x84
#define CMD_SET_COEFF       0x85
#define CMD_GET_COEFF       0x86
#define CMD_TEMPCOMP_ON     0x87
#define CMD_TEMPCOMP_OFF    0x88
#define CMD_UPDATE_POSITION 0x8c
#define CMD_GET_POSITION    0x8d
#define CMD_SET_MOTOR_SPEED 0x9d
#define CMD_GET_MOTOR_SPEED 0x9e
#define CMD_SET_TEMP_OFFSET 0x9f

#define FOCUS_MOTORSPEED_NORMAL 8
#define FOCUS_MOTORSPEED_FAST   4

class Microtouch : public INDI::Focuser
{
  public:
    Microtouch();
    virtual ~Microtouch() = default;

    virtual bool Handshake();
    const char *getDefaultName();
    virtual bool initProperties();
    virtual bool updateProperties();
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual IPState MoveFocuser(FocusDirection dir, int speed, uint16_t duration);
    virtual IPState MoveAbsFocuser(uint32_t targetTicks);
    virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks);
    virtual bool SetFocuserSpeed(int speed);
    virtual bool AbortFocuser();
    virtual void TimerHit();

  private:
    void GetFocusParams();
    bool reset();
    bool reset(double pos);
    bool updateMotorSpeed();
    bool updateTemperature();
    bool updatePosition();
    bool updateSpeed();
    bool isMoving();
    bool Ack();

    bool MoveFocuser(unsigned int position);
    bool setMotorSpeed(char speed);
    bool setSpeed(unsigned short speed);
    bool setTemperatureCalibration(double calibration);
    bool setTemperatureCoefficient(double coefficient);
    bool setTemperatureCompensation(bool enable);
    float CalcTimeLeft(timeval, float);
    bool WriteCmd(char cmd);
    bool WriteCmdGetResponse(char cmd, char *readbuffer, char numbytes);
    char WriteCmdGetByte(char cmd);
    bool WriteCmdSetByte(char cmd, char val);
    signed short int WriteCmdGetShortInt(char cmd);
    bool WriteCmdSetShortInt(char cmd, short int val);
    int WriteCmdGetInt(char cmd);
    bool WriteCmdSetInt(char cmd, int val);
    bool WriteCmdSetIntAsDigits(char cmd, int val);

    double targetPos { 0 };
    double lastPos { 0 };
    double lastTemperature { 0 };
    unsigned int currentSpeed { 0 };

    struct timeval focusMoveStart { 0, 0 };
    float focusMoveRequest { 0 };

    INumber TemperatureN[1];
    INumberVectorProperty TemperatureNP;

    ISwitch MotorSpeedS[2];
    ISwitchVectorProperty MotorSpeedSP;

    INumber MaxTravelN[1];
    INumberVectorProperty MaxTravelNP;

    INumber TemperatureSettingN[2];
    INumberVectorProperty TemperatureSettingNP;

    ISwitch TemperatureCompensateS[2];
    ISwitchVectorProperty TemperatureCompensateSP;

    ISwitch ResetS[1];
    ISwitchVectorProperty ResetSP;

    INumber ResetToPosN[1];
    INumberVectorProperty ResetToPosNP;
};
