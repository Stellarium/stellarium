/*
    Gemini Telescope Design Integra85 Focusing Rotator.
    Copyright (C) 2017 Hans Lambermont

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
#include "indirotatorinterface.h"

class Integra : public INDI::Focuser, public INDI::RotatorInterface
{
  public:

    typedef enum { MOTOR_FOCUS, MOTOR_ROTATOR } MotorType;

    enum INTEGRA_HOMING_STATE
    {
        HOMING_IDLE,
        HOMING_START,
        HOMING_ABORT,
        HOMING_COUNT
    };

    Integra();
    virtual ~Integra() = default;

    virtual bool Handshake();
    const char * getDefaultName();
    virtual bool initProperties();
    virtual bool updateProperties();
    virtual bool ISNewNumber (const char * dev, const char * name, double values[], char * names[], int n);
    virtual bool ISNewSwitch (const char * dev, const char * name, ISState * states, char * names[], int n);

  protected:
    // Focuser
    virtual IPState MoveAbsFocuser(uint32_t targetTicks);
    virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks);
    virtual bool AbortFocuser();

    // Rotator
    virtual IPState MoveRotator(double angle);
    virtual bool AbortRotator();
    virtual bool SyncRotator(double angle);
    virtual bool ReverseRotator(bool enabled);

    // Misc.
    virtual bool saveConfigItems(FILE *fp);
    virtual void TimerHit();

  private:
    bool genericIntegraCommand(const char *name, const char *cmd, const char *expectStart, char *returnValueString);
    bool getFirmware();
    bool getFocuserType();
    bool Ack();
    bool gotoMotor(MotorType type, int32_t position);
    bool relativeGotoMotor(MotorType type, int32_t relativePosition);
    bool getPosition(MotorType type);
    bool stopMotor(MotorType type);
    bool isMotorMoving(MotorType type);
    bool getTemperature();
    bool findHome();
    bool abortHome();
    bool isHomingComplete();
    void cleanPrint(const char *cmd, char *cleancmd);
    bool saveToEEPROM();
    bool getMaxPosition(MotorType type);
    uint32_t rotatorDegreesToTicks(double angle);
    double rotatorTicksToDegrees(uint32_t ticks);

    INumber MaxPositionN[2];
    INumberVectorProperty MaxPositionNP;

    INumber SensorN[2];
    INumberVectorProperty SensorNP;
    enum { SENSOR_TEMPERATURE };

    ISwitch FindHomeS[HOMING_COUNT];
    ISwitchVectorProperty FindHomeSP;

    INumber RotatorAbsPosN[1];
    INumberVectorProperty RotatorAbsPosNP;

    double lastTemperature { 0 };
    int timeToReadTemperature = 0;
    double rotatorTicksPerDegree { 0 };
    double rotatorDegreesPerTick = 0.0;
    uint32_t lastFocuserPosition { 0 };
    bool haveReadFocusPositionAtLeastOnce = false;
    uint32_t lastRotatorPosition { 0 };
    bool haveReadRotatorPositionAtLeastOnce = false;
    uint32_t targetPosition { 0 };
};
