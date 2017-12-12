/*
   Optec Gemini Focuser Rotator INDI driver
   Copyright (C) 2017 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include <map>

class Gemini : public INDI::Focuser, public INDI::RotatorInterface
{
  public:
    Gemini();    
    ~Gemini();

    enum
    {
        FOCUS_A_COEFF,
        FOCUS_B_COEFF,
        FOCUS_C_COEFF,
        FOCUS_D_COEFF,
        FOCUS_E_COEFF,
        FOCUS_F_COEFF
    };
    enum
    {
        STATUS_MOVING,
        STATUS_HOMING,
        STATUS_HOMED,
        STATUS_FFDETECT,
        STATUS_TMPPROBE,
        STATUS_REMOTEIO,
        STATUS_HNDCTRL,
        STATUS_REVERSE,
        STATUS_UNKNOWN
    };
    enum
    {
        GOTO_CENTER,
        GOTO_HOME
    };    
    typedef enum
    {
        DEVICE_FOCUSER,
        DEVICE_ROTATOR
    } DeviceType;

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;

protected:

    virtual bool Handshake() override;
    virtual const char *getDefaultName() override;

    virtual bool initProperties() override;    
    virtual bool updateProperties() override;
    virtual bool saveConfigItems(FILE *fp) override;

    // Focuser Functions
    virtual IPState MoveAbsFocuser(uint32_t targetPosition) override;
    virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks) override;
    virtual IPState MoveFocuser(FocusDirection dir, int speed, uint16_t duration) override;
    virtual bool AbortFocuser() override;

    virtual void TimerHit() override;    

    // Misc functions
    bool ack();
    bool isResponseOK();

  protected:
    // Move from private to public to validate
    bool focuserConfigurationComplete = false;
    bool rotatorConfigurationComplete = false;

    // Rotator Overrides
    virtual IPState HomeRotator() override;
    virtual IPState MoveRotator(double angle) override;
    virtual bool ReverseRotator(bool enabled) override;

  private:
    uint32_t focuserSimPosition=0;
    uint32_t rotatorSimPosition=0;
    uint32_t rotatorSimPA=0;
    uint32_t targetFocuserPosition=0;
    uint32_t targetRotatorPosition=0;
    uint32_t targetRotatorAngle=0;
    uint32_t maxControllerTicks=0;

    ISState focuserSimStatus[8];
    ISState rotatorSimStatus[8];

    bool simCompensationOn;
    char focusTarget[8];

    struct timeval focusMoveStart;
    float focusMoveRequest;

    ////////////////////////////////////////////////////////////
    // Focuser Functions
    ///////////////////////////////////////////////////////////

    // Get functions
    bool getFocusConfig();
    bool getFocusStatus();

    // Set functions

    // Position
    bool setFocusPosition(u_int16_t position);

    // Temperature
    bool setTemperatureCompensation(bool enable);
    bool setTemperatureCompensationMode(char mode);
    bool setTemperatureCompensationCoeff(char mode, int16_t coeff);
    bool setTemperatureCompensationOnStart(bool enable);

    // Backlash
    bool setBacklashCompensation(DeviceType type, bool enable);
    bool setBacklashCompensationSteps(DeviceType type, uint16_t steps);

    // Motion functions
    bool home(DeviceType type);
    bool halt(DeviceType type);
    bool center(DeviceType type);
    bool homeOnStart(DeviceType type, bool enable);

    ////////////////////////////////////////////////////////////
    // Focuser Properties
    ///////////////////////////////////////////////////////////

    // Set/Get Temperature
    INumber TemperatureN[1];
    INumberVectorProperty TemperatureNP;

    // Enable/Disable temperature compnesation
    ISwitch TemperatureCompensateS[2];
    ISwitchVectorProperty TemperatureCompensateSP;

    // Enable/Disable temperature compnesation on start
    ISwitch TemperatureCompensateOnStartS[2];
    ISwitchVectorProperty TemperatureCompensateOnStartSP;

    // Temperature Coefficient
    INumber TemperatureCoeffN[5];
    INumberVectorProperty TemperatureCoeffNP;

    // Temperature Coefficient Mode
    ISwitch TemperatureCompensateModeS[5];
    ISwitchVectorProperty TemperatureCompensateModeSP;

    // Enable/Disable backlash
    ISwitch FocuserBacklashCompensationS[2];
    ISwitchVectorProperty FocuserBacklashCompensationSP;

    // Backlash Value
    INumber FocuserBacklashN[1];
    INumberVectorProperty FocuserBacklashNP;

    // Home On Start
    ISwitch FocuserHomeOnStartS[2];
    ISwitchVectorProperty FocuserHomeOnStartSP;

    // Go to home/center
    ISwitch FocuserGotoS[2];
    ISwitchVectorProperty FocuserGotoSP;

    // Status indicators
    ILight FocuserStatusL[8];
    ILightVectorProperty FocuserStatusLP;

    bool isFocuserAbsolute;
    bool isFocuserHoming;

    ////////////////////////////////////////////////////////////
    // Rotator Functions
    ///////////////////////////////////////////////////////////

    // Get functions
    bool getRotatorConfig();
    bool getRotatorStatus();

    IPState MoveAbsRotatorTicks(uint32_t targetTicks);
    IPState MoveAbsRotatorAngle(double angle);
    bool reverseRotator(bool enable);

    ////////////////////////////////////////////////////////////
    // Rotator Properties
    ///////////////////////////////////////////////////////////

    // Status
    ILight RotatorStatusL[8];
    ILightVectorProperty RotatorStatusLP;

    // Rotator Steps
    INumber RotatorAbsPosN[1];
    INumberVectorProperty RotatorAbsPosNP;

#if 0
    // Reverse Direction
    ISwitch RotatorReverseS[2];
    ISwitchVectorProperty RotatorReverseSP;



    // Rotator Degrees or PA (Position Angle)
    INumber RotatorAbsAngleN[1];
    INumberVectorProperty RotatorAbsAngleNP;

    // Abort
    ISwitch AbortRotatorS[1];
    ISwitchVectorProperty AbortRotatorSP;

    // Go to home/center
    ISwitch RotatorGotoS[2];
    ISwitchVectorProperty RotatorGotoSP;
#endif

    // Enable/Disable backlash
    ISwitch RotatorBacklashCompensationS[2];
    ISwitchVectorProperty RotatorBacklashCompensationSP;

    // Backlash Value
    INumber RotatorBacklashN[1];
    INumberVectorProperty RotatorBacklashNP;

    // Home On Start
    ISwitch RotatorHomeOnStartS[2];
    ISwitchVectorProperty RotatorHomeOnStartSP;

    bool isRotatorHoming;

    ////////////////////////////////////////////////////////////
    // Hub Functions
    ///////////////////////////////////////////////////////////

    // Led level
    bool setLedLevel(int level);

    // Device Nickname
    bool setNickname(DeviceType type, const char *nickname);

    // Misc functions
    bool resetFactory();
    float calcTimeLeft(timeval, float);

    ////////////////////////////////////////////////////////////
    // Hub Properties
    ///////////////////////////////////////////////////////////

    // Reset to Factory setting
    ISwitch ResetS[1];
    ISwitchVectorProperty ResetSP;

    // Focus and rotator name configure in the HUB
    IText HFocusNameT[2];
    ITextVectorProperty HFocusNameTP;

    // Led Intensity Value
    INumber LedN[1];
    INumberVectorProperty LedNP;

    uint32_t DBG_FOCUS;
};
